#include <L6474.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pwm.h>
#include "main.h"

#define true 1
#define false 0

static uint8_t number_of_devices = 0;
static uint8_t spi_tx_bursts[L6474_CMD_ARG_MAX_NB_BYTES][MAX_NUMBER_OF_DEVICES];
static uint8_t spi_rx_bursts[L6474_CMD_ARG_MAX_NB_BYTES][MAX_NUMBER_OF_DEVICES];

static bool spi_preemtion_by_isr = false;
static bool isr_flag = false;

#include <IrqHandlers.h>

extern IrqHandlers irqHandlers;
extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;



extern void Error_Handler(void);

typedef struct L6474
{
	 TIM_OC_InitTypeDef sConfigOC;

	 uint8_t who_am_i;
	 void (*error_handler_callback)(uint16_t error);
	 deviceParams_t device_prm;
	 uint8_t device_instance;
} L6474;

/* Error while initialising the SPI. */
#define L6474_ERROR_0        (0x8000)

/* Error of bad SPI transaction. */
#define L6474_ERROR_1        (0x8001)

/* Maximum number of steps. */
#define MAX_STEPS            (0x7FFFFFFF)

/* Maximum frequency of the PWMs in Hz. */
#define L6474_MAX_PWM_FREQ   (10000)

/* Minimum frequency of the PWMs in Hz. */
#define L6474_MIN_PWM_FREQ   (2)

float L6474_Tval_Current_to_Par(struct L6474*, float current_mA);
float L6474_Par_to_Tval_Current(struct L6474*, float Tval);
float L6474_Tmin_Time_to_Par(struct L6474*, float ton_min_us);
float L6474_Par_to_Tmin_Time(struct L6474*, float Tmin);

void L6474_SetIsrFlag()
{
	isr_flag = TRUE;
}

void L6474_ResetIsrFlag()
{
	isr_flag = FALSE;
}

int printf(const char *str, ...);

L6474* createL6474()
{

	if (!(number_of_devices < MAX_NUMBER_OF_DEVICES)) {
		 printf("Instantiation of the L6474 component failed: it can be stacked up to %d times.\r\n", MAX_NUMBER_OF_DEVICES);
		exit(EXIT_FAILURE);
	}

	L6474* ptr = malloc(sizeof(L6474));
	memset(ptr, 0, sizeof(L6474));

	ptr->error_handler_callback = 0;
	ptr->device_instance = number_of_devices++;
	memset(spi_tx_bursts, 0, L6474_CMD_ARG_MAX_NB_BYTES * MAX_NUMBER_OF_DEVICES * sizeof(uint8_t));
	memset(spi_rx_bursts, 0, L6474_CMD_ARG_MAX_NB_BYTES * MAX_NUMBER_OF_DEVICES * sizeof(uint8_t));


	return ptr;
}

int L6474_Init(struct L6474* l6474, void *init)
{

	  L6474_PwmInit(l6474);

	  /* Initialise the L6474s ------------------------------------------------*/

	  /* Standby-reset deactivation */
	  L6474_ReleaseReset(l6474);

	  /* Let a delay after reset */
	  L6474_Delay(l6474, 1);

	  /* Set device parameters to the predefined values from "l6474_target_config.h". */
	  L6474_SetDeviceParamsToPredefinedValues(l6474);

	  if (init == NULL)
	    /* Set device registers to the predefined values from "l6474_target_config.h". */
	    L6474_SetRegisterToPredefinedValues(l6474);
	  else
	    /* Set device registers to the passed initialization values. */
	    L6474_SetRegisterToInitializationValues(l6474, (L6474_init_t *) init);

	  /* Disable L6474 powerstage */
	  L6474_CmdDisable(l6474);

	  /* Get Status to clear flags after start up */
	  L6474_CmdGetStatus(l6474);

	  return COMPONENT_OK;
}

int L6474_ReadID(struct L6474* l6474, uint8_t *id)
{
	  *id = l6474->device_instance;
	  return COMPONENT_OK;
}


unsigned int L6474_CmdGetStatus(struct L6474* l6474)
{

	  uint32_t i;
	  uint16_t status;
	  uint8_t spiIndex = number_of_devices - l6474->device_instance - 1;
	  bool itDisable = FALSE;


	  do
	  {
	    spi_preemtion_by_isr = FALSE;
	    if (itDisable)
	    {
	      /* re-enable L6474_EnableIrq if disable in previous iteration */
	      L6474_EnableIrq(l6474, NULL);
	      itDisable = FALSE;
	    }

	    for (i = 0; i < number_of_devices; i++)
	    {
	       spi_tx_bursts[0][i] = L6474_NOP;
	       spi_tx_bursts[1][i] = L6474_NOP;
	       spi_tx_bursts[2][i] = L6474_NOP;
	       spi_rx_bursts[1][i] = 0;
	       spi_rx_bursts[2][i] = 0;
	    }


	    spi_tx_bursts[0][spiIndex] = L6474_GET_STATUS;

	    /* Disable interruption before checking */
	    /* pre-emption by ISR and SPI transfers*/
	    L6474_DisableIrq(l6474, NULL);
	    itDisable = TRUE;


	  } while (spi_preemtion_by_isr); // check pre-emption by ISR



	  for (i = 0; i < L6474_CMD_ARG_NB_BYTES_GET_STATUS + L6474_RSP_NB_BYTES_GET_STATUS; i++)
	  {
	     L6474_WriteBytes(l6474, &spi_tx_bursts[i][0], &spi_rx_bursts[i][0]);
	  }


	  status = (spi_rx_bursts[1][spiIndex] << 8) | (spi_rx_bursts[2][spiIndex]);

	  /* re-enable L6474_EnableIrq after SPI transfers*/
	  L6474_EnableIrq(l6474, NULL);

	  return (status);

}


float L6474_GetParameter(struct L6474* l6474, unsigned int parameter)
{

    unsigned int register_value = (unsigned int) L6474_CmdGetParam(l6474, (L6474_Registers_t) parameter);
    float value;


    switch ((L6474_Registers_t) parameter) {
        case L6474_TVAL:
            value = L6474_Par_to_Tval_Current(l6474, (float) register_value);
            break;
        case L6474_TON_MIN:
        case L6474_TOFF_MIN:
            value = L6474_Par_to_Tmin_Time(l6474, (float) register_value);
            break;
        default:
            value = (float) register_value;
            break;
    }

    return value;


}

int32_t L6474_ConvertPosition(struct L6474*,uint32_t abs_position_reg);

signed int L6474_GetPosition(struct L6474* l6474)
{
	return L6474_ConvertPosition(l6474, L6474_CmdGetParam(l6474, L6474_ABS_POS));
}


signed int L6474_GetMark(struct L6474* l6474)
{
	return L6474_ConvertPosition(l6474, L6474_CmdGetParam(l6474, L6474_MARK));
}


unsigned int L6474_GetCurrentSpeed(struct L6474* l6474)
{
	return l6474->device_prm.speed;
}


unsigned int L6474_GetMaxSpeed(struct L6474* l6474)
{
	return l6474->device_prm.maxSpeed;
}


unsigned int L6474_GetMinSpeed(struct L6474* l6474)
{
	return l6474->device_prm.minSpeed;
}


unsigned int L6474_GetAcceleration(struct L6474* l6474)
{
	return l6474->device_prm.acceleration;

}


unsigned int L6474_GetDeceleration(struct L6474* l6474)
{
	return l6474->device_prm.deceleration;
}


direction_t L6474_GetDirection(struct L6474* l6474)
{
	return l6474->device_prm.direction;
}

void L6474_SetParameter(struct L6474* l6474, unsigned int parameter, float value)
{
	float register_value;
	switch ((L6474_Registers_t) parameter) {
		case L6474_TVAL:
			register_value = L6474_Tval_Current_to_Par(l6474, value);
			break;
	    case L6474_TON_MIN:
	    case L6474_TOFF_MIN:
	    	register_value = L6474_Tmin_Time_to_Par(l6474, value);
	        break;
	    default:
	    	register_value = value;
	    	break;
	}

	L6474_CmdSetParam(l6474, (L6474_Registers_t) parameter, (unsigned int) register_value);
}

void L6474_SetHome(struct L6474* l6474)
{
	L6474_CmdSetParam(l6474, L6474_ABS_POS, 0);
}

void L6474_SetMark(struct L6474* l6474)
{
	uint32_t mark = L6474_CmdGetParam(l6474, L6474_ABS_POS);
	L6474_CmdSetParam(l6474, L6474_MARK, mark);
}

bool L6474_SetMaxSpeed(struct L6474 *l6474, unsigned int newMaxSpeed)
{
	  bool cmdExecuted = FALSE;
	  if ((newMaxSpeed >= L6474_MIN_PWM_FREQ) &&
	      (newMaxSpeed <= L6474_MAX_PWM_FREQ) &&
	      (l6474->device_prm.minSpeed <= newMaxSpeed) &&
	      ((l6474->device_prm.motionState == INACTIVE)||
	       (l6474->device_prm.commandExecuted == RUN_CMD)))
	  {
	    l6474->device_prm.maxSpeed = newMaxSpeed;
	    cmdExecuted = TRUE;
	  }
	  return cmdExecuted;
}

bool L6474_SetMinSpeed(struct L6474 *l6474, unsigned int newMinSpeed)
{
	  bool cmdExecuted = FALSE;
	  if ((newMinSpeed >= L6474_MIN_PWM_FREQ)&&
	      (newMinSpeed <= L6474_MAX_PWM_FREQ) &&
	      (newMinSpeed <= l6474->device_prm.maxSpeed) &&
	      ((l6474->device_prm.motionState == INACTIVE)||
	       (l6474->device_prm.commandExecuted == RUN_CMD)))
	  {
		  l6474->device_prm.minSpeed = newMinSpeed;
		  cmdExecuted = TRUE;
	  }
	  return cmdExecuted;
}

bool L6474_SetAcceleration(struct L6474 *l6474, uint16_t newAcc)
{
	  bool cmdExecuted = FALSE;
	  if ((newAcc != 0)&&
	      ((l6474->device_prm.motionState == INACTIVE)||
	       (l6474->device_prm.commandExecuted == RUN_CMD)))
	  {
		  l6474->device_prm.acceleration = newAcc;
		  cmdExecuted = TRUE;
	  }
	  return cmdExecuted;
}

bool L6474_SetDeceleration(struct L6474 *l6474, uint16_t newDec)
{
	  bool cmdExecuted = FALSE;
	  if ((newDec != 0)&&
	      ((l6474->device_prm.motionState == INACTIVE)||
	       (l6474->device_prm.commandExecuted == RUN_CMD)))
	  {
		  l6474->device_prm.deceleration = newDec;
	    cmdExecuted = TRUE;
	  }
	  return cmdExecuted;
}

void L6474_ComputeSpeedProfile(struct L6474*,uint32_t nbSteps);

void L6474_GoTo(struct L6474* l6474, int32_t targetPosition)
{
	  motorDir_t direction;
	  int32_t steps;

	  ///* Eventually deactivate motor */
	  if (l6474->device_prm.motionState != INACTIVE)
	  {
		  L6474_HardStop(l6474);
	  }

	  /* Get current position */
	  l6474->device_prm.currentPosition = L6474_ConvertPosition(l6474, L6474_CmdGetParam(l6474, L6474_ABS_POS));

	  /* Compute the number of steps to perform */
	  steps = targetPosition - l6474->device_prm.currentPosition;

	  if (steps >= 0)
	  {
		  l6474->device_prm.stepsToTake = steps;
		  direction = FORWARD;
	  }
	  else
	  {
		  l6474->device_prm.stepsToTake = -steps;
		  direction = BACKWARD;
	  }

	  if (steps != 0)
	  {
		  l6474->device_prm.commandExecuted = MOVE_CMD;

	    /* Direction setup */
		  L6474_SetDirection(l6474, direction);

		  L6474_ComputeSpeedProfile(l6474, l6474->device_prm.stepsToTake);

	    /* Motor activation */
		  L6474_StartMovement(l6474);
	  }
}

void L6474_GoHome(struct L6474* l6474)
{
	L6474_GoTo(l6474, 0);
}

void L6474_GoMark(struct L6474* l6474)
{
    uint32_t mark;

    mark = L6474_ConvertPosition(l6474, L6474_CmdGetParam(l6474, L6474_MARK));
    L6474_GoTo(l6474, mark);
}

void L6474_Run(struct L6474* l6474, direction_t direction)
{
	  /* Eventually deactivate motor */
	  if (l6474->device_prm.motionState != INACTIVE)
	  {
		  L6474_HardStop(l6474);
	  }

	  /* Direction setup */
	  L6474_SetDirection(l6474, direction);

	  l6474->device_prm.commandExecuted = RUN_CMD;

	  /* Motor activation */
	  L6474_StartMovement(l6474);
}



void L6474_Move(struct L6474* l6474, direction_t direction, uint32_t stepCount)
{
	  /* Eventually deactivate motor */
	  if (l6474->device_prm.motionState != INACTIVE)
	  {
		  L6474_HardStop(l6474);
	  }

	  if (stepCount != 0)
	  {
		  l6474->device_prm.stepsToTake = stepCount;

		  l6474->device_prm.commandExecuted = MOVE_CMD;

		  l6474->device_prm.currentPosition = L6474_ConvertPosition(l6474, L6474_CmdGetParam(l6474, L6474_ABS_POS));

	    /* Direction setup */
		  L6474_SetDirection(l6474, direction);

		  L6474_ComputeSpeedProfile(l6474, stepCount);

	    /* Motor activation */
		  L6474_StartMovement(l6474);
	  }
}

int L6474_SoftStop(struct L6474* l6474)
{
	  bool cmdExecuted = FALSE;
	  if (l6474->device_prm.motionState != INACTIVE)
	  {
		  l6474->device_prm.commandExecuted = SOFT_STOP_CMD;
		  cmdExecuted = TRUE;
	  }
	  return (cmdExecuted);
}


void L6474_HardStop(struct L6474* l6474)
{
	  /* Disable corresponding PWM */
	  L6474_PwmStop(l6474);

	  /* Set inactive state */
	  l6474->device_prm.motionState = INACTIVE;
	  l6474->device_prm.commandExecuted = NO_CMD;
	  l6474->device_prm.stepsToTake = MAX_STEPS;
}

void L6474_SoftHiz(struct L6474* l6474)
{
    L6474_SoftStop(l6474);
    L6474_CmdDisable(l6474);
}

void L6474_HardHiz(struct L6474* l6474)
{
    L6474_HardStop(l6474);
    L6474_CmdDisable(l6474);
}


void L6474_WaitWhileActive(struct L6474* l6474)
{
	while (L6474_GetDeviceState(l6474) != INACTIVE);
}


motorState_t L6474_GetDeviceState(struct L6474* l6474)
{
	return l6474->device_prm.motionState;
}

uint16_t L6474_ReadStatusRegister(struct L6474* l6474)
{
	return (L6474_CmdGetParam(l6474, L6474_STATUS));
}

bool L6474_SetStepMode(struct L6474* l6474, step_mode_t step_mode)
{
    if ((motorStepMode_t) step_mode > STEP_MODE_1_16) {
        return false;
    }

    L6474_SoftHiz(l6474);
    L6474_SelectStepMode(l6474, (motorStepMode_t) step_mode);
    return true;
}


void L6474_AttachErrorHandler(struct L6474* l6474, void (*callback)(uint16_t error))
{
	l6474->error_handler_callback = (void (*)(uint16_t error)) callback;
}

void L6474_CmdEnable(struct L6474* l6474)
{
	L6474_SendCommand(l6474, L6474_ENABLE);
}

uint8_t L6474_GetFwVersion(struct L6474* l6474)
{
	return (L6474_FW_VERSION);
}

//...


//status_t L6474_Init(struct L6474*, void *init);
//status_t L6474_ReadID(struct L6474*, uint8_t *id);





//void L6474_WaitWhileActive(struct L6474*) {}
void L6474_CmdDisable(struct L6474* l6474)
{
	L6474_SendCommand(l6474, L6474_DISABLE);
}


//void L6474_CmdEnable(struct L6474*) {}


uint32_t L6474_CmdGetParam(struct L6474* l6474, L6474_Registers_t parameter)
{
	  uint32_t i;
	  uint32_t spiRxData;
	  uint8_t maxArgumentNbBytes = 0;
	  uint8_t spiIndex = number_of_devices - l6474->device_instance - 1;
	  bool itDisable = FALSE;

	  do
	  {
	    spi_preemtion_by_isr = FALSE;
	    if (itDisable)
	    {
	      /* re-enable L6474_EnableIrq if disable in previous iteration */
	      L6474_EnableIrq(l6474, NULL);
	      itDisable = FALSE;
	    }



	    for (i = 0; i < number_of_devices; i++)
	    {
	      spi_tx_bursts[0][i] = L6474_NOP;
	      spi_tx_bursts[1][i] = L6474_NOP;
	      spi_tx_bursts[2][i] = L6474_NOP;
	      spi_tx_bursts[3][i] = L6474_NOP;
	      spi_rx_bursts[1][i] = 0;
	      spi_rx_bursts[2][i] = 0;
	      spi_rx_bursts[3][i] = 0;
	    }

	    switch (parameter)
	    {
	      case L6474_ABS_POS: ;
	      case L6474_MARK:
	        spi_tx_bursts[0][spiIndex] = ((uint8_t)L6474_GET_PARAM )| (parameter);
	        maxArgumentNbBytes = 3;
	        break;
	      case L6474_EL_POS: ;
	      case L6474_CONFIG: ;
	      case L6474_STATUS:
	        spi_tx_bursts[1][spiIndex] = ((uint8_t)L6474_GET_PARAM )| (parameter);
	        maxArgumentNbBytes = 2;
	        break;
	      default:
	        spi_tx_bursts[2][spiIndex] = ((uint8_t)L6474_GET_PARAM )| (parameter);
	        maxArgumentNbBytes = 1;
	    }

	    /* Disable interruption before checking */
	    /* pre-emption by ISR and SPI transfers*/
	    L6474_DisableIrq(l6474, NULL);
	    itDisable = TRUE;
	  } while (spi_preemtion_by_isr); // check pre-emption by ISR

	  for (i = L6474_CMD_ARG_MAX_NB_BYTES-1-maxArgumentNbBytes;
	       i < L6474_CMD_ARG_MAX_NB_BYTES;
	       i++)
	  {
	     L6474_WriteBytes(l6474, &spi_tx_bursts[i][0], &spi_rx_bursts[i][0]);
	  }

	  spiRxData = ((uint32_t)spi_rx_bursts[1][spiIndex] << 16) |
	              (spi_rx_bursts[2][spiIndex] << 8) |
	              (spi_rx_bursts[3][spiIndex]);

	  /* re-enable L6474_EnableIrq after SPI transfers*/
	  L6474_EnableIrq(l6474, NULL);

	  return (spiRxData);
}

//uint16_t L6474_CmdGetStatus(struct L6474*);
void L6474_CmdNop(struct L6474* l6474)
{
	L6474_SendCommand(l6474, L6474_NOP);
}

void L6474_CmdSetParam(struct L6474* l6474,L6474_Registers_t parameter, uint32_t value)
{
	  uint32_t i;
	  uint8_t maxArgumentNbBytes = 0;
	  uint8_t spiIndex = number_of_devices - l6474->device_instance - 1;
	  bool itDisable = FALSE;
	  do
	  {
	    spi_preemtion_by_isr = FALSE;
	    if (itDisable)
	    {
	      /* re-enable L6474_EnableIrq if disable in previous iteration */
	      L6474_EnableIrq(l6474, NULL);
	      itDisable = FALSE;
	    }

	    for (i = 0; i < number_of_devices; i++)
	    {
	      spi_tx_bursts[0][i] = L6474_NOP;
	      spi_tx_bursts[1][i] = L6474_NOP;
	      spi_tx_bursts[2][i] = L6474_NOP;
	      spi_tx_bursts[3][i] = L6474_NOP;
	    }

	    switch (parameter)
	    {
	      case L6474_ABS_POS: ;
	      case L6474_MARK:
	          spi_tx_bursts[0][spiIndex] = parameter;
	          spi_tx_bursts[1][spiIndex] = (uint8_t)(value >> 16);
	          spi_tx_bursts[2][spiIndex] = (uint8_t)(value >> 8);
	          maxArgumentNbBytes = 3;
	          break;
	      case L6474_EL_POS: ;
	      case L6474_CONFIG:
	          spi_tx_bursts[1][spiIndex] = parameter;
	          spi_tx_bursts[2][spiIndex] = (uint8_t)(value >> 8);
	          maxArgumentNbBytes = 2;
	          break;
	      default:
	          spi_tx_bursts[2][spiIndex] = parameter;
	          maxArgumentNbBytes = 1;
	          break;
	  }
	    spi_tx_bursts[3][spiIndex] = (uint8_t)(value);

	    /* Disable interruption before checking */
	    /* pre-emption by ISR and SPI transfers*/
	    L6474_DisableIrq(l6474, NULL);
	    itDisable = TRUE;
	  } while (spi_preemtion_by_isr); // check pre-emption by ISR

	  /* SPI transfer */
	  for (i = L6474_CMD_ARG_MAX_NB_BYTES-1-maxArgumentNbBytes;
	       i < L6474_CMD_ARG_MAX_NB_BYTES;
	       i++)
	  {
	     L6474_WriteBytes(l6474, &spi_tx_bursts[i][0],&spi_rx_bursts[i][0]);
	  }
	  /* re-enable L6474_EnableIrq after SPI transfers*/
	  L6474_EnableIrq(l6474, NULL);
}

void L6474_SelectStepMode(struct L6474* l6474, motorStepMode_t stepMod)
{
	  uint8_t stepModeRegister;
	  L6474_STEP_SEL_t l6474StepMod;

	  switch (stepMod)
	  {
	    case STEP_MODE_FULL:
	      l6474StepMod = L6474_STEP_SEL_1;
	      break;
	    case STEP_MODE_HALF:
	      l6474StepMod = L6474_STEP_SEL_1_2;
	      break;
	    case STEP_MODE_1_4:
	      l6474StepMod = L6474_STEP_SEL_1_4;
	      break;
	    case STEP_MODE_1_8:
	      l6474StepMod = L6474_STEP_SEL_1_8;
	      break;
	    case STEP_MODE_1_16:
	    default:
	      l6474StepMod = L6474_STEP_SEL_1_16;
	      break;
	  }

	  /* Eventually deactivate motor */
	  if (l6474->device_prm.motionState != INACTIVE)
	  {
	    L6474_HardStop(l6474);
	  }

	  /* Read Step mode register and clear STEP_SEL field */
	  stepModeRegister = (uint8_t)(0xF8 & L6474_CmdGetParam(l6474, L6474_STEP_MODE)) ;

	  /* Apply new step mode */
	  L6474_CmdSetParam(l6474, L6474_STEP_MODE, stepModeRegister | (uint8_t)l6474StepMod);

	  /* Reset abs pos register */
	  L6474_SetHome(l6474);

}

//motorDir_t L6474_GetDirection(struct L6474*);

void L6474_SetDirection(struct L6474* l6474, motorDir_t direction)
{
	  if (l6474->device_prm.motionState == INACTIVE)
	  {
		  l6474->device_prm.direction = direction;
		  L6474_SetDirectionGpio(l6474, direction);
	  }
}


void L6474_ApplySpeed(struct L6474* l6474, uint16_t newSpeed)
{
	  if (newSpeed < L6474_MIN_PWM_FREQ)
	  {
		  newSpeed = L6474_MIN_PWM_FREQ;
	  }

	  if (newSpeed > L6474_MAX_PWM_FREQ)
	  {
		  newSpeed = L6474_MAX_PWM_FREQ;
	  }

	  l6474->device_prm.speed = newSpeed;

	  //printf("new speed: %i \r\n", newSpeed);

	  L6474_PwmSetFreq(l6474, newSpeed);
}

void L6474_ComputeSpeedProfile(struct L6474* l6474,uint32_t nbSteps)
{
	uint32_t reqAccSteps;
	uint32_t reqDecSteps;

	/* compute the number of steps to get the targeted speed */
	uint16_t minSpeed = l6474->device_prm.minSpeed;
	reqAccSteps = (l6474->device_prm.maxSpeed - minSpeed);
	reqAccSteps *= (l6474->device_prm.maxSpeed + minSpeed);
	reqDecSteps = reqAccSteps;
	reqAccSteps /= (uint32_t)l6474->device_prm.acceleration;
	reqAccSteps /= 2;

	/* compute the number of steps to stop */
	reqDecSteps /= (uint32_t)l6474->device_prm.deceleration;
	reqDecSteps /= 2;

	if(( reqAccSteps + reqDecSteps ) > nbSteps)
	{
		/* Triangular move  */
		/* reqDecSteps = (Pos * Dec) /(Dec+Acc) */
		uint32_t dec = l6474->device_prm.deceleration;
		uint32_t acc = l6474->device_prm.acceleration;

		reqDecSteps =  ((uint32_t) dec * nbSteps) / (acc + dec);
		if (reqDecSteps > 1)
		{
			reqAccSteps = reqDecSteps - 1;
			if(reqAccSteps == 0)
			{
				reqAccSteps = 1;
			}
		}
		else
		{
			reqAccSteps = 0;
		}

		l6474->device_prm.endAccPos = reqAccSteps;
		l6474->device_prm.startDecPos = reqDecSteps;
	}
	else
	{
		/* Trapezoidal move */
		/* accelerating phase to endAccPos */
		/* steady phase from  endAccPos to startDecPos */
		/* decelerating from startDecPos to stepsToTake*/
		l6474->device_prm.endAccPos = reqAccSteps;
		l6474->device_prm.startDecPos = nbSteps - reqDecSteps - 1;
	}
}


void L6474_ErrorHandler(struct L6474* l6474,uint16_t error)
{
	if (l6474->error_handler_callback != 0)
	{
		(void) l6474->error_handler_callback(error);
	}
	else
	{
		/* Aborting the program. */
		exit(EXIT_FAILURE);
	}
}

void L6474_SendCommand(struct L6474* l6474,uint8_t param)
{
	  uint32_t i;
	  bool itDisable = FALSE;
	  uint8_t spiIndex = number_of_devices - l6474->device_instance - 1;

	  do
	  {
		  spi_preemtion_by_isr = FALSE;
		  if (itDisable)
		  {
	      /* re-enable L6474_EnableIrq if disable in previous iteration */
			L6474_EnableIrq(l6474, NULL);
			itDisable = FALSE;
		  }

		  for (i = 0; i < number_of_devices; i++)
		  {
			  spi_tx_bursts[3][i] = L6474_NOP;
		  }
		  spi_tx_bursts[3][spiIndex] = param;

		  /* Disable interruption before checking */
		  /* pre-emption by ISR and SPI transfers*/
		  L6474_DisableIrq(l6474, NULL);
		  itDisable = TRUE;
	  } while (spi_preemtion_by_isr); // check pre-emption by ISR

	  L6474_WriteBytes(l6474, &spi_tx_bursts[3][0], &spi_rx_bursts[3][0]);

	  /* re-enable L6474_EnableIrq after SPI transfers*/
	  L6474_EnableIrq(l6474, NULL);
}

void L6474_SetRegisterToPredefinedValues(struct L6474* l6474)
{
	  L6474_CmdSetParam(l6474, L6474_ABS_POS, 0);
	  L6474_CmdSetParam(l6474, L6474_EL_POS, 0);
	  L6474_CmdSetParam(l6474, L6474_MARK, 0);
	  switch (l6474->device_instance)
	  {
	  	case 0:
	  	L6474_CmdSetParam(l6474, L6474_TVAL, L6474_Tval_Current_to_Par(l6474, L6474_CONF_PARAM_TVAL_DEVICE_0));
		L6474_CmdSetParam(l6474, L6474_T_FAST, (uint8_t)L6474_CONF_PARAM_TOFF_FAST_DEVICE_0 | (uint8_t)L6474_CONF_PARAM_FAST_STEP_DEVICE_0);
		L6474_CmdSetParam(l6474, L6474_TON_MIN, L6474_Tmin_Time_to_Par(l6474, L6474_CONF_PARAM_TON_MIN_DEVICE_0));
	  	L6474_CmdSetParam(l6474, L6474_TOFF_MIN, L6474_Tmin_Time_to_Par(l6474, L6474_CONF_PARAM_TOFF_MIN_DEVICE_0));
	  	L6474_CmdSetParam(l6474, L6474_OCD_TH, L6474_CONF_PARAM_OCD_TH_DEVICE_0);
	  	L6474_CmdSetParam(l6474, L6474_STEP_MODE, (uint8_t)L6474_CONF_PARAM_STEP_SEL_DEVICE_0 | (uint8_t)L6474_CONF_PARAM_SYNC_SEL_DEVICE_0);
	  	L6474_CmdSetParam(l6474, L6474_ALARM_EN, L6474_CONF_PARAM_ALARM_EN_DEVICE_0);
	  	L6474_CmdSetParam(l6474, L6474_CONFIG, 
				(uint16_t)L6474_CONF_PARAM_CLOCK_SETTING_DEVICE_0 |
				(uint16_t)L6474_CONF_PARAM_TQ_REG_DEVICE_0 |
	  			(uint16_t)L6474_CONF_PARAM_OC_SD_DEVICE_0 |
	  			(uint16_t)L6474_CONF_PARAM_SR_DEVICE_0 |
	  			(uint16_t)L6474_CONF_PARAM_TOFF_DEVICE_0);
	      break;
	      case 1:
	      L6474_CmdSetParam(l6474, L6474_TVAL, L6474_Tval_Current_to_Par(l6474, L6474_CONF_PARAM_TVAL_DEVICE_1));
	      L6474_CmdSetParam(l6474, L6474_T_FAST,
			      (uint8_t)L6474_CONF_PARAM_TOFF_FAST_DEVICE_1 |
			      (uint8_t)L6474_CONF_PARAM_FAST_STEP_DEVICE_1);
	      L6474_CmdSetParam(l6474, L6474_TON_MIN, L6474_Tmin_Time_to_Par(l6474, L6474_CONF_PARAM_TON_MIN_DEVICE_1));
	      L6474_CmdSetParam(l6474, L6474_TOFF_MIN, L6474_Tmin_Time_to_Par(l6474, L6474_CONF_PARAM_TOFF_MIN_DEVICE_1));
	      L6474_CmdSetParam(l6474, L6474_OCD_TH, L6474_CONF_PARAM_OCD_TH_DEVICE_1);
	      L6474_CmdSetParam(l6474, L6474_STEP_MODE, 
			      (uint8_t)L6474_CONF_PARAM_STEP_SEL_DEVICE_1 |
			      (uint8_t)L6474_CONF_PARAM_SYNC_SEL_DEVICE_1);
	      L6474_CmdSetParam(l6474, L6474_ALARM_EN, L6474_CONF_PARAM_ALARM_EN_DEVICE_1);
	      L6474_CmdSetParam(l6474, L6474_CONFIG, 
			      (uint16_t)L6474_CONF_PARAM_CLOCK_SETTING_DEVICE_1 |
			      (uint16_t)L6474_CONF_PARAM_TQ_REG_DEVICE_1 |
			      (uint16_t)L6474_CONF_PARAM_OC_SD_DEVICE_1 |
			      (uint16_t)L6474_CONF_PARAM_SR_DEVICE_1 |
			      (uint16_t)L6474_CONF_PARAM_TOFF_DEVICE_1);
	      break;
	      case 2:
	      L6474_CmdSetParam(l6474, L6474_TVAL, L6474_Tval_Current_to_Par(l6474, L6474_CONF_PARAM_TVAL_DEVICE_2));
	      L6474_CmdSetParam(l6474, L6474_T_FAST,
			      (uint8_t)L6474_CONF_PARAM_TOFF_FAST_DEVICE_2 |
			      (uint8_t)L6474_CONF_PARAM_FAST_STEP_DEVICE_2);
	      L6474_CmdSetParam(l6474, L6474_TON_MIN, L6474_Tmin_Time_to_Par(l6474, L6474_CONF_PARAM_TON_MIN_DEVICE_2));
	      L6474_CmdSetParam(l6474, L6474_TOFF_MIN, L6474_Tmin_Time_to_Par(l6474, L6474_CONF_PARAM_TOFF_MIN_DEVICE_2));
	      L6474_CmdSetParam(l6474, L6474_OCD_TH, L6474_CONF_PARAM_OCD_TH_DEVICE_2);
	      L6474_CmdSetParam(l6474, L6474_STEP_MODE, 
			      (uint8_t)L6474_CONF_PARAM_STEP_SEL_DEVICE_2 |
			      (uint8_t)L6474_CONF_PARAM_SYNC_SEL_DEVICE_2);
	      L6474_CmdSetParam(l6474, L6474_ALARM_EN, L6474_CONF_PARAM_ALARM_EN_DEVICE_2);
	      L6474_CmdSetParam(l6474, L6474_CONFIG,
			      (uint16_t)L6474_CONF_PARAM_CLOCK_SETTING_DEVICE_2 |
			      (uint16_t)L6474_CONF_PARAM_TQ_REG_DEVICE_2 |
			      (uint16_t)L6474_CONF_PARAM_OC_SD_DEVICE_2 |
			      (uint16_t)L6474_CONF_PARAM_SR_DEVICE_2 |
			      (uint16_t)L6474_CONF_PARAM_TOFF_DEVICE_2);
	      break;
	      default: ;
	 }

}

void L6474_SetRegisterToInitializationValues(struct L6474* l6474, L6474_init_t *init)
{
	  L6474_CmdSetParam(l6474, L6474_ABS_POS, 0);
	  L6474_CmdSetParam(l6474, L6474_EL_POS, 0);
	  L6474_CmdSetParam(l6474, L6474_MARK, 0);
	  L6474_CmdSetParam(l6474, L6474_TVAL, L6474_Tval_Current_to_Par(l6474, init->torque_regulation_current_mA));
	  L6474_CmdSetParam(l6474, L6474_T_FAST, 
			  (uint8_t) init->maximum_fast_decay_time |
			  (uint8_t) init->fall_time);
	  L6474_CmdSetParam(l6474, L6474_TON_MIN, L6474_Tmin_Time_to_Par(l6474, init->minimum_ON_time_us));
	  L6474_CmdSetParam(l6474, L6474_TOFF_MIN, L6474_Tmin_Time_to_Par(l6474, init->minimum_OFF_time_us));
	  L6474_CmdSetParam(l6474, L6474_OCD_TH, init->overcurrent_threshold);
	  L6474_CmdSetParam(l6474, L6474_STEP_MODE,
			  (uint8_t) init->step_selection |
			  (uint8_t) init->sync_selection);
	  L6474_CmdSetParam(l6474, L6474_ALARM_EN, init->alarm);
	  L6474_CmdSetParam(l6474, L6474_CONFIG,
			  (uint16_t) init->clock |
			  (uint16_t) init->torque_regulation_method |
			  (uint16_t) init->overcurrent_shutwdown |
			  (uint16_t) init->slew_rate |
			  (uint16_t) init->target_swicthing_period);
	  L6474_SetAcceleration(l6474, (uint16_t) init->acceleration_pps_2);
	  L6474_SetDeceleration(l6474, (uint16_t) init->deceleration_pps_2);
	  L6474_SetMaxSpeed(l6474, (uint16_t) init->maximum_speed_pps);
	  L6474_SetMinSpeed(l6474, (uint16_t) init->minimum_speed_pps);
}

status_t L6474_ReadWrite(struct L6474 *l6474,uint8_t* pBufferToRead,uint8_t* pBufferToWrite,uint16_t NumBytes);

uint8_t L6474_SpiWriteBytes(struct L6474* l6474, uint8_t *pByteToTransmit, uint8_t *pReceivedByte)
{
	return (uint8_t) (L6474_ReadWrite(l6474, pReceivedByte, pByteToTransmit, number_of_devices) == COMPONENT_OK ? 0 : 1);
}

void L6474_WriteBytes(struct L6474* l6474, uint8_t *pByteToTransmit, uint8_t *pReceivedByte)
{
	  if (L6474_SpiWriteBytes(l6474, pByteToTransmit, pReceivedByte) != 0)
	  {
		  L6474_ErrorHandler(l6474, L6474_ERROR_1);
	  }

	  if (isr_flag)
	  {
	    spi_preemtion_by_isr = TRUE;
	  }
}



void L6474_SetDeviceParamsToPredefinedValues(struct L6474* l6474)
{
	switch (l6474->device_instance)
	{
		case 0:
		l6474->device_prm.acceleration = L6474_CONF_PARAM_ACC_DEVICE_0;
		l6474->device_prm.deceleration = L6474_CONF_PARAM_DEC_DEVICE_0;
		l6474->device_prm.maxSpeed = L6474_CONF_PARAM_MAX_SPEED_DEVICE_0;
		l6474->device_prm.minSpeed = L6474_CONF_PARAM_MIN_SPEED_DEVICE_0;
		break;
		
		case 1:
		l6474->device_prm.acceleration = L6474_CONF_PARAM_ACC_DEVICE_1;
		l6474->device_prm.deceleration = L6474_CONF_PARAM_DEC_DEVICE_1;
		l6474->device_prm.maxSpeed = L6474_CONF_PARAM_MAX_SPEED_DEVICE_1;
		l6474->device_prm.minSpeed = L6474_CONF_PARAM_MIN_SPEED_DEVICE_1;
		break;
		
		case 2:
		l6474->device_prm.acceleration = L6474_CONF_PARAM_ACC_DEVICE_2;
		l6474->device_prm.deceleration = L6474_CONF_PARAM_DEC_DEVICE_2;
		l6474->device_prm.maxSpeed = L6474_CONF_PARAM_MAX_SPEED_DEVICE_2;
		l6474->device_prm.minSpeed = L6474_CONF_PARAM_MIN_SPEED_DEVICE_2;
		break;
	  }

	  l6474->device_prm.accu = 0;
	  l6474->device_prm.currentPosition = 0;
	  l6474->device_prm.endAccPos = 0;
	  l6474->device_prm.relativePos = 0;
	  l6474->device_prm.startDecPos = 0;
	  l6474->device_prm.stepsToTake = 0;
	  l6474->device_prm.speed = 0;
	  l6474->device_prm.commandExecuted = NO_CMD;
	  l6474->device_prm.direction = FORWARD;
	  l6474->device_prm.motionState = INACTIVE;
}

void L6474_StartMovement(struct L6474* l6474)
{
	/* Enable L6474 powerstage */
	L6474_CmdEnable(l6474);
	if (l6474->device_prm.endAccPos != 0)
	{
		l6474->device_prm.motionState = ACCELERATING;
	}
	else
	{
		l6474->device_prm.motionState = DECELERATING;
	}
	
	l6474->device_prm.accu = 0;
	l6474->device_prm.relativePos = 0;
	L6474_ApplySpeed(l6474, l6474->device_prm.minSpeed);
}

void L6474_StepClockHandler(struct L6474* l6474)
{

	  /* Set isr flag */
	  isr_flag = TRUE;

	  /* Incrementation of the relative position */
	  l6474->device_prm.relativePos++;
	  switch (l6474->device_prm.motionState)
	  {
	  	  case ACCELERATING:
	  	  {
			  uint32_t relPos = l6474->device_prm.relativePos;
			  uint32_t endAccPos = l6474->device_prm.endAccPos;
			  uint16_t speed = l6474->device_prm.speed;
			  uint32_t acc = ((uint32_t)l6474->device_prm.acceleration << 16);

	        	if ((l6474->device_prm.commandExecuted == SOFT_STOP_CMD)||
	            		((l6474->device_prm.commandExecuted != RUN_CMD) &&
	             		(relPos == l6474->device_prm.startDecPos)))
	        	{
	          		l6474->device_prm.motionState = DECELERATING;
	          		l6474->device_prm.accu = 0;
	        	}
			else if ((speed >= l6474->device_prm.maxSpeed) ||
				((l6474->device_prm.commandExecuted != RUN_CMD) && 
				 (relPos == endAccPos)))
			{
				l6474->device_prm.motionState = STEADY;
			}
	        	else
	        	{
				bool speedUpdated = FALSE;
				/* Go on accelerating */
				if (speed == 0) speed = 1;


	  			l6474->device_prm.accu += acc / speed;
				while (l6474->device_prm.accu >= (0X10000L))
				{
					l6474->device_prm.accu -= (0X10000L);
					speed +=1;
					speedUpdated = TRUE;
	        		}
		
				if (speedUpdated)
				{
					if (speed > l6474->device_prm.maxSpeed)
					{
						speed = l6474->device_prm.maxSpeed;
					}
			
					l6474->device_prm.speed = speed;
					L6474_ApplySpeed(l6474, l6474->device_prm.speed);
	       			}
	  		}
	  	      break;
	  	}
		case STEADY:
		{
			uint16_t maxSpeed = l6474->device_prm.maxSpeed;
			uint32_t relativePos = l6474->device_prm.relativePos;
			if  ((l6474->device_prm.commandExecuted == SOFT_STOP_CMD) ||
					((l6474->device_prm.commandExecuted != RUN_CMD) &&
					 (relativePos >= (l6474->device_prm.startDecPos))) ||
					((l6474->device_prm.commandExecuted == RUN_CMD) &&
					 (l6474->device_prm.speed > maxSpeed)))
			{
				l6474->device_prm.motionState = DECELERATING;
				l6474->device_prm.accu = 0;
			}
			else if ((l6474->device_prm.commandExecuted == RUN_CMD)
					&& (l6474->device_prm.speed < maxSpeed))
			{
				l6474->device_prm.motionState = ACCELERATING;
				l6474->device_prm.accu = 0;
			}
			
			break;
	       }
		case DECELERATING:
		{
	  		uint32_t relativePos = l6474->device_prm.relativePos;
			uint16_t speed = l6474->device_prm.speed;
			uint32_t deceleration = ((uint32_t)l6474->device_prm.deceleration << 16);
			if (((l6474->device_prm.commandExecuted == SOFT_STOP_CMD) && 
						(speed <=  l6474->device_prm.minSpeed)) ||
						((l6474->device_prm.commandExecuted != RUN_CMD) &&
						 (relativePos >= l6474->device_prm.stepsToTake)))
			{
				/* Motion process complete */
				L6474_HardStop(l6474);
			}
			else if ((l6474->device_prm.commandExecuted == RUN_CMD) &&
					(speed <= l6474->device_prm.maxSpeed))
			{
				l6474->device_prm.motionState = STEADY;
			}
			else
			{
				/* Go on decelerating */
				if (speed > l6474->device_prm.minSpeed)
				{
					bool speedUpdated = FALSE;
					if (speed == 0) speed = 1;
					l6474->device_prm.accu += deceleration / speed;
					while (l6474->device_prm.accu >= (0X10000L))
	  				{
	  					l6474->device_prm.accu -= (0X10000L);
	  					if (speed > 1)
	  					{
	  						speed -=1;
	  					}
	  					speedUpdated = TRUE;
	  				}

	  				if (speedUpdated)
	 				{
	  					if (speed < l6474->device_prm.minSpeed)
	  					{
	  						speed = l6474->device_prm.minSpeed;
	  					}
	  					
						l6474->device_prm.speed = speed;
						L6474_ApplySpeed(l6474, l6474->device_prm.speed);
	  				}
	  			}
	  	    }
	  	
		    break;
	  	}
	  	default:
	  	{
			break;
	  	}
	  }
	  /* Set isr flag */
	  isr_flag = FALSE;
}

float L6474_Tval_Current_to_Par(struct L6474* l6474, float current_mA)
{
	return ((float)(((current_mA - 31.25f) / 31.25f) + 0.5f));
}

float L6474_Par_to_Tval_Current(struct L6474* l6474, float Tval)
{
	return ((float)((Tval - 0.5f) * 31.25f + 31.25f));
}

float L6474_Tmin_Time_to_Par(struct L6474* l6474, float ton_min_us)
{
	return ((float)(((ton_min_us - 0.5f) * 2.0f) + 0.5f));
}

float L6474_Par_to_Tmin_Time(struct L6474* l6474, float Tmin)
{
	return ((float)(((Tmin - 0.5f) / 2.0f) + 0.5f));
}


int32_t L6474_ConvertPosition(struct L6474* l6474, uint32_t abs_position_reg)
{
	  int32_t operation_result;

	  if (abs_position_reg & L6474_ABS_POS_SIGN_BIT_MASK)
	  {
	    /* Negative register value */
	    abs_position_reg = ~abs_position_reg;
	    abs_position_reg += 1;

	    operation_result = (int32_t) (abs_position_reg & L6474_ABS_POS_VALUE_MASK);
	    operation_result = -operation_result;
	  }
	  else
	  {
	    operation_result = (int32_t) abs_position_reg;
	  }

	  return operation_result;
}


status_t L6474_Read(struct L6474*, uint8_t* pBuffer, uint16_t NumBytesToRead)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_StatusTypeDef result = HAL_SPI_Receive(&hspi1, pBuffer, NumBytesToRead, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	if(result != HAL_OK)
	{
		return COMPONENT_ERROR;
	}
	else
	{
		return COMPONENT_OK;
	}
}

status_t L6474_Write(struct L6474*, uint8_t* pBuffer, uint16_t NumBytesToWrite)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_StatusTypeDef result = HAL_SPI_Transmit(&hspi1, pBuffer, NumBytesToWrite,100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	if(result != HAL_OK)
	{
		return COMPONENT_ERROR;
	}
	else
	{
		return COMPONENT_OK;
	}
}

status_t L6474_ReadWrite(
		struct L6474 *l6474,
		uint8_t* pBufferToRead,
		uint8_t* pBufferToWrite,
		uint16_t NumBytes)
{

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_StatusTypeDef result = HAL_OK;

	//result = HAL_SPI_Transmit_DMA(&hspi1, pBufferToRead, NumBytes);

	//if(result == HAL_OK)
	//{
	//	result = HAL_SPI_Receive_DMA(&hspi1, pBufferToWrite, NumBytes);
	//}

	result = HAL_SPI_TransmitReceive(&hspi1, pBufferToWrite, pBufferToRead, NumBytes,100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	//HAL_Delay(100);

	//while(wTransferState == TRANSFER_WAIT) {}

	if(result != HAL_OK)
	{
		return COMPONENT_ERROR;
	}
	else
	{
		return COMPONENT_OK;
	}

}

void L6474_Delay(struct L6474*,uint32_t delay)
{
	HAL_Delay(delay);
}

///Enable Irq
void L6474_EnableIrq(struct L6474 *l6474,void *handle)
{
	__enable_irq();
}

///Disable Irq
void L6474_DisableIrq(struct L6474*,void *handle)
{
	__disable_irq();
}



void interrupt_handler(void *userData)
{
	L6474_StepClockHandler((L6474*)userData);
}


void L6474_PwmSetFreq(struct L6474* l6474, uint16_t newFreq)
{
    /* Computing the period of PWM. */
    double period = 1.0f / newFreq;
    //printf("frequency: %i, period: %f\r\n", newFreq, period);

    /* Setting the period and the duty-cycle of PWM. */
    pwm_period(period);
    pwm_write(0.5f);

    /* Setting a callback with the same period of PWM's, to update the state machine. */
    //ticker.attach(Callback<void()>(this, &L6474::L6474_StepClockHandler), period);

    irqHandlers.tim3.userptr = l6474;
    irqHandlers.tim3.func = interrupt_handler;

    /*
    irqHandlers.tim3.userptr=this;
	irqHandlers.tim3.func = [](void *userData)
	{
		((L6474*)userData)->L6474_StepClockHandler();
	};
	*/
}

///Set PWM1 frequency and start it
void L6474_Pwm1SetFreq(struct L6474*,void *handle, uint16_t newFreq)
{

}

///Set PWM2 frequency and start it
void L6474_Pwm2SetFreq(struct L6474*,void *handle, uint16_t newFreq)
{


}
///Set PWM3 frequency and start it
void L6474_Pwm3SetFreq(struct L6474*,void *handle, uint16_t newFreq)
{


}
///Init the PWM






///Stop the PWM
void L6474_PwmStop(struct L6474*)
{
	pwm_write(0.0f);


	//pwm.write(0.0f);
	irqHandlers.tim3.func = NULL;
	irqHandlers.tim3.userptr = NULL;
}

///Reset the L6474 reset pin
void L6474_ReleaseReset(struct L6474*)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
}


///Set the L6474 reset pin
void L6474_Reset(struct L6474*)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
}

///Set direction GPIO
void L6474_SetDirectionGpio(struct L6474*, uint8_t gpioState)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, (GPIO_PinState)gpioState);
}

///Write bytes to the L6474s via SPI


