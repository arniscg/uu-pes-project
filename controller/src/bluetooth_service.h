#include <bluetooth/gatt.h>

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	// printk("Value: %d. Not sure what to do here\n", value);
}

// https://www.guidgenerator.com/
// a34536f1-cc82-4c80-a494-d0c51b5fb11c
#define BT_CONTROLLER_SERVICE_UUID_BYTES        0xa3, 0x45, 0x36, 0xf1, 0xcc, 0x82, 0x4c, 0x80, 0xa4, 0x94, 0xd0, 0xc5, 0x1b, 0x5f, 0xb1, 0x1c
#define BT_LIGHT_CHARACTERISTIC_UUID_BYTES      0xa3, 0x45, 0x36, 0xf2, 0xcc, 0x82, 0x4c, 0x80, 0xa4, 0x94, 0xd0, 0xc5, 0x1b, 0x5f, 0xb1, 0x1c
#define BT_BUTTON_CHARACTERISTIC_UUID_BYTES     0xa3, 0x45, 0x36, 0xf3, 0xcc, 0x82, 0x4c, 0x80, 0xa4, 0x94, 0xd0, 0xc5, 0x1b, 0x5f, 0xb1, 0x1c
#define BT_ADJUSTMENT_CHARACTERISTIC_UUID_BYTES 0xa3, 0x45, 0x36, 0xf4, 0xcc, 0x82, 0x4c, 0x80, 0xa4, 0x94, 0xd0, 0xc5, 0x1b, 0x5f, 0xb1, 0x1c

#define BT_CONTROLLER_SERVICE_UUID BT_UUID_DECLARE_128(BT_CONTROLLER_SERVICE_UUID_BYTES)
#define BT_LIGHT_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(BT_LIGHT_CHARACTERISTIC_UUID_BYTES)
#define BT_BUTTON_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(BT_BUTTON_CHARACTERISTIC_UUID_BYTES)
#define BT_ADJUSTMENT_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(BT_ADJUSTMENT_CHARACTERISTIC_UUID_BYTES)

static void handle_adjustment_request(uint8_t data);

static ssize_t write_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags) {

    uint8_t data = ((uint8_t *)buf)[0];

    handle_adjustment_request(data);

    return len;
}

BT_GATT_SERVICE_DEFINE(controller_service,
    BT_GATT_PRIMARY_SERVICE(BT_CONTROLLER_SERVICE_UUID),
    BT_GATT_CHARACTERISTIC(BT_LIGHT_CHARACTERISTIC_UUID, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_BUTTON_CHARACTERISTIC_UUID, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_ADJUSTMENT_CHARACTERISTIC_UUID, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_cb, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);