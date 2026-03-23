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
#include <unistd.h>

#include "app.h"
#include "stack.h"



void push_back(struct cmd_queue* cq, struct cmd_queue** queue);
void pop_back(struct cmd_queue** queue);
void clear_queue(struct cmd_queue** queue);
int queue_len(struct cmd_queue* q);
cmd_queue *make_queue(char* value);
struct cmd_queue* first_in_queue(struct cmd_queue* queue);


void push_cmd(App *ptr, const char* cmd)
{
	pthread_mutex_lock (&ptr->imutex);
	push_back(make_queue(cmd), &ptr->cq);
	pthread_mutex_unlock (&ptr->imutex);
}

void pop_cmd(App *ptr)
{
	pthread_mutex_lock (&ptr->imutex);
	pop_back(&ptr->cq);
	pthread_mutex_unlock (&ptr->imutex);
}


void get_cmd(App *cd, char* cmd)
{
	pthread_mutex_lock (&cd->imutex);
	struct cmd_queue * last = first_in_queue(cd->cq);        
    
	if(last != NULL)
	{
		strcpy(cmd, last->data);
		printf("cmd: %s\n", cmd);
	}
    
	//pop_back(&cd->cq);
	//strcpy(cmd,cd->cmd);
	pthread_mutex_unlock (&cd->imutex);
}


void get_output(App *cd, char*output, int *nlen)
{
	int len;
	pthread_mutex_lock (&cd->omutex);
	cd->olen = 0;
	pthread_mutex_unlock (&cd->omutex);

	while(true)
 	{
 		pthread_mutex_lock (&cd->omutex);
		*nlen = cd->olen;
		pthread_mutex_unlock (&cd->omutex);

		if(*nlen > 0)
		{	
			//printf("received packet: len: %i\n", *nlen);
			len = *nlen;
			pthread_mutex_lock (&cd->omutex);
			strncpy(output, cd->output, *nlen);
			output[*nlen]='\0';
			pthread_mutex_unlock (&cd->omutex);

			//fputs("data: ", stdout);
			//fwrite(output,1,len,stdout);
			//fputs("\n",stdout);
			break;
		}
		else
		{
        	}
	}
}              

#define MAX_SPEED 10000.0f

int loop(App *ptr);

gboolean label_update_speed(App *app)
{
	GtkEntry*en = app->speed_display;
	GtkProgressBar *pb=app->progress_bar; 
	int speed;

	pthread_mutex_lock (&app->smutex);
	speed = app->speed; 
	pthread_mutex_unlock (&app->smutex);


	char label[100];
	sprintf(label, "%i", speed);
	gtk_entry_set_text(GTK_ENTRY(en), label); 
	gtk_widget_queue_draw (GTK_WIDGET (en));

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb), speed/MAX_SPEED); 
	gtk_widget_queue_draw (GTK_WIDGET (pb));


	return TRUE;
}


gboolean label_update_position(App *app)
{
	GtkEntry *lb = app->actual_pos_label;
	char label[100];
	int position;
	pthread_mutex_lock (&app->pmutex);
	position = app->position;
	pthread_mutex_unlock (&app->pmutex);


	
	sprintf(label, "Aktualna pozycja: %.0f %%", (float)position / app->throttle->close_pos * 100);
	gtk_label_set_text(GTK_LABEL(lb), label); 

	return TRUE;
}


gboolean connection_check(App *app)
{
	fputs("checking connection: \n",stdout);
	
	if ( app->connection_ok == 0 )
	{
		fputs("conection ended\n",stdout);
		fputs("now trying to reconnect\n",stdout);
	
		if(app->thread != 0)
		{
			fputs("waiting for thread to end\n",stdout);
			pthread_join(app->thread, NULL);
			fputs("ok1\n",stdout);
		}
		
		if( socket_connect(app) == 0)
		{
		//	fputs("connection succesfull\n",stdout);	
			pthread_create(&app->thread, NULL, &loop, app);
			//close(app->sfd);
			fputs("connection successfull\n",stdout);
		}
		
	}
	
	app->connection_ok = 0;

	return TRUE;
}

void button_clicked(GtkWidget *widget,
                        App* data)
{
	
}

int about_dialog_close(GtkWidget *widget, App *app)
{
	gtk_widget_hide(widget);
	return true;
}

void about_clicked(GtkWidget *widget,
                        App *data)
{
	
}

int change_theme_window_close(GtkWidget *widget,
                        App *app)
{
	//....
	
	
	
	gtk_widget_hide(widget);
	return true;
}


extern const char *themes[]; 

void adwaita_theme_toggled(GtkWidget *widget,
                        App *app)
{
	if ( gtk_toggle_button_get_active(widget) )
	{
		fputs("Adwaita theme selected\n",stdout);
		app->theme = 0;
		g_object_set (gtk_settings_get_default (),
			"gtk-theme-name", themes[ app->theme  ],
			"gtk-application-prefer-dark-theme", app->dark_mode ,
                NULL);

	}
}


void high_contrast_theme_toggled( GtkWidget *widget,
                        App *app )
{
	if ( gtk_toggle_button_get_active(widget) )
	{
		fputs("High Contrast Theme selected\n",stdout);
		app->theme = 1;
		g_object_set (gtk_settings_get_default (),
			"gtk-theme-name", themes[ app->theme  ],
			"gtk-application-prefer-dark-theme", app->dark_mode ,
                NULL);
	}
}

void high_contrast_inverse_theme_toggled( GtkWidget *widget,
                        App *app )
{
	if ( gtk_toggle_button_get_active(widget) )
	{
		fputs("High Contrast Inverse theme selected\n",stdout);
		app->theme = 2;
		g_object_set (gtk_settings_get_default (),
			"gtk-theme-name", themes[ app->theme  ],
			"gtk-application-prefer-dark-theme", app->dark_mode ,
                NULL);
	}
}


void light_mode_toggled( GtkWidget *widget,
                        App *app )
{
	if ( gtk_toggle_button_get_active(widget) )
	{
		fputs("Light mode selected\n",stdout);
		app->dark_mode=0;
		g_object_set (gtk_settings_get_default (),
			"gtk-theme-name", themes[ app->theme  ],
			"gtk-application-prefer-dark-theme", app->dark_mode ,
                NULL);

	}
}

void dark_mode_toggled( GtkWidget *widget,
                        App *app )
{
	if ( gtk_toggle_button_get_active(widget) )
	{
		fputs("Dark mode selected\n",stdout);
		app->dark_mode=1;
		g_object_set (gtk_settings_get_default (),
			"gtk-theme-name", themes[ app->theme  ],
			"gtk-application-prefer-dark-theme", app->dark_mode ,
                NULL);
	}
}

void change_theme(GtkWidget *widget,
                        App *app)
{
	gtk_window_present (GTK_WINDOW (app->change_theme_window));
}

void quit_clicked(GtkWidget *widget,
                        App *data)
{
	gtk_main_quit();
}

void on_window_destroy(GtkWidget *widget,
                       App *data)
{
		
}

void show_error_msg(const char* str, int message_type, GtkWindow *window)
{

	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
 	GtkDialog *dialog = gtk_message_dialog_new (window,
                                  flags,
                                  message_type,
                                  GTK_BUTTONS_CLOSE,
                                  str,
                                  " ",
                                  " ");
 		
	gtk_dialog_run (GTK_DIALOG (dialog));
 	gtk_widget_destroy (dialog);

}


void add_label(GtkWidget *widget,
                       App *data)
{

	int steps, speed;
	char cmd[1000];
	cmd[0] = 0;
	gchar *str = gtk_combo_box_text_get_active_text(data->cmd_combobox);
	if(str == NULL)
	{
		show_error_msg("You need to specify command",GTK_MESSAGE_ERROR,data->main_window);
	}
	else
	{
		stpcpy(&cmd[strlen(cmd)], str);

		str = gtk_entry_get_text (data->steps_entry);
		if(str==NULL || !isdigit(str[0]))
		{
			show_error_msg("You need to specify steps number",GTK_MESSAGE_ERROR, data->main_window);
			return;
		}		

		stpcpy(stpcpy(&cmd[strlen(cmd)], " steps="),str);


		str = gtk_entry_get_text (data->speed_entry);
		if(str==NULL || !isdigit(str[0]))
		{
			show_error_msg("You need to specify speed number",GTK_MESSAGE_ERROR, data->main_window);
			return;
		}		

		stpcpy(stpcpy(&cmd[strlen(cmd)], ",speed="),str);


		//printf("steps: %i", steps);
		
		//const gchar* str = gtk_entry_get_text (data->speed_entry);

		
		GtkWidget *child_widget = gtk_label_new_with_mnemonic (cmd);
	
		GtkWidget *listbox_row = gtk_list_box_row_new();
		gtk_container_add (GTK_CONTAINER (data->listbox),
                 	  listbox_row);	
		gtk_container_add (GTK_CONTAINER (listbox_row),
                   	child_widget);	
	
		gtk_widget_show_all(data->listbox);
	}

}


void __remove_label(GtkWidget* widget, App* data)
{
	gtk_container_remove(data->listbox,
                             widget);
}

typedef struct CallbackTmp
{
	int a;
	int b;
	GtkWidget *w;
} CallbackTmp;

void find_n_label(GtkWidget* widget, CallbackTmp* ptr)
{
	if(ptr->b++ == ptr->a)
	{
		ptr->w = widget;
	}
}

int listbox_n_rows(GtkListBox *widget)
{
	int len = 0;
	while(gtk_list_box_get_row_at_index(widget,len) != NULL) len++;
	return len;
}

int row_pos(GtkListBox *widget, GtkListBoxRow* row)
{
	int i = -1;
	GtkListBoxRow* w;
	do
	{
		w = gtk_list_box_get_row_at_index(widget,++i);
		if(w == row)
			return i;
	}
	while(w);

	return -1;

}


GtkWidget* row_get_child(GtkListBox *listbox, GtkListBoxRow* row)
{
	CallbackTmp tmp;
	tmp.a = row_pos(listbox, row);

	if(tmp.a == -1)
	{
		return NULL;
	}

	tmp.b = 0;
	tmp.w = NULL;
	gtk_container_forall(listbox, find_n_label,  &tmp);
	
	return tmp.w;
}

void remove_label(GtkWidget* widget, App* data)
{
	GtkListBoxRow * row = gtk_list_box_get_selected_row(data->listbox);
	GtkWidget *w;

	if(row != NULL)
	{
		w = row_get_child(data->listbox,row);
		if(w != NULL)
		{
			__remove_label(w, data);
		}
	}
}



void clear_labels(GtkWidget *widget,
                       App *data)
{
	GList* list = gtk_container_get_children(data->listbox);
	if(list != NULL)
	{
		gtk_container_forall(data->listbox,
                                     G_CALLBACK(__remove_label),
                                     data);
	}

	g_list_free(list);

}

#define cmpstr(a,b) (strncmp(a, (const char*)b, strlen(a)))

char *eat_space(const char* str)
{
	while(str[0] == ' ' || str[0] == '\t')
	{
		++str;
	}

	return str;
}


void set_speed_in(const char *str,App* data)
{
    const char * text = "minspeed ";
    size_t a = strlen(str);
    size_t b = strlen(text);
    char * cmd = malloc(a + b + 1);
    stpcpy(stpcpy(cmd, text), str);
    push_cmd(data, cmd);
	free(cmd);
}

void set_microstep(GtkWidget *widget, App* data)
{
	const char * str = gtk_entry_get_text (data->entry);
    const char * text = "microstep ";
    size_t a = strlen(str);
    size_t b = strlen(text);
    char * cmd = malloc(a + b + 1);
    stpcpy(stpcpy(cmd, text), str);
    push_cmd(data, cmd);
	free(cmd);
}

void set_direction_in(const char* str, App* data)
{
    char output[OUTPUT_SIZE];
    const char * text = "direction ";
    size_t a = strlen(str);
    size_t b = strlen(text);
    char * cmd = malloc(a + b + 1);         
    stpcpy(stpcpy(cmd, text), str);
    push_cmd(data, cmd);
    free(cmd);
}

void set_acceleration_in(const char* str, App* data)
{
    char output[OUTPUT_SIZE];
    const char * text = "acceleration ";
    size_t a = strlen(str);
    size_t b = strlen(text);
    char * cmd = malloc(a + b + 1);         
    stpcpy(stpcpy(cmd, text), str);
    push_cmd(data, cmd);
    free(cmd);
}

void set_deceleration_in(const char* str, App* data)
{
    char output[OUTPUT_SIZE];
    const char * text = "deceleration ";
    size_t a = strlen(str);
    size_t b = strlen(text);
    char * cmd = malloc(a + b + 1);         
    stpcpy(stpcpy(cmd, text), str);
    push_cmd(data, cmd);
    free(cmd);
}

void move(GtkWidget *widget,
                        App* data)
{
    char output[OUTPUT_SIZE];
    int len;
    push_cmd(data, "move");
    //get_output(data, output, &len);
}

void parse_command( App* app, const char *str )
{
	const char *p;
	char direction[10];
	char steps[21];
	char speed[21];
	char cmd[64];
	int stop=true;
	
	for(const gchar *iter = str; iter[0] != '\0';)
	{
		iter = eat_space(iter);
	
		p = "movefwd";
		if(cmpstr(p, iter) == 0)
		{
			iter += strlen(p);
			strcpy(direction, "fwd");
			continue;
		}
	
		
		p = "movebwd";
		if(cmpstr(p, iter) == 0)
		{
			iter += strlen(p);
			strcpy(direction, "bwd");
			continue;
		}
	
	
		p = "steps";
		if(cmpstr(p, iter) == 0)
		{
			iter += strlen(p);
			iter = eat_space(iter);
			
			if(iter[0] == '=')
			{
				++iter;
			}
			iter = eat_space(iter);
			int i = 0;
			for(; i<20 && isdigit(iter[0]); ++i, ++iter)
			{
				steps[i] = iter[0];
			}
	
			steps[i] = '\0';
			if(iter[0] == ',')
			{
				++iter;
			}
			continue;
		}
	
		p = "speed";
		if(cmpstr(p, iter) == 0)
		{
			iter += strlen(p);
			iter = eat_space(iter);
			
			if(iter[0] == '=')
			{
				++iter;
			}
			iter = eat_space(iter);
			int i = 0;
			for(; i<20 && isdigit(iter[0]); ++i, ++iter)
			{
				speed[i] = iter[0];
			}
	
			speed[i] = '\0';
			if(iter[0] == ',')
			{
				++iter;
			}
			continue;
		}
	
		p = "stop";
		if(cmpstr(p, iter) == 0)
		{
			iter += strlen(p);
			iter = eat_space(iter);
			
			if(iter[0] == '=')
			{
				++iter;
			}
			iter = eat_space(iter);
			
			p="true";
			if(cmpstr(p, iter) == 0)
			{
				stop=true;
				iter=iter+strlen(p);
			}
	
			p="false";
			if(cmpstr(p, iter) == 0)
			{
				stop=false;
				iter=iter+strlen(p);
			}
	
			iter = eat_space(iter);
			if(iter[0] == ',')
			{
				++iter;
			}
			continue;
		}
		++iter;
	
	}

	//fprintf(stdout,"move speed: %s\n",speed);

	set_direction_in(direction,app);
	set_speed_in(speed,app);
	strcpy(stpcpy(cmd, "movesteps "), steps);
	push_cmd(app, cmd);

	//fputs("cmd: ", stderr);
	//fputs(cmd, stderr);
	//fputs("\n",stderr);

	if(stop)
	{
		push_cmd(app, "stop");	
	}
	//else
	//{
	//	push_cmd(app, "nop");	
	//}
}

void label(GtkWidget* w, App* app)
{		
	const gchar *str = gtk_label_get_text(w);
	parse_command( app, str );
}

void run(GtkWidget *widget,
                       App *data)
{
	int n = listbox_n_rows(data->listbox);
	GtkListBoxRow* row = NULL;
	GtkWidget *w = NULL;

	const gchar *str;

	//for(int i = 0; i < n; ++i)
	//int i = n;
	for(int i = 0; i<n; ++i)
	{
		//printf("i: %i\n",i);
		row = gtk_list_box_get_row_at_index(data->listbox,i);
		if(row != NULL)
		{
			w = row_get_child(data->listbox, row);
		}

		if(w != NULL)
		{
			gtk_container_forall(w,      //container
                                     	    G_CALLBACK(label),     //callback
                                            data);  
		
			//__remove_label(w,data);
		}


	}
	
	
	gtk_container_forall(data->listbox,__remove_label, data);


	push_cmd(data,"stop");
}

void cancel(GtkWidget *widget,
                       App *data)
{

	struct cmd_queue *c = data->cq;
	pthread_mutex_lock (&data->imutex);
	clear_queue(&data->cq);
	pthread_mutex_unlock (&data->imutex);
}



void on_destroy (GtkApplication *app, App* data)
{
	data->quit = true;
	//pthread_join(data->thread, NULL);
	usleep(1000);
	close(data->sfd);
	exit(0);
}




void start(GtkWidget *widget,
                        App* data)
{
	move(widget,data); 
}

void stop(GtkWidget *widget,
                        App* data)
{
	//char output[OUTPUT_SIZE];
	//int len;
 	push_cmd(data, "stop");
	//get_output(data, output, &len);
}




void set_speed(GtkWidget *widget,
                        App* data)
{
	char output[OUTPUT_SIZE];
	int len;
 	const char * str = gtk_entry_get_text (data->entry);
 	set_speed_in(str,data);
	//get_output(data, output, &len);
}

void set_acceleration(GtkWidget *widget,
                        App* data)
{
	char output[OUTPUT_SIZE];
	int len;
	const char * str = gtk_entry_get_text (data->entry);
	set_acceleration_in(str,data);
	//get_output(data, output, &len);
}

void set_deceleration(GtkWidget *widget,
                        App* data)
{
	char output[OUTPUT_SIZE];
	int len;
	const char * str = gtk_entry_get_text (data->entry);
	set_deceleration_in(str,data);
	//get_output(data, output, &len);
}

void set_direction(GtkWidget *widget,
                        App* data)
{
	char output[OUTPUT_SIZE];
	int len;
	char *str = gtk_combo_box_text_get_active_text(data->direction_combobox);
	for(int i = 0; str[i] != '\0'; ++i) str[i] = tolower(str[i]);
	set_direction_in(str, data);
	//get_output(data, output, &len);
}

int validate_line(char *str, App *app)
{
	const char *p;
	char direction[10];
	char steps[21] = {0};
	char speed[21] = {0};
	char cmd[64] = {0};
	
	for(const gchar *iter = str; iter[0] != '\0';)
	{
		iter = eat_space(iter);

		p = "movefwd";
		if(cmpstr(p, iter) == 0)
		{
	    		iter += strlen(p);
	    		strcpy(direction, "fwd");
	    		continue;
		}

		p = "movebwd";
		if(cmpstr(p, iter) == 0)
		{
	    		iter += strlen(p);
	    		strcpy(direction, "bwd");
	    		continue;
		}

		p = "steps";
		if(cmpstr(p, iter) == 0)
		{
	    	iter += strlen(p);
	    	iter = eat_space(iter);
			
	    	if(iter[0] == '=')
	    	{
				++iter;
	   		}
		
			iter = eat_space(iter);
			int i = 0;
			for(; i<20 && isdigit(iter[0]); ++i, ++iter)
			{
				steps[i] = iter[0];
			}

			steps[i] = '\0';
			if(iter[0] == ',')
			{
				++iter;
			}
			continue;
		}


		p = "speed";
		if(cmpstr(p, iter) == 0)
		{
			iter += strlen(p);
			iter = eat_space(iter);
			
			if(iter[0] == '=')
			{
				++iter;
			}
			
			iter = eat_space(iter);
			int i = 0;
			for(; i<20 && isdigit(iter[0]); ++i, ++iter)
			{
				speed[i] = iter[0];
			}

			speed[i] = '\0';
			if(iter[0] == ',')
			{
				++iter;
			}
			
			continue;
		}

		p = "stop";
		if(cmpstr(p, iter) == 0)
		{
			iter += strlen(p);
			iter = eat_space(iter);
			
			if(iter[0] == '=')
			{
				++iter;
			}
			iter = eat_space(iter);
			
			p="true";
			if(cmpstr(p, iter) == 0)
			{
				iter=iter+strlen(p);
			}

			p="false";
			if(cmpstr(p, iter) == 0)
			{
				iter=iter+strlen(p);
			}

			iter = eat_space(iter);
			if(iter[0] == ',')
			{
				++iter;
			}
			continue;
		}


		++iter;
	}

	if(cmpstr(direction,"fwd") != 0)
	{
		if(cmpstr(direction, "bwd") != 0)
		{
			show_error_msg("invalid command: expected movefwd or movefwd\n",GTK_MESSAGE_ERROR, app->main_window);
			return 1;
		}
	}

	if(strlen(steps) == 0)
	{
		show_error_msg("invalid command: no steps number\n",GTK_MESSAGE_ERROR, app->main_window);
		return 1;
	}

	if(strlen(speed) == 0)
	{
		show_error_msg("invalid command: no speed number\n",GTK_MESSAGE_ERROR, app->main_window);
		return 1;
	}

	return 0;
}

int validate_file_format(FILE *file, App *app)
{
	char *line = NULL;
	size_t linecap;
	ssize_t linelen;
	while((linelen = getline(&line, &linecap, file)) > 0)
	{
		if(linelen > 0)
		{
			if(validate_line(line, app) != 0)
			{
				return 1;
			}
		}
	}
	return 0;
}

void read_file(FILE *file, App* app)
{
	char *line = NULL;
	size_t linecap;
	ssize_t linelen; //length without newline
	char cmd[1000];

	clear_labels(app->listbox, app);

	while((linelen = getline(&line, &linecap, file)) > 0)
	{
		if(linelen > 0)
		{
			*stpncpy(cmd, line, linelen-1) = '\0'; 
			//fprintf(stdout, "cmd: %s\n", cmd);
			GtkWidget *child_widget = gtk_label_new_with_mnemonic (cmd);
		
			GtkWidget *listbox_row = gtk_list_box_row_new();
			gtk_container_add (GTK_CONTAINER (app->listbox),
                 	  	listbox_row);	
			gtk_container_add (GTK_CONTAINER (listbox_row),
                   		child_widget);	
		
		}
	}


	gtk_widget_show_all(app->listbox);

}

void open_file(GtkWidget *widget,
                        App* data)
{
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;
	gchar *filename;
	FILE *file = NULL;
	

	fputs("file chooser\n",stdout);
	dialog = gtk_file_chooser_dialog_new("Open File",
					     data->main_window,
					     action,
					     "_Cancel",
					     GTK_RESPONSE_CANCEL,
					     "_Open",
					     GTK_RESPONSE_ACCEPT,
					     NULL);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		file = fopen(filename, "r");
		g_free(filename);
	}

	gtk_widget_destroy(dialog);

	if(file == NULL)
	{
		show_error_msg("can't open file", GTK_MESSAGE_ERROR, data->main_window);	
	}
	else
	{

		if(validate_file_format(file, data) == 0)
		{
			rewind(file);
			read_file(file,data);
		}
		fclose(file);
	}


}



void write_line_to_file(GtkWidget* w, FILE *file)
{
	const gchar *str = gtk_label_get_text(w);
	fputs(str, file);
	fputs("\n",file);
}

void write_to_file(FILE *file, GtkListBox *listbox)
{
	int n = listbox_n_rows(listbox);
	GtkListBoxRow* row = NULL;
	GtkWidget *w = NULL;

	for(int i = 0; i<n; ++i)
	{
		row = gtk_list_box_get_row_at_index(listbox,i);
		if(row != NULL)
		{
			w = row_get_child(listbox, row);
		}

		if(w != NULL)
		{
			gtk_container_forall(w,      //container
                                    	    G_CALLBACK(write_line_to_file),     //callback
                                            file);  
		}


	}
	
	
}

void save_file(GtkWidget *widget,
                        App* data)
{
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	gint res;
	gchar *filename;
	FILE *file = NULL;
	

	fputs("file chooser\n",stdout);
	dialog = gtk_file_chooser_dialog_new("Open File",
					     data->main_window,
					     action,
					     "_Cancel",
					     GTK_RESPONSE_CANCEL,
					     "_Save",
					     GTK_RESPONSE_ACCEPT,
					     NULL);
	
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		file = fopen(filename, "w");
		g_free(filename);
	}
	
	gtk_widget_destroy(dialog);
	
	if(file == NULL)
	{
		show_error_msg("can't open file", GTK_MESSAGE_ERROR, data->main_window);	
	}
	else
	{
		write_to_file(file, data->listbox);
		fclose(file);
	}

	

}

void scale_moved(GtkWidget *widget, App *app)
{
	char buf[100];
	//GtkPositionType = gtk_scale_get_value_pos(app->scale);


	double val = gtk_adjustment_get_value(app->adjustment);
	sprintf(buf, "%i", (int)val);
	
	if(val != app->speed)
	{
		set_speed_in(buf,app);
	}
	//printf("val: %f\n", val);
	//fputs("ok\n",stdout);
}

void main_window_resize (GtkWindow* window, App *app)
{
	gint w_width, w_height, s_width, s_height, marigin_end;
	gtk_window_get_size(window, &w_width, &w_height);
	GtkAllocation *alloc;

	alloc = g_new(GtkAllocation, 1);
	gtk_widget_get_allocation(app->main_window_stackswitcher, alloc);
	s_width = alloc->width;
	s_height = alloc->height;

	g_free(alloc);

	marigin_end = w_width / 2 - s_width;
        marigin_end = marigin_end * (marigin_end > 0);

	gtk_widget_set_margin_end ( app->main_window_stackswitcher, marigin_end );

}

void show_window(GtkWidget *widget, App *app)
{
	gtk_widget_show(app->debug_window);
}

int close_add_widget_window(GtkWidget *widget, App *app)
{
	gtk_widget_hide(widget);
	return true;
}

int debug_window_close(GtkWidget *widget, App *app)
{
	gtk_widget_hide(widget);
	return true;
}



void show_credits(GtkWidget *widget, App *app)
{
	gtk_widget_show(app->about_dialog);
}
