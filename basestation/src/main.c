#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include "bluetooth.h"
#include "pwm-led.h"

#define MY_STACK_SIZE 				1000
#define BUTTON_DEBOUNCE_DELAY_MS 	100
#define THRESHOLD					10 // Right now it's very low because of low ligt LED

#if 0
/* Semaphore used to signal from button ISR to task */
K_SEM_DEFINE(button_sem, 0, 1);

//struct k_mutex state_mutex;
K_MUTEX_DEFINE(state_mutex);

#define SW0_NODE	DT_ALIAS(sw0)
#define SW0			DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_PIN		DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))

/* Variables used for button debouncing */
static volatile uint32_t time, last_time;

typedef enum _state
{
	basestation_off,
	basestation_on
} state_t;

static volatile state_t basestation_state = basestation_on;

#endif

typedef enum
{
	off				= 0,
	normal			= 150,
	simple_study 	= 250,
	intensive_study = 500
} brightness;

static volatile uint16_t reference_val = normal;
static volatile uint8_t adjustment_mode = 0;
K_MUTEX_DEFINE(mode_mutex);

/* Compare measured value to reference value and increase/decrease brightness*/
void compare_to_reference(uint16_t measured_val)
{
	uint8_t new_adjustment_mode;

	printk("Reference: %d, measured: %d\n", reference_val, measured_val);
	if(measured_val > reference_val + THRESHOLD)
	{
		new_adjustment_mode = 1;
		decrease_brightness_by(5);
	} else if (measured_val < reference_val - THRESHOLD)
	{
		new_adjustment_mode = 1;
		increase_brightness_by(5);
	} else
	{
		new_adjustment_mode = 0;
	}

	// Send adjustment request only when it changes
	if (new_adjustment_mode != adjustment_mode) {
		adjustment_mode = new_adjustment_mode;
		if (adjustment_mode)
			request_adjustment(1);
		else
			request_adjustment(0);
	}
}

/* Change the mode, i.e. reference value, according to the received Bluetooth message */
void change_mode(uint16_t data)
{
	switch(data)
	{
		k_mutex_lock(&mode_mutex, K_FOREVER);

		case 0: 
			reference_val = off;
		case 1:
			reference_val = normal;
		case 2:
			reference_val = simple_study;
		case 4:
			reference_val = intensive_study;
		default:
			reference_val = reference_val;

		k_mutex_unlock(&mode_mutex);
	}
}

static void handle_bt_sensor_value(uint16_t data) 
{
	// printk("Received ambient light value %d lux\n", data);
	compare_to_reference(data);
}

static void handle_bt_button(uint16_t data) 
{
	printk("Received button value %d\n", data);
	change_mode(data);
}

void basestation_task(void) 
{
	pwm_led_init();

	int bt_okay = connect_bluetooth();
	if (!bt_okay) 
	{
		printk("Failed to connect to bluetooth");
		return;
	}

	//TODO: disconnect communication if basestation_state == basestation_off
	//reconnect if basestation_state == basestation_on

}

#if 0

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	/* Debounce the button */
	time = k_uptime_get_32();
	if (time < last_time + BUTTON_DEBOUNCE_DELAY_MS) 
	{
		last_time = time;
		return;
	}
	k_sem_give(&button_sem);
	time = last_time;
}

/* Structure to add callback to button pin */
/* Should not be on stack !!! */
static struct gpio_callback button_cb_data;

/* If button #0 was pressed, the basestation should be turned on/off */
void button_task()
{
	/* Set up button interrupt */
	const struct device *sw0;
	sw0 = device_get_binding(SW0);
	gpio_pin_configure(sw0, SW0_PIN, GPIO_INPUT | SW0_FLAGS);
	gpio_pin_interrupt_configure(sw0, SW0_PIN, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_PIN));
	gpio_add_callback(sw0, &button_cb_data);

	while(1)
	{
		k_sem_take(&button_sem, K_FOREVER);

		k_mutex_lock(&state_mutex, K_FOREVER);
		if(basestation_state == basestation_off)
		{
			basestation_state == basestation_on;
			printk("The basestation has been turned on\n");
		} else
		{
			basestation_state == basestation_off;
			printk("The basestation has been turned off\n");
		}
		k_mutex_unlock(&state_mutex);
	}
}

K_THREAD_DEFINE(button_id, MY_STACK_SIZE, button_task, 
				NULL, NULL, NULL, 5, 0, 0);

#endif

K_THREAD_DEFINE(work_id, MY_STACK_SIZE, basestation_task,
				NULL, NULL, NULL, 5, 0, 0);
