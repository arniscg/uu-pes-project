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

void main(void)
{
	// Initialize bluetooth connection
	int bt_okay = connect_bluetooth();
	if (!bt_okay) {
		printf("Failed to connect to bluetooth");
		return;
	}

	// Initialize the light sensor
	const struct device *veml7700 = device_get_binding("VEML7700");
	if (veml7700 == NULL) {
		printf("No device \"%s\" found; did initialization fail?\n", "VEML7700");
		return;
	}

	struct sensor_value lux;

	// Main loop
	while (1) {
		k_sleep(K_SECONDS(2));

		// Get sensor reading
		sensor_sample_fetch(veml7700);
		sensor_channel_get(veml7700, SENSOR_CHAN_LIGHT, &lux);

		// Send to basestation
		send_light_sensor_value(lux.val1);
		send_button_value(3);
	}
}
