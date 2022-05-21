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
#include <devicetree.h>
#include <drivers/gpio.h>

#define BUTTON_DEBOUNCE_DELAY_MS 	100
#define MY_STACK_SIZE 				1000
#define WAIT_LONG 					2000
#define WAIT_SHORT					200

#define SW0_NODE	DT_ALIAS(sw0)
#define SW0			DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_PIN		DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))

#define SW1_NODE	DT_ALIAS(sw1)
#define SW1			DT_GPIO_LABEL(SW1_NODE, gpios)
#define SW1_PIN		DT_GPIO_PIN(SW1_NODE, gpios)
#define SW1_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW1_NODE, gpios))

#define SW2_NODE	DT_ALIAS(sw2)
#define SW2			DT_GPIO_LABEL(SW2_NODE, gpios)
#define SW2_PIN		DT_GPIO_PIN(SW2_NODE, gpios)
#define SW2_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW2_NODE, gpios))

#define SW3_NODE	DT_ALIAS(sw3)
#define SW3			DT_GPIO_LABEL(SW2_NODE, gpios)
#define SW3_PIN		DT_GPIO_PIN(SW3_NODE, gpios)
#define SW3_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW3_NODE, gpios))

static uint32_t time, last_time;
static struct gpio_callback button_cb_data;
static struct gpio_callback button_cb_data1;
static struct gpio_callback button_cb_data2;
static struct gpio_callback button_cb_data3;

static volatile uint8_t adjustment_mode = 0;

static void handle_adjustment_request(uint8_t data)
{
	if (data == 0)
	{
		printk("Exiting adjustment mode\n");
		adjustment_mode = 0;
	} else if (data == 1)
	{
		printk("Entering adjustment mode\n");
		adjustment_mode = 1;
	} else
	{
		printk("Warning: Got unexpected adjutment request valule '%d'\n", data);
	}
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	time = k_uptime_get_32();
	 //printf("Button pressed\n");
	if (time < last_time + BUTTON_DEBOUNCE_DELAY_MS) 
	{
		last_time = time;
		return;
	}

	time = last_time;

	if(pins == BIT(SW0_PIN))
	{
        send_button_value(0);       
	}
	else if(pins == BIT(SW1_PIN))
	{
		send_button_value(1);  
	}
	else if(pins == BIT(SW2_PIN))
	{
       send_button_value(2); 
	}
	else if(pins == BIT(SW3_PIN))
	{
		send_button_value(3); 
	}
	else
	{
		printf("not valid button pressed");
	}
}

void button_task()
{
     /* Set up button interrupt */
	const struct device *sw0;
	sw0 = device_get_binding(SW0);
	gpio_pin_configure(sw0, SW0_PIN, GPIO_INPUT | SW0_FLAGS);
	gpio_pin_interrupt_configure(sw0, SW0_PIN, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_PIN));
	gpio_add_callback(sw0, &button_cb_data);

	const struct device *sw1;
	sw1 = device_get_binding(SW1);
	gpio_pin_configure(sw1, SW1_PIN, GPIO_INPUT | SW1_FLAGS);
	gpio_pin_interrupt_configure(sw1, SW1_PIN, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data1, button_pressed, BIT(SW1_PIN));
	gpio_add_callback(sw1, &button_cb_data1);

	const struct device *sw2;
	sw2 = device_get_binding(SW2);
	gpio_pin_configure(sw2, SW2_PIN, GPIO_INPUT | SW2_FLAGS);
	gpio_pin_interrupt_configure(sw2, SW2_PIN, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data2, button_pressed, BIT(SW2_PIN));
	gpio_add_callback(sw2, &button_cb_data2);

	const struct device *sw3;
	sw3 = device_get_binding(SW3);
	gpio_pin_configure(sw3, SW3_PIN, GPIO_INPUT | SW3_FLAGS);
	gpio_pin_interrupt_configure(sw3, SW3_PIN, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data3, button_pressed, BIT(SW3_PIN));
	gpio_add_callback(sw3, &button_cb_data3);

	while(1)
	{
		k_sleep(K_SECONDS(3));
	}  
}

void main()
{
    int bt_okay = connect_bluetooth();
	if (!bt_okay) {
		printf("Failed to connect to bluetooth");
		return;
	}

    const struct device *veml7700 = device_get_binding("VEML7700");
	if (veml7700 == NULL) {
		printf("No device \"%s\" found; did initialization fail?\n", "VEML7700");
		return;
	}

    struct sensor_value lux;

	while(1)
	{
		if (connected_to_basestation)
		{
			if (adjustment_mode)
				k_msleep(WAIT_SHORT);
			else
				k_msleep(WAIT_LONG);

			// Get sensor reading
			sensor_sample_fetch(veml7700);
			sensor_channel_get(veml7700, SENSOR_CHAN_LIGHT, &lux);

			// Send to basestation
			send_light_sensor_value(lux.val1);
		} else 
		{
			k_msleep(WAIT_LONG);
		}
	}      
}

K_THREAD_DEFINE(button, MY_STACK_SIZE, button_task, NULL, NULL, NULL, 0, 0, 0);