#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>

#define BT_CONTROLLER_SERVICE_UUID BT_UUID_DECLARE_128(0xa3, 0x45, 0x36, 0xf1, 0xcc, 0x82, 0x4c, 0x80, 0xa4, 0x94, 0xd0, 0xc5, 0x1b, 0x5f, 0xb1, 0x1c)
#define BT_LIGHT_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(0xa3, 0x45, 0x36, 0xf2, 0xcc, 0x82, 0x4c, 0x80, 0xa4, 0x94, 0xd0, 0xc5, 0x1b, 0x5f, 0xb1, 0x1c)
#define BT_BUTTON_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(0xa3, 0x45, 0x36, 0xf3, 0xcc, 0x82, 0x4c, 0x80, 0xa4, 0x94, 0xd0, 0xc5, 0x1b, 0x5f, 0xb1, 0x1c)

static void start_scan(void);
static void handle_bt_sensor_value(uint16_t data);
static void handle_bt_button(uint16_t data);

static struct bt_conn *default_conn;

static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params_light;
static struct bt_gatt_subscribe_params subscribe_params_buttons;

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, uint16_t length) {
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	uint16_t value = ((uint16_t *)data)[0];
	if (params->value_handle == subscribe_params_light.value_handle) {
		handle_bt_sensor_value(value);
	} else if (params->value_handle == subscribe_params_buttons.value_handle) {
		handle_bt_button(value);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params) {
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("Discovered attribute: %d (perm: %d)\n", bt_gatt_attr_value_handle(attr), attr->perm);
	if (bt_uuid_cmp(discover_params.uuid, BT_CONTROLLER_SERVICE_UUID) == 0) {
		printk("Discovering light value characteristic\n");
		memcpy(&uuid, BT_LIGHT_CHARACTERISTIC_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_LIGHT_CHARACTERISTIC_UUID) == 0) {
		printk("Discovering button characteristic\n");
		memcpy(&uuid, BT_BUTTON_CHARACTERISTIC_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		subscribe_params_light.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_BUTTON_CHARACTERISTIC_UUID) == 0) {
		printk("Discovering CCC descriptor\n");
	 	memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params_buttons.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else {
		subscribe_params_light.notify = notify_func;
		subscribe_params_light.value = BT_GATT_CCC_NOTIFY;
		subscribe_params_light.ccc_handle = attr->handle;
		subscribe_params_buttons.notify = notify_func;
		subscribe_params_buttons.value = BT_GATT_CCC_NOTIFY;
		subscribe_params_buttons.ccc_handle = attr->handle;

		printk("Subscribing to light...\n");
		err = bt_gatt_subscribe(conn, &subscribe_params_light);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}

		printk("Subscribing to desired light...\n");
		err = bt_gatt_subscribe(conn, &subscribe_params_buttons);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}
	}

	return BT_GATT_ITER_STOP;
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int err;
	struct bt_le_conn_param *param;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_UUID128_SOME:
	case BT_DATA_UUID128_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			printk("AD malformed\n");
			return true;
		}

		if (data->data_len == BT_UUID_SIZE_128) {
			if (bt_uuid_cmp(BT_UUID_DECLARE_128( // This is ugly
				data->data[0], data->data[1], data->data[2], data->data[3],
				data->data[4], data->data[5], data->data[6], data->data[7],
				data->data[8], data->data[9], data->data[10], data->data[11],
				data->data[12], data->data[13], data->data[14], data->data[15]
			), BT_CONTROLLER_SERVICE_UUID) == 0) {
				printk("Found controller service!\n");
				err = bt_le_scan_stop();
				if (err) {
					printk("Stop LE scan failed (err %d)\n", err);
					return true;
				}

				param = BT_LE_CONN_PARAM_DEFAULT;
				err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
							param, &default_conn);
				if (err) {
					printk("Create conn failed (err %d)\n", err);
					start_scan();
				}
			} else {
				printk("No match\n");
				return true;
			}

			return false;
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void start_scan(void)
{
	int err;

	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime. */
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn == default_conn) {
		// memcpy(&uuid, BT_UUID_HRS, sizeof(uuid));
		memcpy(&uuid, BT_CONTROLLER_SERVICE_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		printk("Discovering primary service\n");
		err = bt_gatt_discover(default_conn, &discover_params);
		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/**
 * Function that initialized everything related with bluetooth
 */
static int connect_bluetooth() {
	int err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
        return 0;
	}

	printk("Bluetooth initialized\n");

	start_scan();

    return 1;
}