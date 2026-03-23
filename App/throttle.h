#ifndef THROTTLE_H
#define THROTTLE_H

typedef struct App App;

#define DISABLED 0
#define AUTO_MODE 1
#define MANUAL_MODE 2

#define STATE_CLOSED 0
#define STATE_OPEN 1

typedef struct Throttle
{
	int state;
	int open_pos;
	int close_pos;
	int speed;
	int mode;
} Throttle;

#endif
