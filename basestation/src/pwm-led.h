#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

#define PWM_LED0_NODE	DT_ALIAS(pwmled)

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#define PWM_CTLR	DT_PWMS_CTLR(PWM_LED0_NODE)
#define PWM_CHANNEL	DT_PWMS_CHANNEL(PWM_LED0_NODE)
#define PWM_FLAGS	DT_PWMS_FLAGS(PWM_LED0_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_CTLR	DT_INVALID_NODE
#define PWM_CHANNEL	0
#define PWM_FLAGS	0
#endif

#define MIN_PERIOD_USEC	(USEC_PER_SEC / 128U)
#define MAX_PERIOD_USEC	USEC_PER_SEC

static const struct device *pwm;
static uint32_t max_period;
static uint32_t period;
static uint8_t duty_cycle;

static void pwm_led_init(void);
static void increase_brightness_by(uint8_t duty);
static void decrease_brightness_by(uint8_t duty);
static void set_brightness(uint8_t duty_cyle);

static void pwm_led_init()
{
    pwm = DEVICE_DT_GET(PWM_CTLR);
	if (!device_is_ready(pwm)) 
    {
		printk("Error: PWM device %s is not ready\n", pwm->name);
		return;
	}

    //Calibrate
    printk("Calibrating for channel %d...\n", PWM_CHANNEL);
	max_period = MAX_PERIOD_USEC;
	while (pwm_pin_set_usec(pwm, PWM_CHANNEL,
				max_period, max_period / 2U, PWM_FLAGS)) 
    {
		max_period /= 2U;
		if (max_period < (4U * MIN_PERIOD_USEC)) 
        {
			printk("Error: PWM device "
			       "does not support a period at least %u\n",
			       4U * MIN_PERIOD_USEC);
			return;
		}
	}

	printk("Done calibrating; maximum/minimum periods %u/%u usec\n",
	       max_period, MIN_PERIOD_USEC);

	period = MIN_PERIOD_USEC;

	duty_cycle = 50;
	
	set_brightness(duty_cycle);
}

static void increase_brightness_by(uint8_t duty)
{
    
    if(duty < duty_cycle)
    {
        duty_cycle -= duty;
    } else
    {
        duty_cycle = 1;
    }

    int ret = pwm_pin_set_usec(pwm, PWM_CHANNEL,
				       period, (uint32_t ) period*duty_cycle/100, PWM_FLAGS);
		if (ret) 
        {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}
}

static void decrease_brightness_by(uint8_t duty)
{
    
    if(duty_cycle + duty < 100)
    {
        duty_cycle += duty;
    } else
    {
        duty_cycle = 99;
    }

    int ret = pwm_pin_set_usec(pwm, PWM_CHANNEL,
				       period, (uint32_t ) period*duty_cycle/100, PWM_FLAGS);
		if (ret) 
        {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}
}

static void set_brightness(uint8_t duty_cycle)
{
    
    if(duty_cycle > 100)
    {
        printk("Failed to set duty cycle\n");
    } else
    {
        duty_cycle = 100-duty_cycle;
    }

    int ret = pwm_pin_set_usec(pwm, PWM_CHANNEL,
				       period, (uint32_t ) period*duty_cycle/100, PWM_FLAGS);
		if (ret) 
        {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}
}