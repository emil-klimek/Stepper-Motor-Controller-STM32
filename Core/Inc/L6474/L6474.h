#ifndef L6474_NEW_H
#define L6474_NEW_H


#define NULL 0
#ifndef __cplusplus
typedef int bool;
#endif


#include <api.h>

#include <motor_def.h>
#include <L6474_def.h>


typedef enum {
    BWD = 0, /* Backward. */
    FWD = 1  /* Forward. */
} direction_t;

typedef int step_mode_t;

typedef struct L6474 L6474;

 void L6474_EnableIrq(struct L6474 *l6474,void *handle);
 void L6474_DisableIrq(struct L6474*,void *handle);

///Disable Irq


 void L6474_SetIsrFlag();
 void L6474_ResetIsrFlag();
 L6474* createL6474();
 int L6474_Init(struct L6474*, void *init);
 int L6474_ReadID(struct L6474*, uint8_t *id);
 unsigned int L6474_CmdGetStatus(struct L6474*);
 float L6474_GetParameter(struct L6474*, unsigned int parameter);
 signed int L6474_GetPosition(struct L6474*);
 signed int L6474_GetMark(struct L6474*);
 unsigned int L6474_GetCurrentSpeed(struct L6474*);
 unsigned int L6474_GetMaxSpeed(struct L6474*);
 unsigned int L6474_GetMinSpeed(struct L6474*);
 unsigned int L6474_GetAcceleration(struct L6474*);
 unsigned int L6474_GetDeceleration(struct L6474*);
 direction_t L6474_GetDirection(struct L6474*);
 void L6474_SetParameter(struct L6474*, unsigned int parameter, float value);
 void L6474_SetHome(struct L6474*);
 void L6474_SetMark(struct L6474*);
  bool L6474_SetMaxSpeed(struct L6474*, unsigned int speed);
 bool L6474_SetMinSpeed(struct L6474*, unsigned int speed);
 bool L6474_SetAcceleration(struct L6474*, uint16_t newAcc);
 bool L6474_SetDeceleration(struct L6474*, uint16_t newDec);
 void L6474_GoTo(struct L6474*, int32_t targetPosition);
 void L6474_GoHome(struct L6474*);
 void L6474_GoMark(struct L6474*);
 void L6474_Run(struct L6474*, direction_t direction);
 void L6474_Move(struct L6474*, direction_t direction, uint32_t stepCount);
 int L6474_SoftStop(struct L6474*);
 void L6474_HardStop(struct L6474*);
 void L6474_SoftHiz(struct L6474*);
 void L6474_HardHiz(struct L6474*);
 void L6474_WaitWhileActive(struct L6474*);
 motorState_t L6474_GetDeviceState(struct L6474*);
 uint16_t L6474_ReadStatusRegister(struct L6474*);
 bool L6474_SetStepMode(struct L6474*, step_mode_t step_mode);
 void L6474_AttachErrorHandler(struct L6474*, void (*fptr)(uint16_t error));
 void L6474_CmdEnable(struct L6474*);
 void L6474_CmdDisable(struct L6474*);
 uint8_t L6474_GetFwVersion(struct L6474*);
 void L6474_Delay(struct L6474*,uint32_t delay);
 void L6474_PwmInit(struct L6474*);
 void L6474_ReleaseReset(struct L6474*);
 uint32_t L6474_CmdGetParam(struct L6474*, L6474_Registers_t parameter);
 float L6474_Par_to_Tval_Current(struct L6474*, float Tval);
 float L6474_Par_to_Tmin_Time(struct L6474*, float Tmin);
 void L6474_CmdSetParam(struct L6474* l6474,L6474_Registers_t parameter, uint32_t value);
 void L6474_SelectStepMode(struct L6474* l6474, motorStepMode_t stepMod);
 void L6474_SetDirection(struct L6474* l6474, motorDir_t direction);
 void L6474_SetDirectionGpio(struct L6474*, uint8_t gpioState);
 void L6474_SetDeviceParamsToPredefinedValues(struct L6474* l6474);
 void L6474_PwmStop(struct L6474*);
 void L6474_PwmSetFreq(struct L6474* l6474, uint16_t newFreq);
 void L6474_StartMovement(struct L6474* l6474);
 void L6474_WriteBytes(struct L6474* l6474, uint8_t *pByteToTransmit, uint8_t *pReceivedByte);
 void L6474_SetRegisterToInitializationValues(struct L6474* l6474, L6474_init_t *init);
 void L6474_SetRegisterToPredefinedValues(struct L6474* l6474);
 void L6474_SendCommand(struct L6474* l6474,uint8_t param);
 void L6474_CmdDisable(struct L6474* l6474);
 void L6474_CmdDisable(struct L6474* l6474);




#endif
