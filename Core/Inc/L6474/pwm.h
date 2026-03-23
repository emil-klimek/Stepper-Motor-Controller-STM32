#ifndef PWM_H
#define PWM_H

#include "main.h"


#define CPU_FREQUENCY 8000000
#define COUNTER_PERIOD 100

extern TIM_HandleTypeDef htim3;


struct L6474;
void L6474_PwmInit(struct L6474* l6474);
int prescaler(float value);
void pwm_write(float value);
void pwm_period(float seconds);


/*
class Pwm
{
public:
	Pwm();

	//counter period = 1000

	void write(float value);
	void period(float seconds);

private:
	TIM_OC_InitTypeDef sConfigOC;
};
*/

#endif
