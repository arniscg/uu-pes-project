#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "bluetooth_service.h"

static uint8_t connected_to_basestation = 0;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		BT_CONTROLLER_SERVICE_UUID_BYTES
	)
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}

	connected_to_basestation = 1;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	connected_to_basestation = 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
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

	bt_ready();

	bt_conn_auth_cb_register(&auth_cb_display);

    return 1;
}

/**
 * Function that sends ambient sensor value to basestation
 */
static void send_light_sensor_value(uint16_t value) {
	printk("Sending ambient light value %d lux to basestation\n", value);
	bt_gatt_notify(NULL, &controller_service.attrs[1], &value, sizeof(value));
}

/**
 * Function that sends button value to basestation
 */
static void send_button_value(uint16_t value) {
	printk("Sending button value %d to basestation\n", value);
	bt_gatt_notify(NULL, &controller_service.attrs[3], &value, sizeof(value));
}