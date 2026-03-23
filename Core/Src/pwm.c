

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <pwm.h>

#define TIMER_FREQUENCY                   8000000
#define COUNTER_PERIOD                    10000
#define AUTO_RELOAD 					  100

int timer_freq(float seconds)
{
	return 1 / seconds;
}

int prescaler(float period)
{
	if(period == 0.0f)
	{
		return 1;
	}
	else
	{
		return TIMER_FREQUENCY / (timer_freq ( period ) * ( AUTO_RELOAD + 1 )) - 1;
	}
}

int compare_val(float value)
{
	return value * AUTO_RELOAD;
}

void L6474_PwmInit(struct L6474* l6474)
{
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_Base_Start_IT(&htim3);


	TIM3->PSC  =                prescaler (0.0);
	TIM3->CCR2 =  			  compare_val (1.0);
	TIM3->ARR  =        	  AUTO_RELOAD;
}

void pwm_write(float value)
{
	TIM3->CCR2 = compare_val(value);
}

void pwm_period(float seconds)
{
	TIM3->PSC = prescaler(seconds);
}


