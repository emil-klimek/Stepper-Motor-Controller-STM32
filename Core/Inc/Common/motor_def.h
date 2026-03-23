#ifndef MOTOR_DEF_H
#define MOTOR_DEF_H

#include <stdint.h>
#include <component_def.h>

#ifndef __cplusplus
typedef int bool;
#endif

#ifndef FALSE
#define FALSE (0)
#endif
/// boolean for true condition
#ifndef TRUE
#define TRUE  (1)
#endif

typedef enum {
  BACKWARD = 0,
  FORWARD = 1
} motorDir_t;

typedef enum {
  ACTION_RESET = ((uint8_t)0x00),
  ACTION_COPY  = ((uint8_t)0x08)
} motorAction_t;

typedef enum {
  ACCELERATING = 0,
  DECELERATING = 1,
  STEADY = 2,
  INACTIVE= 3
} motorState_t;

typedef enum {
  STEP_MODE_FULL   = ((uint8_t)0x00),
  STEP_MODE_HALF   = ((uint8_t)0x01),
  STEP_MODE_1_4    = ((uint8_t)0x02),
  STEP_MODE_1_8    = ((uint8_t)0x03),
  STEP_MODE_1_16   = ((uint8_t)0x04),
  STEP_MODE_1_32   = ((uint8_t)0x05),
  STEP_MODE_1_64   = ((uint8_t)0x06),
  STEP_MODE_1_128  = ((uint8_t)0x07)
} motorStepMode_t;

typedef enum {
  RUN_CMD,
  MOVE_CMD,
  SOFT_STOP_CMD,
  NO_CMD
} deviceCommand_t;

typedef struct {
  /// accumulator used to store speed increase smaller than 1 pps
  volatile uint32_t accu;
  /// Position in steps at the start of the goto or move commands
  volatile int32_t currentPosition;
  /// position in step at the end of the accelerating phase
  volatile uint32_t endAccPos;
  /// nb steps performed from the beggining of the goto or the move command
  volatile uint32_t relativePos;
  /// position in step at the start of the decelerating phase
  volatile uint32_t startDecPos;
  /// nb steps to perform for the goto or move commands
  volatile uint32_t stepsToTake;
  /// acceleration in pps^2
  volatile uint16_t acceleration;
  /// deceleration in pps^2
  volatile uint16_t deceleration;
  /// max speed in pps (speed use for goto or move command)
  volatile uint16_t maxSpeed;
  /// min speed in pps
  volatile uint16_t minSpeed;
  /// current speed in pps
  volatile uint16_t speed;
  /// command under execution
  volatile deviceCommand_t commandExecuted;
  /// FORWARD or BACKWARD direction
  volatile motorDir_t direction;
  /// Current State of the device
  volatile motorState_t motionState;
} deviceParams_t;

typedef struct
{
  /* ACTION ----------------------------------------------------------------*
   * Declare here the component's generic functions.                        *
   * Tag this group of functions with the " Generic " C-style comment.      *
   * A component's interface has to define at least the two generic         *
   * functions provided here below within the "Example" section, as the     *
   * first and second functions of its Virtual Table. They have to be       *
   * specified exactly in the given way.                                    *
   *                                                                        *
   * Example:                                                               *
   *   status_t (*Init)   (void *handle, void *init);                       *
   *   status_t (*ReadID) (void *handle, uint8_t *id);                      *
   *------------------------------------------------------------------------*/
  /* Generic */
  status_t (*Init)(void *handle, void *init);
  status_t (*ReadID)(void *handle, uint8_t *id);

  /* ACTION ----------------------------------------------------------------*
   * Declare here the component's interrupts related functions.             *
   * Tag this group of functions with the " Interrupts " C-style comment.   *
   * Do not specify any function if not required.                           *
   *                                                                        *
   * Example:                                                               *
   *   void     (*ConfigIT) (void *handle, int a);                          *
   *------------------------------------------------------------------------*/
  /* Interrupts */
  /// Function pointer to AttachErrorHandler
  void (*AttachErrorHandler)(void *handle, void (*callback)(void *handle, uint16_t error));
  /// Function pointer to AttachFlagInterrupt
  void (*AttachFlagInterrupt)(void *handle, void (*callback)(void *handle));
  /// Function pointer to AttachBusyInterrupt
  void (*AttachBusyInterrupt)(void *handle, void (*callback)(void *handle));
  /// Function pointer to FlagInterruptHandler
  void (*FlagInterruptHandler)(void *handle);

  /* ACTION ----------------------------------------------------------------*
   * Declare here the component's specific functions.                       *
   * Tag this group of functions with the " Specific " C-style comment.     *
   * Do not specify any function if not required.                           *
   *                                                                        *
   * Example:                                                               *
   *   status_t (*GetValue) (void *handle, float *f);                       *
   *------------------------------------------------------------------------*/
  /* Specific */
  /// Function pointer to GetAcceleration
  uint16_t (*GetAcceleration)(void *handle);
  /// Function pointer to GetCurrentSpeed
  uint16_t (*GetCurrentSpeed)(void *handle);
  /// Function pointer to GetDeceleration
  uint16_t (*GetDeceleration)(void *handle);
  /// Function pointer to GetDeviceState
  motorState_t(*GetDeviceState)(void *handle);
  /// Function pointer to GetFwVersion
  uint8_t (*GetFwVersion)(void *handle);
  /// Function pointer to GetMark
  int32_t (*GetMark)(void *handle);
  /// Function pointer to GetMaxSpeed
  uint16_t (*GetMaxSpeed)(void *handle);
  /// Function pointer to GetMinSpeed
  uint16_t (*GetMinSpeed)(void *handle);
  /// Function pointer to GetPosition
  int32_t (*GetPosition)(void *handle);
  /// Function pointer to GoHome
  void (*GoHome)(void *handle);
  /// Function pointer to GoMark
  void (*GoMark)(void *handle);
  /// Function pointer to GoTo
  void (*GoTo)(void *handle, int32_t targetPosition);
  /// Function pointer to HardStop
  void (*HardStop)(void *handle);
  /// Function pointer to Move
  void (*Move)(void *handle, motorDir_t direction, uint32_t stepCount);
  /// Function pointer to ResetAllDevices
  //void (*ResetAllDevices)(void *handle);
  /// Function pointer to Run
  void (*Run)(void *handle, motorDir_t direction);
  /// Function pointer to SetAcceleration
  bool (*SetAcceleration)(void *handle, uint16_t newAcc);
  /// Function pointer to SetDeceleration
  bool (*SetDeceleration)(void *handle, uint16_t newDec);
  /// Function pointer to SetHome
  void (*SetHome)(void *handle);
  /// Function pointer to SetMark
  void (*SetMark)(void *handle);
  /// Function pointer to SetMaxSpeed
  bool (*SetMaxSpeed)(void *handle, uint16_t newMaxSpeed);
  /// Function pointer to SetMinSpeed
  bool (*SetMinSpeed)(void *handle, uint16_t newMinSpeed);
  /// Function pointer to SoftStop
  bool (*SoftStop)(void *handle);
  /// Function pointer to StepClockHandler
  void (*StepClockHandler)(void *handle);
  /// Function pointer to WaitWhileActive
  void (*WaitWhileActive)(void *handle);
  /// Function pointer to CmdDisable
  void (*CmdDisable)(void *handle);
  /// Function pointer to CmdEnable
  void (*CmdEnable)(void *handle);
  /// Function pointer to CmdGetParam
  uint32_t (*CmdGetParam)(void *handle, uint32_t param);
  /// Function pointer to CmdGetStatus
  uint16_t (*CmdGetStatus)(void *handle);
  /// Function pointer to CmdNop
  void (*CmdNop)(void *handle);
  /// Function pointer to CmdSetParam
  void (*CmdSetParam)(void *handle, uint32_t param, uint32_t value);
  /// Function pointer to ReadStatusRegister
  uint16_t (*ReadStatusRegister)(void *handle);
  /// Function pointer to ReleaseReset
  void (*ReleaseReset)(void *handle);
  /// Function pointer to Reset
  void (*Reset)(void *handle);
  /// Function pointer to SelectStepMode
  void (*SelectStepMode)(void *handle, motorStepMode_t stepMod);
  /// Function pointer to SetDirection
  void (*SetDirection)(void *handle, motorDir_t direction);
  /// Function pointer to CmdGoToDir
  void (*CmdGoToDir)(void *handle, motorDir_t direction, int32_t targetPosition);
  /// Function pointer to CheckBusyHw
  uint8_t (*CheckBusyHw)(void *handle);
  /// Function pointer to CheckStatusHw
  uint8_t (*CheckStatusHw)(void *handle);
  /// Function pointer to CmdGoUntil
  void (*CmdGoUntil)(void *handle, motorAction_t action, motorDir_t direction, uint32_t targetPosition);
  /// Function pointer to CmdHardHiZ
  void (*CmdHardHiZ)(void *handle);
  /// Function pointer to CmdReleaseSw
  void (*CmdReleaseSw)(void *handle, motorAction_t action, motorDir_t direction);
  /// Function pointer to CmdResetDevice
  void (*CmdResetDevice)(void *handle);
  /// Function pointer to CmdResetPos
  void (*CmdResetPos)(void *handle);
  /// Function pointer to CmdRun
  void (*CmdRun)(void *handle, motorDir_t direction, uint32_t targetPosition);
  /// Function pointer to CmdSoftHiZ
  void (*CmdSoftHiZ)(void *handle);
  /// Function pointer to CmdStepClock
  void (*CmdStepClock)(void *handle, motorDir_t direction);
  /// Function pointer to FetchAndClearAllStatus
  void (*FetchAndClearAllStatus)(void *handle);
  /// Function pointer to GetFetchedStatus
  uint16_t (*GetFetchedStatus)(void *handle);
  /// Function pointer to GetNbDevices
  uint8_t (*GetNbDevices)(void *handle);
  /// Function pointer to IsDeviceBusy
  bool (*IsDeviceBusy)(void *handle);
  /// Function pointer to SendQueuedCommands
  void (*SendQueuedCommands)(void *handle);
  /// Function pointer to QueueCommands
  void (*QueueCommands)(void *handle, uint8_t temp, uint32_t command);
  /// Function pointer to WaitForAllDevicesNotBusy
  void (*WaitForAllDevicesNotBusy)(void *handle);
  /// Function pointer to ErrorHandler
  void (*ErrorHandler)(void *handle, uint16_t error);
  /// Function pointer to BusyInterruptHandler
  void (*BusyInterruptHandler)(void *handle);
  /// Function pointer to CmdSoftStop
  void (*CmdSoftStop)(void *handle);
} MOTOR_VTable_t;


#endif
