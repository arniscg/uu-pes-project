#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include "bluetooth.h"
#include "pwm-led.h"
#include <drivers/gpio.h>

#define MY_STACK_SIZE 				1000
#define BUTTON_DEBOUNCE_DELAY_MS 	100
#define THRESHOLD					20
#define ADJUSTMENT_STEP				2

/* Semaphore used to signal from button ISR to task */
K_SEM_DEFINE(button_sem, 0, 1);

//struct k_mutex state_mutex;
// Not sure we need a lock when there is only one writer and one reader
// It's only used by writer, so it doesn't really lock anyone out. So currently it does nothing.
K_MUTEX_DEFINE(state_mutex);

#define SW0_NODE	DT_ALIAS(sw0)
#define SW0			DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_PIN		DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))

/* Variables used for button debouncing */
static volatile uint32_t time, last_time;

typedef enum _state
{
	auto_adjust_off,
	auto_adjust_on
} state_t;

static volatile state_t basestation_state = auto_adjust_on;

typedef enum
{
	off				= 0,
	normal			= 150,
	simple_study 	= 250,
	intensive_study = 400
} brightness;

static volatile uint16_t reference_val = normal;
static volatile uint8_t adjustment_mode = 0;
K_MUTEX_DEFINE(mode_mutex);

uint16_t abs(uint16_t val) {
	if (val < 0) {
		return val * -1;
	}

	return val;
}

/* Compare measured value to reference value and increase/decrease brightness*/
void compare_to_reference(uint16_t measured_val)
{
	uint8_t new_adjustment_mode;

	printk("Reference: %d, measured: %d\n", reference_val, measured_val);
	if (abs(measured_val - reference_val) > THRESHOLD)
	{
		new_adjustment_mode = 1;
		if (measured_val > reference_val)
			decrease_brightness_by(ADJUSTMENT_STEP);
		else 
			increase_brightness_by(ADJUSTMENT_STEP);
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
			break;
		case 1:
			reference_val = normal;
			break;
		case 2:
			reference_val = simple_study;
			break;
		case 3:
			reference_val = intensive_study;
			break;
		default:
			reference_val = reference_val;
			break;

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
}

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
		if(basestation_state == auto_adjust_off)
		{
			basestation_state = auto_adjust_on;
			printk("The auto adjustment has been turned on\n");
			connect_bluetooth();
		} else
		{
			basestation_state = auto_adjust_off;
			printk("The auto adjustment has been turned off\n");
			bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_POWER_OFF);
		}
		k_mutex_unlock(&state_mutex);
	}
}

K_THREAD_DEFINE(button_id, MY_STACK_SIZE, button_task, 
				NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(work_id, MY_STACK_SIZE, basestation_task,
				NULL, NULL, NULL, 5, 0, 0);
