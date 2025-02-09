/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <tinyara/clock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ble_manager/ble_manager.h>
#include <semaphore.h>
#include <errno.h>

#define RMC_TAG "[RMC]"
#define RMC_LOG(tag, fmt, args...) printf("\x1b[32m"tag"\x1b[0m" fmt, ##args)

static sem_t g_conn_sem = { 0, };
static ble_conn_handle g_conn = 0;
static int g_scan_done = -1;
static uint8_t g_target[BLE_BD_ADDR_MAX_LEN] = { 0, };

static void ble_scan_state_changed_cb(ble_scan_state_e scan_state)
{
	RMC_LOG(RMC_TAG, "'%s' is called[%d]\n", __FUNCTION__, scan_state);
	return;
}

static void ble_device_scanned_cb(ble_scanned_device *scanned_device)
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	printf("scan mac : %02x:%02x:%02x:%02x:%02x:%02x\n", 
		scanned_device->conn_info.addr.mac[0],
		scanned_device->conn_info.addr.mac[1],
		scanned_device->conn_info.addr.mac[2],
		scanned_device->conn_info.addr.mac[3],
		scanned_device->conn_info.addr.mac[4],
		scanned_device->conn_info.addr.mac[5]
	);
	if (g_scan_done == 0) {
		g_target[0] = scanned_device->conn_info.addr.mac[0];
		g_target[1] = scanned_device->conn_info.addr.mac[1];
		g_target[2] = scanned_device->conn_info.addr.mac[2];
		g_target[3] = scanned_device->conn_info.addr.mac[3];
		g_target[4] = scanned_device->conn_info.addr.mac[4];
		g_target[5] = scanned_device->conn_info.addr.mac[5];
		g_scan_done = 1;
	}
	
	return;
}

static void ble_device_disconnected_cb(ble_conn_handle conn_id)
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	return;
}

static void ble_device_connected_cb(ble_device_connected *dev)
{
	int ret;
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	ret = sem_post(&g_conn_sem);

	g_conn = dev->conn_handle;
	RMC_LOG(RMC_TAG, "Conn Handle : %d\n", g_conn);
	return;
}

static void ble_operation_notification_cb(ble_client_operation_handle *handle, ble_data *read_result)
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	printf("conn : %d // attr : %x // len : %d\n", handle->conn_handle, handle->attr_handle, read_result->length);
	if (read_result->length > 0) {
		printf("read : ");
		int i;
		for (i = 0; i < read_result->length; i++) {
			printf("%02x ", read_result->data[i]);
		}
		printf("\n");
	}
	return;
}

static void ble_server_connected_cb(ble_conn_handle con_handle, ble_server_connection_type_e conn_type, uint8_t mac[BLE_BD_ADDR_MAX_LEN])
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	printf("conn : %d / conn_type : %d\n", con_handle, conn_type);
	printf("conn mac : %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return;
}

static void utc_cb_charact_a_1(ble_server_attr_cb_type_e type, ble_conn_handle linkindex, ble_attr_handle handle, void *arg)
{
	char *arg_str = "None";
	if (arg != NULL) {
		arg_str = (char *)arg;
	}
	printf("-- [RMC CHAR] %s [ type : %d / handle : %d / attr : %d ] --\n", arg_str, type, linkindex, handle);
}

static void utc_cb_desc_b_1(ble_server_attr_cb_type_e type, ble_conn_handle linkindex, ble_attr_handle handle, void *arg)
{
	char *arg_str = "None";
	if (arg != NULL) {
		arg_str = (char *)arg;
	}
	printf("-- [RMC DESC] %s [ type : %d / handle : %d / attr : %d ] --\n", arg_str, type, linkindex, handle);
}

static ble_server_gatt_t gatt_profile[] = {
	{
		.type = BLE_SERVER_GATT_SERVICE,
		.uuid = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01},
		.uuid_length = 16,
		.attr_handle = 0x006a,
	},

	{
		.type = BLE_SERVER_GATT_CHARACT, 
		.uuid = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02}, 
		.uuid_length = 16, .property = BLE_ATTR_PROP_RWN | BLE_ATTR_PROP_WRITE_NO_RSP, 
		.permission = BLE_ATTR_PERM_R_PERMIT | BLE_ATTR_PERM_W_PERMIT, 
		.attr_handle = 0x006b, 
		.cb = utc_cb_charact_a_1, 
		.arg = NULL
	},

	{
		.type = BLE_SERVER_GATT_DESC, 
		.uuid = {0x02, 0x29}, 
		.uuid_length = 2, 
		.permission = BLE_ATTR_PERM_R_PERMIT | BLE_ATTR_PERM_W_PERMIT, 
		.attr_handle = 0x006c, 
		.cb = utc_cb_desc_b_1, 
		.arg = NULL,
	},
};

static uint8_t ble_filter[] = { 0x02, 0x01, 0x05, 0x03, 0x19, 0x80, 0x01, 0x05, 0x03, 0x12, 0x18, 0x0f, 0x18 };

/****************************************************************************
 * ble_rmc_main
 ****************************************************************************/
int ble_rmc_main(int argc, char *argv[])
{
	RMC_LOG(RMC_TAG, "test!!\n");

	ble_result_e ret = BLE_MANAGER_FAIL;

	ble_client_init_config client_config = {
		ble_scan_state_changed_cb,
		ble_device_scanned_cb,
		ble_device_disconnected_cb,
		ble_device_connected_cb,
		ble_operation_notification_cb,
		240};

	ble_server_init_config server_config = {
		ble_server_connected_cb,
		true,
		gatt_profile, sizeof(gatt_profile) / sizeof(ble_server_gatt_t)};

	if (argc < 2) {
		return 0;
	}

	RMC_LOG(RMC_TAG, "cmd : %s\n", argv[1]);

	if (strncmp(argv[1], "init", 5) == 0) {
		if (argc == 3 && strncmp(argv[2], "null", 5) == 0) {
			ret = ble_manager_init(NULL);
			if (ret != BLE_MANAGER_SUCCESS) {
				if (ret != BLE_MANAGER_ALREADY_WORKING) {
					RMC_LOG(RMC_TAG, "init with null fail[%d]\n", ret);
					return 0;
				}
				RMC_LOG(RMC_TAG, "init is already done\n");
			} else {
				RMC_LOG(RMC_TAG, "init with NULL done[%d]\n", ret);
			}
		} else {
			ret = ble_manager_init(&server_config);
			if (ret != BLE_MANAGER_SUCCESS) {
				if (ret != BLE_MANAGER_ALREADY_WORKING) {
					RMC_LOG(RMC_TAG, "init fail[%d]\n", ret);
					return 0;
				}
				RMC_LOG(RMC_TAG, "init is already done\n");
			} else {
				RMC_LOG(RMC_TAG, "init with config done[%d]\n", ret);
			}
		}
	}

	if (strncmp(argv[1], "cbs", 4) == 0) {
		ret = ble_client_set_cb(&client_config);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "set callback fail[%d]\n", ret);
			return 0;
		}
		RMC_LOG(RMC_TAG, "set callback done[%d]\n", ret);
	}

	if (strncmp(argv[1], "deinit", 7) == 0) {
		ret = ble_manager_deinit();
		RMC_LOG(RMC_TAG, "deinit done[%d]\n", ret);
	}

	if (strncmp(argv[1], "bond", 5) == 0) {
		if (argc == 3) {
			if (strncmp(argv[2], "list", 5) == 0) {
				RMC_LOG(RMC_TAG, "== BLE Bonded List ==\n", ret);

				ble_bonded_device_list dev_list[BLE_MAX_BONDED_DEVICE] = { 0, };
				uint16_t dev_count = 0;
				uint8_t *mac;

				ret = ble_manager_get_bonded_device(dev_list, &dev_count);

				RMC_LOG(RMC_TAG, "Bonded Dev : %d\n", dev_count);
				
				for (int i = 0; i < dev_count; i++) {
					mac = dev_list[i].bd_addr;
					RMC_LOG(RMC_TAG, "DEV#%d : %02x:%02x:%02x:%02x:%02x:%02x\n", i + 1, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				}

			} else if (strncmp(argv[2], "clear", 6) == 0) {
				ret = ble_manager_delete_bonded_all();
				if (ret != BLE_MANAGER_SUCCESS) {
					RMC_LOG(RMC_TAG, "fail to delete all of bond dev[%d]\n", ret);
				} else {
					RMC_LOG(RMC_TAG, "success to delete all of bond dev\n");
				}
			}
		}

		if (argc == 4 && strncmp(argv[2], "del", 4) == 0) {
			int cnt = 0;
			uint8_t mac[BLE_BD_ADDR_MAX_LEN] = { 0, };

			char *ptr = strtok(argv[3], ":");
			while (ptr != NULL) {
				mac[cnt++] = strtol(ptr, NULL, 16);
				ptr = strtok(NULL, ":");
			}
			RMC_LOG(RMC_TAG, "TARGET : %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			ret = ble_manager_delete_bonded(mac);
			if (ret == BLE_MANAGER_SUCCESS) {
				RMC_LOG(RMC_TAG, "success to delete bond dev\n");
			} else if (ret == BLE_MANAGER_NOT_FOUND) {
				RMC_LOG(RMC_TAG, "[%02x:%02x:%02x:%02x:%02x:%02x] is not found\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			} else {
				RMC_LOG(RMC_TAG, "fail to delete bond dev[%d]\n", ret);
			}
		}
		RMC_LOG(RMC_TAG, "bond command done.\n");
	}

	if (strncmp(argv[1], "mac", 4) == 0) {
		uint8_t mac[BLE_BD_ADDR_MAX_LEN];
		int i;

		ret = ble_manager_get_mac_addr(mac);

		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "get mac fail[%d]\n", ret);
			return 0;
		}

		RMC_LOG(RMC_TAG, "BLE mac : %02x", mac[0]);
		for (i = 1; i < BLE_BD_ADDR_MAX_LEN; i++) {
			printf(":%02x", mac[i]);
		}
		printf("\n");
	}

	if (strncmp(argv[1], "scan", 5) == 0) {
		if (argc == 3 && argv[2][0] == '1') {
			printf("Start !\n");
			ret = ble_client_start_scan(NULL);

			if (ret != BLE_MANAGER_SUCCESS) {
				RMC_LOG(RMC_TAG, "scan start fail[%d]\n", ret);
				return 0;
			}
		} else if (argc == 3 && argv[2][0] == '2') {
			printf("Start with filter!\n");

			ble_scan_filter filter = { 0, };
			memcpy(&(filter.raw_data), ble_filter, sizeof(ble_filter));
			filter.raw_data_length = sizeof(ble_filter);
			filter.scan_duration = 1500;
			ret = ble_client_start_scan(&filter);

			if (ret != BLE_MANAGER_SUCCESS) {
				RMC_LOG(RMC_TAG, "scan start fail[%d]\n", ret);
				return 0;
			}
		} else {
			printf("stop !\n");
			ret = ble_client_stop_scan();

			if (ret != BLE_MANAGER_SUCCESS) {
				RMC_LOG(RMC_TAG, "scan stop fail[%d]\n", ret);
				return 0;
			}
		}
	}

	if (strncmp(argv[1], "check", 6) == 0) {
		bool chk = false;
		ret = ble_manager_conn_is_active(g_conn, &chk);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "Fail to get status[%d]\n", ret);
		} else {
			if (chk == true) {
				RMC_LOG(RMC_TAG, "Connected!\n");
			} else {
				RMC_LOG(RMC_TAG, "Disonnected!\n");
			}
		}
	}

	if (strncmp(argv[1], "connect", 8) == 0) {
		struct timespec abstime;

		ble_scan_filter filter = { 0, };
		memcpy(&(filter.raw_data), ble_filter, sizeof(ble_filter));
		filter.raw_data_length = sizeof(ble_filter);
		filter.scan_duration = 1000;
		g_scan_done = 0;
		ret = ble_client_start_scan(&filter);

		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "scan start fail[%d]\n", ret);
			return 0;
		}

		sleep(2);

		if (g_scan_done != 1) {
			RMC_LOG(RMC_TAG, "scan fail to get adv packet\n", ret);
			return 0;
		}

		printf("Try to connect! [%02x:%02x:%02x:%02x:%02x:%02x]\n", 
			g_target[0],
			g_target[1],
			g_target[2],
			g_target[3],
			g_target[4],
			g_target[5]
		);

		ble_conn_info conn_info = { 0, };
		memcpy(conn_info.addr.mac, g_target, BLE_BD_ADDR_MAX_LEN);
		conn_info.addr.type = BLE_ADDR_TYPE_PUBLIC;
		conn_info.conn_interval = 8;
		conn_info.slave_latency = 128;
		conn_info.mtu = 240;
		conn_info.is_secured_connect = true;

		sem_init(&g_conn_sem, 0, 0);

		ret = ble_client_connect(&conn_info);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "connect fail[%d]\n", ret);
		}

		ret = clock_gettime(CLOCK_REALTIME, &abstime);

		abstime.tv_sec = abstime.tv_sec + 10;
		abstime.tv_nsec = 0;

		int status = sem_timedwait(&g_conn_sem, &abstime);
		sem_destroy(&g_conn_sem);
		int errcode = errno;
		if (status < 0) {
			RMC_LOG(RMC_TAG, "ERROR: sem_timedwait failed with: %d\n", errcode);
			return 0;
		} else {
			RMC_LOG(RMC_TAG, "PASS: sem_timedwait succeeded\n");
		}
		
		ble_client_operation_handle handle[2] = { 0, };
		
		handle[0].conn_handle = g_conn;
		handle[0].attr_handle = 0xff03;

		handle[1].conn_handle = g_conn;
		handle[1].attr_handle = 0x006e;

		ret = ble_client_operation_enable_notification(&handle[0]);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "Fail to enable noti handle1[%d]\n", ret);
		} else {
			RMC_LOG(RMC_TAG, "Success to enable noti handle1.\n");
		}
		ret = ble_client_operation_enable_notification(&handle[1]);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "Fail to enable noti handle2[%d]\n", ret);
		} else {
			RMC_LOG(RMC_TAG, "Success to enable noti handle2.\n");
		}
	}

	RMC_LOG(RMC_TAG, "done\n");
	return 0;
}
