#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "widgets.h"

#define BUF_SIZE 500
#define CMD_SIZE BUF_SIZE
#define OUTPUT_SIZE BUF_SIZE
#define FRAME_RATE 50

#define true 1
#define false 0

#include "stack.h"

#include "throttle.h"

#define DIRECTION_FWD 1
#define DIRECTION_BWD 2


typedef struct App
{
	GtkBuilder*  gtkBuilder;

	GtkWidget *box0;
	GtkWidget *box1;
	GtkWidget *box2;
	GtkWidget *box3;	
	GtkWidget *box_plt0;
	GtkWidget *box_plt1;
	GtkWidget *box_plt2;
	GtkWidget *box_plt3;

	gfloat *plt0X;
	gfloat *plt0Y;

	gfloat *plt1X;
	gfloat *plt1Y;

	gfloat *plt2X;
	gfloat *plt2Y;

	gfloat *plt3X;
	gfloat *plt3Y;

	gfloat *plt4X;
	gfloat *plt4Y;

	GtkWidget *button_box;
	GtkWidget *start_button;
	GtkWidget *stop_button;
	GtkWidget *set_speed_button;
	GtkWidget *set_direction_button;
	GtkWidget *direction_combobox;
	GtkWidget *entry;
	GtkWidget *speed_display;
	GtkWidget *steps_entry;
	GtkWidget *speed_entry;
	GtkWidget *scrolled_window;
	GtkWidget *add_label_button;
	GtkWidget *listbox;
	GtkWidget *cmd_combobox;
	GtkWidget *main_window;
	GtkWidget *scale;
	GtkWidget *adjustment;
	GtkWidget *about_dialog;
	GtkWidget *change_theme_window;
	GtkWidget *main_window_headerbar;
	GtkWidget *main_window_stackswitcher;
	GtkWidget *flowbox;
	GtkWidget *select_widget_window_flowbox;
	GtkWidget *select_widget_window;
	GtkWidget *debug_window;
	GtkWidget *progress_bar;
	GtkWidget *actual_pos_label;
	GtkWidget *set_target_pos_window; 
	GtkWidget *target_pos_entry;
	GtkWidget *target_pos_button;
	GtkWidget *treeview;
	GtkWidget *liststore;
	GtkWidget *select_date_and_time_window;
	GtkWidget *calendar;
	GtkWidget *calendar_window;
	GtkWidget *time_window;
	GtkWidget *date_button;
	GtkWidget *time_button;
	GtkWidget *hour_spin_button;
	GtkWidget *min_spin_button;
	GtkWidget *throttle_state_combobox;
	GtkWidget *manual_mode_toggle;
	GtkWidget *auto_mode_toggle;
	GtkWidget *set_throttle_full_circle_window;
	GtkWidget *set_throttle_rotation_speed_window;
	GtkWidget *set_throttle_full_circle_button;
	GtkWidget *set_throttle_rotation_speed_button;
	GtkWidget *throttle_full_circle_entry;
	GtkWidget *throttle_rotation_speed_entry;
	GtkWidget *set_location_window;
	GtkWidget *location_entry;

	GtkImage *preview;

	GtkCssProvider *provider;

	cmd_queue *cq;
	int sfd;
	int quit;
	int speed;
	int direction;
	int position;
	float dt;
	float dv;
	float sp;
	float tp;

	float angle;

	int olen;
	char cmd[BUF_SIZE];
	char output[BUF_SIZE];

	int connection_ok;

	Tile tiles[5][2];
	Tile *selected_tile;
	Tile *selected_weather_widget;


	Throttle *throttle;

	pthread_mutex_t imutex;
	pthread_mutex_t omutex;
	pthread_mutex_t smutex;
	pthread_mutex_t dmutex;
	pthread_mutex_t pmutex;
	
	pthread_t thread;
	pthread_t throttle_thread;

	int dark_mode;
	int theme;

	FT_Library ft_library;
} App;

#endif //APP_H
