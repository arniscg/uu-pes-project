#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include "bluetooth.h"

static volatile uint8_t adjustment_mode = 0;

static void handle_adjustment_request(uint8_t data) {
	if (data == 0) {
		printk("Exiting adjustment mode\n");
		adjustment_mode = 0;
	} else if (data == 1) {
		printk("Entering adjustment mode\n");
		adjustment_mode = 1;
	} else {
		printk("Warning: Got unexpected adjutment request valule '%d'\n", data);
	}
}

void main(void)
{
	// Initialize bluetooth connection
	int bt_okay = connect_bluetooth();
	if (!bt_okay) {
		printk("Failed to connect to bluetooth");
		return;
	}

	// Initialize the light sensor
	const struct device *veml7700 = device_get_binding("VEML7700");
	if (veml7700 == NULL) {
		printk("No device \"%s\" found; did initialization fail?\n", "VEML7700");
		return;
	}

	struct sensor_value lux;

	// Main loop
	while (1) {
		if (adjustment_mode)
			k_msleep(200);
		else
			k_msleep(2000);

		// Get sensor reading
		sensor_sample_fetch(veml7700);
		sensor_channel_get(veml7700, SENSOR_CHAN_LIGHT, &lux);

		// Send to basestation
		send_light_sensor_value(lux.val1);
		// send_button_value(3);
	}
}
