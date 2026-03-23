#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include <api.h>


typedef enum state
{
	MOVING,
	MOVE_STEPS,
	STOP
} state;

typedef struct controller controller;





C_API int update_timer(struct controller *ptr);
C_API int get_milliseconds(struct controller *ptr);
C_API int L6474_init(struct controller *ptr);
C_API int init_ENC28J60(struct controller*ptr);
C_API int init_driver(struct controller* ptr);
C_API controller *create_controller();
C_API void destroy_controller(struct controller *driver);
C_API void update(struct controller* ptr);

#endif
