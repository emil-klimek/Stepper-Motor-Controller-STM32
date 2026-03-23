#include <time.h>
#include "app.h"
#include "throttle.h"

void set_actual_pos_button_clicked(GtkWidget *widget, App *app)
{
	fputs("set_actual_pos_button_clicked\n",stdout);
	push_cmd(app, "setpos 0");
	app->throttle->state = STATE_CLOSED;

}

int set_target_pos_window_close(GtkWidget *widget, App *app)
{
	gtk_widget_hide(widget);
	return true;
}


void set_target_pos_button_confirm_clicked(GtkWidget *widget, App *app)
{
	const gchar *str = gtk_entry_get_text (app->target_pos_entry);
	int target_pos = atol(str);
	app->throttle->close_pos = target_pos;	
	gtk_widget_hide( app->set_target_pos_window );
	fprintf( stdout, "target_pos: %i\n", target_pos );
}


void set_throttle_full_circle(GtkWidget *widget, App *app)
{
	gtk_widget_show( app->set_throttle_full_circle_window );
}

void set_throttle_rotation_speed(GtkWidget *widget, App *app)
{
	gtk_widget_show( app->set_throttle_rotation_speed_window );
}

void set_full_circle_confirm_clicked(GtkWidget *widget, App *app)
{
	char buf[512];
	app->throttle->close_pos = atol( gtk_entry_get_text( GTK_ENTRY( app->throttle_full_circle_entry ) ) ); 
	sprintf(buf, "%i kroków", app->throttle->close_pos );

	gtk_button_set_label(GTK_BUTTON(app->set_throttle_full_circle_button), buf );

	gtk_widget_hide( app->set_throttle_full_circle_window );
}

void set_rotation_speed_clicked(GtkWidget *widget, App *app)
{
	char buf[512];
	app->throttle->speed = atol( gtk_entry_get_text( GTK_ENTRY( app-> throttle_rotation_speed_entry   ) ) ); 
	sprintf(buf, "%i", app->throttle->speed );

	gtk_button_set_label(GTK_BUTTON(app->set_throttle_rotation_speed_button ), buf );

	gtk_widget_hide( app->set_throttle_rotation_speed_window );
}



void set_target_pos_button_clicked(GtkWidget *widget, App *app)
{
	gtk_widget_show (  app->set_target_pos_window  );
}


int select_date_and_time_window_close(GtkWidget *widget, App *app)
{
	gtk_widget_hide(widget);
	return true;
}

int calendar_close(GtkWidget *widget, App *app)
{
	gtk_widget_hide(widget);
	return true;
}

void select_time_button(GtkWidget *widget, App *app)
{
	gtk_widget_show(app->time_window);
}

void select_date_button(GtkWidget *widget, App *app)
{
	gtk_widget_show(app->calendar_window);
}

void select_date_and_time_ok_button(GtkWidget *widget, App *app)
{
	GtkTreeIter top_level,child;

	const gchar *date;
	const gchar *time;
	const gchar *state;

	date = gtk_button_get_label(GTK_BUTTON(app->date_button));	
	time = gtk_button_get_label(GTK_BUTTON(app->time_button));	
	state = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->throttle_state_combobox));

	if(date == NULL)
	{
		show_error_msg("no date selected\n",GTK_MESSAGE_ERROR, app->main_window);
		return;
	}


	if(time == NULL)
	{
		show_error_msg("no time selected\n",GTK_MESSAGE_ERROR, app->main_window);
		return;
	}

	if(state == NULL)
	{
		show_error_msg("no state selected\n",GTK_MESSAGE_ERROR, app->main_window);
		return;
	}

	gtk_list_store_append(GTK_LIST_STORE(app->liststore), &top_level);
	gtk_list_store_set(
			GTK_LIST_STORE(app->liststore),
			&top_level,
		      	0, date,
			1, time,
			2, state,
			-1);




	gtk_widget_hide(app->select_date_and_time_window);	
}


char *eat_space(const char* str);


char *dd_mm_yyyy_date_parse(
		 int *dd,
		 int *mm,
		 int *yyyy,
		 const char *str)
{
	str = eat_space(str);
	*dd = strtol(str, &str, 10);
	
	str = eat_space(str);
	
	if(str[0] != '.') return str;
	
	str = eat_space(str+1);
	*mm = strtol(str, &str, 10);
	if(str[0] != '.') return str;
	
	str = eat_space(str+1);
	*yyyy = strtol(str, &str, 10);
	return eat_space(str+1);
}

char *hh_mm_time_parse(
		 int *hh,
		 int *mm,
		 const char *str)
{
	str = eat_space(str);
	*hh = strtol(str, &str, 10);
	
	str = eat_space(str);
	
	if(str[0] != ':') return str;
	
	str = eat_space(str+1);
	*mm = strtol(str, &str, 10);
	if(str[0] != '.') return str;
	
	return eat_space(str+1);
}

#define cmpstr(a,b) (strncmp(a, (const char*)b, sizeof(a)))


int str_to_throttle_state(const char *str)
{
	if(cmpstr("throttle open", str) == 0)
	{
		return STATE_OPEN;
	}
	else if(cmpstr("throttle closed", str) == 0)
	{
		return STATE_CLOSED;
	}

	return 0;
}


void time_window_ok_clicked(GtkWidget *widget, App *app)
{
	guint hh;
	guint mm;
	char buf[512];

	hh = gtk_spin_button_get_value( app->hour_spin_button  );
	mm = gtk_spin_button_get_value( app->min_spin_button );
	
	if(mm < 10)
	{
		sprintf(buf, "%i:0%i", hh,mm);
	}
	else
	{
		sprintf(buf, "%i:%i", hh,mm);
	}
	gtk_button_set_label(GTK_BUTTON(app->time_button), buf);
	gtk_widget_hide(app->time_window);
}

void calendar_ok_clicked(GtkWidget *widget, App *app)
{
	guint year;
	guint month;
	guint day;

	char buf[512];

	
	gtk_calendar_get_date(app->calendar, &year, &month, &day);
	sprintf(buf, "%i.%i.%i", day,month+1,year);
	
	gtk_button_set_label(GTK_BUTTON(app->date_button), buf);
	gtk_widget_hide(app->calendar_window);
}


void add_schedule_time_button(GtkWidget *widget, App *app)
{
	gtk_widget_show(app->select_date_and_time_window);
	
}

void remove_schedule_time_button(GtkWidget *widget, App *app)
{
	GtkTreeIter iter;
	gboolean valid;
	GtkTreeSelection *selection;
	GList *selected_rows;

	selection = gtk_tree_view_get_selection( app->treeview );

	if(selection == NULL) return;

	selected_rows = gtk_tree_selection_get_selected_rows( selection, NULL );
	
	if(selected_rows == NULL ) return;

	for (  GList *i = selected_rows; i != NULL; i = g_list_next(i)  )
	{
		gtk_tree_model_get_iter( GTK_TREE_MODEL(app->liststore), &iter, i->data );
		gtk_list_store_remove(app->liststore, &iter);
	}

	g_list_free(selected_rows);
}

void remove_schedule_all_button(GtkWidget *widget, App *app)
{
	gtk_list_store_clear(app->liststore);
}

void open_throttle(struct App *app)
{
	fputs("opening throttle\n",stdout);
	char cmd[500];
	if(app->throttle->state != STATE_OPEN)
	{	
		sprintf(cmd, "movefwd steps=%i,speed=%i,stop=true", app->throttle->close_pos, app->throttle->speed);
		parse_command(app, cmd);
		app->throttle->state = STATE_OPEN;
	}
}

void close_throttle(struct App *app)
{
	fputs("closing throttle\n",stdout);
	char cmd[500];
	if(app->throttle->state != STATE_CLOSED)
	{
		sprintf(cmd, "movebwd steps=%i,speed=%i,stop=true", app->throttle->close_pos, app->throttle->speed);
		parse_command(app, cmd);
		app->throttle->state = STATE_CLOSED;
	}
}


void throttle_set_manual_mode(App *app)
{
	app->throttle->mode = MANUAL_MODE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->manual_mode_toggle), true);
}

void throttle_set_auto_mode(App *app)
{
	app->throttle->mode = AUTO_MODE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->auto_mode_toggle), true);
}

void manual_open_clicked(GtkWidget *widget, App *app)
{
	throttle_set_manual_mode(app);
	open_throttle(app);		
}

void manual_close_clicked(GtkWidget *widget, App *app)
{
	throttle_set_manual_mode(app);
	close_throttle(app);
}

void stop(GtkWidget *widget, App* data);


void manual_stop_clicked(GtkWidget *widget, App *app)
{
	fputs("manual_stop_clicked",stdout);
	stop(widget, app);
}



void auto_mode_toggled(GtkWidget *widget, App *app)
{
	if ( gtk_toggle_button_get_active(widget) )
	{
		fputs("auto mode toggled\n",stdout);
		app->throttle->mode = AUTO_MODE;
	}
}	

void manual_mode_toggled(GtkWidget *widget, App *app)
{
	if ( gtk_toggle_button_get_active(widget) )
	{
		fputs("manual mode toggled\n",stdout);
		app->throttle->mode = MANUAL_MODE;
	}
}

void init_throttle(App *app)
{
	char buf[500];
	
	Throttle *throttle = app->throttle;
	throttle->state = STATE_CLOSED;
	throttle->open_pos = 0;
	throttle->close_pos = 3200;
	throttle->mode = MANUAL_MODE;
	throttle_set_manual_mode(app);
	throttle->speed = 5000;

	//full_circle:
	//speed: 5000;
	//int full_circle;
	//int speed;

	
	sprintf( buf, "%i kroków", throttle->close_pos );
	gtk_button_set_label(GTK_BUTTON(app->set_throttle_full_circle_button), buf );
	
	sprintf( buf, "%i", throttle->speed );
	gtk_button_set_label(GTK_BUTTON(app->set_throttle_rotation_speed_button), buf);
}

void throttle(App *app)
{
	time_t now;
	struct tm tm_now;
	int prev_min=-1;
	
	GtkTreeIter iter;
	int valid;
	GValue val;

	const char *str;	
	
	//localtime_r( &now, &tm_now );
	char buf[100];

	int day,month,year,h,m;
	int target_state;

	//strftime(buf, sizeof(buf), "%Y-%m-%d, time is %H:%M", &tm_now);
	//printf("Time is '%s'\n", buf);
	//printf("Date: %i.%i.%i\n", tm_now.tm_wday, tm_now.tm_mon+1, tm_now.tm_year+1900);


	while(true)
	{
		if(app->throttle->mode == AUTO_MODE)
		{
			now = time(NULL);
			localtime_r( &now, &tm_now );

			if(prev_min != tm_now.tm_min)
			{
				//TODO: add locks
				valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(app->liststore), &iter);
				while(valid)
				{
					// ========== 0
			
					memset(&val, 0, sizeof(GValue));
					gtk_tree_model_get_value( 
						GTK_TREE_MODEL(app->liststore),
						&iter,
						0,
						&val);
		
					str = g_value_get_string( &val );		
					if(str)
					{
						dd_mm_yyyy_date_parse(&day, &month, &year, str);
						//printf("date: %i.%i.%i", day,month,year);
					}


					g_value_unset( &val );

					//fputs(" | ",stdout);
					// ========== 1
		
					memset(&val, 0, sizeof(GValue));
					gtk_tree_model_get_value( 
						GTK_TREE_MODEL(app->liststore),
						&iter,
						1,
						&val);
					str = g_value_get_string( &val );		
		

					if(str)
					{
						hh_mm_time_parse(&h, &m, str);
					//printf("time: %i:%i", h, m);
					}

					g_value_unset( &val );

					//fputs(" | ",stdout);
					// ========== 2
					memset(&val, 0, sizeof(GValue));
					gtk_tree_model_get_value( 
						GTK_TREE_MODEL(app->liststore),
						&iter,
						2,
						&val);
					str = g_value_get_string( &val );

					if(str)
					{
						target_state = str_to_throttle_state(str);
						//fputs(str, stdout);
					}

					g_value_unset( &val );
					//fputs( "\n",stdout );

					

					if( day == tm_now.tm_mday &&
		   		   	    month == tm_now.tm_mon+1 &&
		   		   	    year == tm_now.tm_year + 1900)
					{
						if(
							h == tm_now.tm_hour &&
							m == tm_now.tm_min
						)
						{
							fputs("time: OK\n",stdout);
							printf("target_state: %i\n", target_state);

							if(target_state != app->throttle->state)
							{
								if(target_state == STATE_CLOSED)
								{
									fputs("trying to close throttle\n",stdout);
									close_throttle(app);	
								}
						
								if(target_state == STATE_OPEN)
								{
									fputs("trying to open throttle\n",stdout);
									open_throttle(app);
								}
							}
						}
					}


					printf("day: %i, month: %i, year: %i\n", day, month, year);
					printf("tm_now.tm_mday: %i, tm_now.tm_mon+1 : %i, tm_now.tm_year + 1900: %i\n", tm_now.tm_mday, tm_now.tm_mon+1, tm_now.tm_year + 1900);

					printf("hour: %i, min: %i\n", h,m);
					printf("tm_now.tm_hour: %i,  tm_now.tm_min: %i\n",  tm_now.tm_hour,  tm_now.tm_min);


					valid = gtk_tree_model_iter_next( GTK_TREE_MODEL(app->liststore), &iter );
				}


				strftime(buf, sizeof(buf), "%Y-%m-%d, time is %H:%M", &tm_now);
				printf("'%s'\n", buf);
			}

			prev_min = tm_now.tm_min;


		}
	}
}
