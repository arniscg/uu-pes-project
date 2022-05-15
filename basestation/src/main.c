#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include "bluetooth.h"

static void handle_bt_sensor_value(uint16_t data) {
	printk("Received ambient light value %d lux\n", data);
}

static void handle_bt_button(uint16_t data) {
	printk("Received button value %d\n", data);
}

void main(void) {

	int bt_okay = connect_bluetooth();
	if (!bt_okay) {
		printk("Failed to connect to bluetooth");
		return;
	}
  
	// Send adjustment request every 5s
	while(true) {

	
		
		//printf("This is basestation");
		//request_adjustment(1);
		k_sleep(K_SECONDS(2));
		//k_msleep(5000);
		
		

	}
}
