
#include <controller.h>
#include <Common/Component.h>
#include <L6474_config.h>
#include <L6474_def.h>
#include <motor_def.h>
#include <L6474.h>

#include <stdlib.h>
#include <stdio.h>
#include <EtherShield.h>
#include <string.h>
#include <ip_udp.h>
#include <ENC28J60/EtherShield.h>
#include <L6474.h>

#define true 1
#define false 0

uint8_t local_ip[4] = { 10,0,0,2 };

typedef struct controller
{
	uint16_t dat_p;
	uint8_t dest_ip[4];
	uint8_t local_ip[4];
	const char* msg;
	char cmd_buf[200];
	uint16_t dport;
	L6474 *motor;
	int position;
	int steps;
	int direction;
	int delay;
	uint8_t local_mac[4];
	unsigned int cur_speed;
	enum state state;
	enum state prev_state;
	uint32_t t1;
	int move_finished;
	int microstep;
} controller;

extern SPI_HandleTypeDef hspi1;

#define BUFFER_SIZE 500
static uint8_t buf[BUFFER_SIZE+1] = {0};

#define PSTR(s) s


int ES_readMacAddr(char *mac);


const char *eat_space(const char*str)
{
        while(str[0] == ' ' || str[0] == '\t')
        {
                ++str;
        }
        return str;
}

C_API int update_timer(struct controller *ptr)
{
	//int t = HAL_GetTick();
	//int dt = t - ptr->t1;
	//ptr->t1 = t;
	//return dt;
	return 0;
}

C_API int get_milliseconds(struct controller *ptr)
{
	int t = HAL_GetTick();
	int dt = t - ptr->t1;
	ptr->t1 = t;
	return dt;
}

#define STEPS_1 (400 * 8)   /* 1 revolution given a 400 steps motor configured at 1/8 microstep mode. */

/* Delay in milliseconds. */
#define DELAY_1 1000
#define DELAY_2 2000
#define DELAY_3 6000
#define DELAY_4 8000

/* Speed in pps (Pulses Per Second).
   In Full Step mode: 1 pps = 1 step/s).
   In 1/N Step Mode:  N pps = 1 step/s). */
#define SPEED_1 2400
#define SPEED_2 1200



L6474_init_t init = {
    160,                              /* Acceleration rate in pps^2. Range: (0..+inf). */
    160,                              /* Deceleration rate in pps^2. Range: (0..+inf). */
    1600,                             /* Maximum speed in pps. Range: (30..10000]. */
    800,                              /* Minimum speed in pps. Range: [30..10000). */
    250,                              /* Torque regulation current in mA. Range: 31.25mA to 4000mA. */
    L6474_OCD_TH_750mA,               /* Overcurrent threshold (OCD_TH register). */
    L6474_CONFIG_OC_SD_ENABLE,        /* Overcurrent shutwdown (OC_SD field of CONFIG register). */
    L6474_CONFIG_EN_TQREG_TVAL_USED,  /* Torque regulation method (EN_TQREG field of CONFIG register). */
    L6474_STEP_SEL_1_8,               /* Step selection (STEP_SEL field of STEP_MODE register). */
    L6474_SYNC_SEL_1_2,               /* Sync selection (SYNC_SEL field of STEP_MODE register). */
    L6474_FAST_STEP_12us,             /* Fall time value (T_FAST field of T_FAST register). Range: 2us to 32us. */
    L6474_TOFF_FAST_8us,              /* Maximum fast decay time (T_OFF field of T_FAST register). Range: 2us to 32us. */
    3,                                /* Minimum ON time in us (TON_MIN register). Range: 0.5us to 64us. */
    21,                               /* Minimum OFF time in us (TOFF_MIN register). Range: 0.5us to 64us. */
    L6474_CONFIG_TOFF_044us,          /* Target Swicthing Period (field TOFF of CONFIG register). */
    L6474_CONFIG_SR_320V_us,          /* Slew rate (POW_SR field of CONFIG register). */
    L6474_CONFIG_INT_16MHZ,           /* Clock setting (OSC_CLK_SEL field of CONFIG register). */
    L6474_ALARM_EN_OVERCURRENT |
    L6474_ALARM_EN_THERMAL_SHUTDOWN |
    L6474_ALARM_EN_THERMAL_WARNING |
    L6474_ALARM_EN_UNDERVOLTAGE |
    L6474_ALARM_EN_SW_TURN_ON |
    L6474_ALARM_EN_WRONG_NPERF_CMD    /* Alarm (ALARM_EN register). */
};

//L6474 *motor;
L6474 *motor;

void flag_irq_handler(void)
{
    /* Set ISR flag. */
    //motor->isr_flag = TRUE; //changed
	L6474_SetIsrFlag();

    /* Get the value of the status register. */
   // L6474_GetS
	//unsigned int status = motor->get_status();
	unsigned int status = L6474_CmdGetStatus(motor);

    /* Check NOTPERF_CMD flag: if set, the command received by SPI can't be performed. */
    /* This often occures when a command is sent to the L6474 while it is not in HiZ state. */
    if ((status & L6474_STATUS_NOTPERF_CMD) == L6474_STATUS_NOTPERF_CMD) {
        printf("    WARNING: \"FLAG\" interrupt triggered. Non-performable command detected when updating L6474's registers while not in HiZ state.\r\n");
    }

    /* Reset ISR flag. */
    //motor->isr_flag = FALSE; //changed
    L6474_ResetIsrFlag();
}



C_API int L6474_init(struct controller *ptr)
{

	//HAL_GetTick()
	//L6474 Create


	//ptr->motor = new L6474;
	ptr->motor = createL6474();
	motor = ptr->motor;


	if(L6474_Init(ptr->motor, &init) != COMPONENT_OK) //if(ptr->motor->init(&init) != COMPONENT_OK)
	{
		printf("failed to initialize\r\n");
	}
	else
	{
		printf("INIT L6474: OK\r\n");
	}


	fputs("init_L6474()\r\n",stdout);

    L6474_SetMaxSpeed(ptr->motor, 10000);
    L6474_SetParameter(ptr->motor, L6474_TVAL, 500);
    ptr->position = L6474_GetPosition(ptr->motor);

    ptr->position = 0;
    printf("ptr->position: %i\r\n",ptr->position);


    L6474_SetMinSpeed(ptr->motor, 3000);
    ptr->cur_speed = 3000;

    ptr->microstep = 2;

	return 0;
}

C_API int ENC28J60_init(struct controller*ptr)
{
	HAL_Delay(1);

	fputs("Setting SPI: ",stdout);
	ES_enc28j60SpiInit(&hspi1);
	fputs("OK\r\n",stdout);


	uint8_t test_macaddr[6] = {};

	fputs("ENC28J60 Init: ",stdout);
	ES_enc28j60Init(ptr->local_mac);
	fputs("OK\r\n",stdout);


	printf("local ip: %i.%i.%i.%i\r\n", local_ip[0], local_ip[1], local_ip[2], local_ip[3] );

	fputs("MAC addr: ", stdout);
	ES_readMacAddr((char*)test_macaddr);
	for(int i =0; i<5; ++i)
	{
		printf("0x%x:", test_macaddr[i]);
	}
	printf("0x%x", test_macaddr[5]);
	fputs("\r\n",stdout);

	uint8_t enc28j60_rev = ES_enc28j60Revision();
	printf("Revision: ");
	if (enc28j60_rev <= 0)
	{
		printf("error\r\n");
		exit(EXIT_FAILURE);
	}

	printf("%i\r\n", enc28j60_rev);


	printf("ip: %i.%i.%i.%i\r\n", ptr->local_ip[0], ptr->local_ip[1], ptr->local_ip[2], ptr->local_ip[3]);

	printf("INIT arp udp tcp: ");
	ES_init_ip_arp_udp_tcp(ptr->local_mac, ptr->local_ip, 80);
	printf("OK\r\n");


	fputs("============================================\r\n", stdout);

	//while(1)
	//{
	//	client_icmp_request(buf,dest_ip);
	//	printf("ping respone: %s\r\n", buf);
	//}

	return 0;
}

void driver_move(struct controller* drv)
{
	drv->steps -= STEPS_1/drv->microstep;
	L6474_Move(drv->motor, (direction_t)drv->direction, STEPS_1/drv->microstep);
	drv->position = drv->position + (drv->direction * 2 - 1)*(STEPS_1/drv->microstep);
}

int init_driver(struct controller* ptr)
{
	return 0;
}



controller *create_controller()
{

	controller *ptr = (controller*)malloc(sizeof(controller));
	memset(ptr, 0, sizeof(controller));

	ptr->dat_p = 0;
	memcpy(ptr->dest_ip, "\x0a\x00\x00\x01", sizeof(uint8_t[4]));  //10.0.0.1
	memcpy(ptr->local_ip, local_ip, sizeof(uint8_t[4]));
	memcpy(ptr->local_mac, "\x53\x54\x4d\x20\x33\x32", sizeof(uint8_t[6]));

	ptr->dport = 49152;
	ptr->direction = FWD;
	ptr->state = STOP;
	ptr->prev_state = STOP;
	ptr->position = 0;
	//ptr->move_to_position = 0;

	update_timer(ptr);
	int r = 0;

	fputs("============================================\r\n", stdout);
	fputs("Initializing\r\n", stdout);
	fputs("START\r\n",stdout);


	r = L6474_init(ptr);
	fputs("L6474: ",stdout);
	fputs(r==0 ? "OK" : "FAIL", stdout);
	fputs("\r\n",stdout);

	r = ENC28J60_init(ptr);
	init_driver(ptr);



	return ptr;
}

void destroy_controller(struct controller *driver)
{
	free(driver);
}

#define cmpstr(a,b) (strncmp(a, (const char*)b, strlen(a)))

int receive_command(struct controller *ctrl)
{
	ctrl->dat_p = packetloop_icmp_udp(buf, ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

	if(ctrl->dat_p==0 || udp_packet_dport(buf) != ctrl->dport)
	{
		return 0;
	}

	return 1;
}

void parse_command(const char *cmd, struct controller *ptr)
{
	int value;

	cmd = eat_space(cmd);

	if(cmpstr("movesteps", cmd) == 0)
	{
		fputs("command\n",stdout);
		fwrite(cmd, 1, 100, stdout);
		printf("state: %i", ptr->state);
		fputs("\r\n",stdout);

		//ptr->move_finished = true;

		if(ptr->move_finished)
		{
			ptr->msg = "OK";
			ptr->move_finished = 0;
		}
		else
		{
			if(ptr->state == STOP)
			{
				ptr->steps = atol(eat_space(cmd+9));
				printf("ptr->steps: %i\r\n", ptr->steps);
				ptr->state = MOVE_STEPS;
			}
			ptr->msg = "MOVING";
		}

	}
	else if(cmpstr("microstep", cmd) == 0)
	{
		//ptr->move_finished = true;
		ptr->microstep = atol(eat_space(cmd+9));
		printf("mircrostep: 1/%i\r\n", ptr->microstep);
		ptr->msg = "OK";
	}
	else if(cmpstr("move", cmd) == 0)
	{
		ptr->msg = "OK";
		ptr->state = MOVING;
	}
	else if(cmpstr("nop", cmd) == 0)
	{
		ptr->msg = "OK";
	}
	else if(cmpstr("acc", cmd) == 0)
	{
		ptr->msg = "OK";
	}
	else if(cmpstr("stop", cmd) == 0)
	{
		ptr->msg = "OK";
		//fputs("stop\r\n",stdout);
		ptr->state = STOP;
		ptr->steps = 0;
	}
	else if(cmpstr("minspeed", cmd) == 0)
	{
		cmd = eat_space(cmd+8);

		value = atol(cmd);

		printf("setting minspeed: %i\r\n", value);

		if(value < 20000)
		{
			if(ptr->state == MOVING)
			{
				L6474_WaitWhileActive(ptr->motor);
			}

			L6474_SetMinSpeed(ptr->motor, value);
			//get_milliseconds(ptr);
			ptr->cur_speed = value;
		}

		ptr->msg = "OK";

	}
	else if(cmpstr("maxspeed", cmd) == 0)
	{
		cmd = eat_space(cmd+8);

		value = atol(cmd);

		if(value < 20000)
		{
			if(ptr->state == MOVING)
			{
				L6474_WaitWhileActive(ptr->motor);
			}

			L6474_SetMaxSpeed(ptr->motor, value);
		}

		ptr->msg = "OK";

	}
	else if(cmpstr("acceleration", cmd) == 0)
	{
		cmd = eat_space(cmd+8);
		value = atol(cmd);

		if(value < 20000)
		{
			if(ptr->state == MOVING)
			{
				L6474_WaitWhileActive(ptr->motor);
			}

			L6474_SetAcceleration(ptr->motor, (uint16_t) value);
		}

		ptr->msg = "OK";
	}
	else if(cmpstr("deceleration", cmd) == 0)
	{
		cmd = eat_space(cmd+8);
		value = atol(cmd);

		if(value < 20000)
		{
			if(ptr->state == MOVING)
			{
				L6474_WaitWhileActive(ptr->motor);
			}

			L6474_SetDeceleration(ptr->motor, (uint16_t) value);
		}

		ptr->msg = "OK";
	}
	else if(cmpstr("getminspeed", cmd) == 0)
	{
		if(ptr->state == STOP)
		{
			sprintf(ptr->cmd_buf, "%i\r\n", 0);
		}
		else
		{
			sprintf(ptr->cmd_buf, "%i\r\n", ptr->cur_speed);
		}


		ptr->msg = ptr->cmd_buf;

	}
	else if(cmpstr("setpos", cmd) == 0)
	{
		cmd = eat_space(cmd+6);
		value = atol(cmd);

		if(value < 20000)
		{
			ptr->position = value;
		}

		ptr->msg = "OK";
	}
	else if(cmpstr("getpos", cmd) == 0)
	{
		sprintf(ptr->cmd_buf, "%i\r\n", ptr->position);
		ptr->msg = ptr->cmd_buf;
	}
	else if(cmpstr("getdirection", cmd) == 0)
	{
		if(ptr->direction == FWD)
		{
			sprintf(ptr->cmd_buf, "fwd\r\n");
		}
		else
		{
			sprintf(ptr->cmd_buf, "bwd\r\n");
		}


		ptr->msg = ptr->cmd_buf;
	}
	else if(cmpstr("direction", cmd) == 0)
	{
		cmd = eat_space(cmd+9);

		if(cmpstr("fwd", cmd) == 0)
		{
			ptr->direction = FWD;
			ptr->msg = "OK";
		}
		else if(cmpstr("bwd", cmd) == 0)
		{
			ptr->direction = BWD;
			ptr->msg = "OK";
		}
		else
		{
			ptr->msg = "incomplete command";
		}

	}
	else
	{
		ptr->msg = "unkown command";
	}
}

void send_response(struct controller *ctrl)
{
	make_udp_reply_from_request(buf, (char*)ctrl->msg, strlen(ctrl->msg), ctrl->dport );
}

void update(controller *ctrl)
{
	ctrl->msg = "message";

	if(ctrl->state == MOVING)
	{
		driver_move(ctrl);

	}
	else if(ctrl->state == MOVE_STEPS)
	{

		if(ctrl->prev_state == MOVING)
		{
			L6474_WaitWhileActive(ctrl->motor);

		}
		else if(ctrl->prev_state == MOVE_STEPS)
		{
			L6474_WaitWhileActive(ctrl->motor);
		}

		printf("ptr->steps: %i\r\n",ctrl->steps);

		if(ctrl->steps > 0)
		{
			if(L6474_GetDeviceState(ctrl->motor) != INACTIVE){
			}
			else
			{
				driver_move(ctrl);
			}

		}

		if (ctrl->steps <= 0)
		{
			L6474_WaitWhileActive(ctrl->motor);
			L6474_CmdDisable(ctrl->motor);
			ctrl->state = STOP;

			fputs("move finished\r\n",stdout);
			ctrl->move_finished = true;
		}

		ctrl->msg = "MOVING";
	}
	else if(ctrl->state == STOP)
	{
		if(ctrl->prev_state != STOP)
		{
			fputs("stopping\r\n",stdout);
			L6474_HardHiz(ctrl->motor);
		}
	}

	int t = get_milliseconds(ctrl);
	ctrl->delay += t;

	ctrl->prev_state = ctrl->state;


	if(ctrl->delay > 1000)
	{
		if(ctrl->state != STOP)
		{
			fputs("didn't received any input for 1000ms stopping...\r\n",stdout);
			ctrl->state = STOP;
			ctrl->msg = "OK";
			ctrl->move_finished = true;
		}
	}



	if(receive_command(ctrl))
	{
		parse_command((const char*)&buf[ctrl->dat_p], ctrl);
		send_response(ctrl);
		ctrl->delay = 0;
	}

}
