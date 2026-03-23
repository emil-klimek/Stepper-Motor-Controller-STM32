
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

#include "app.h"
#include "stack.h"



void loop(App *ptr);

void on_destroy (GtkApplication *app, App* data);
void push_cmd(App *cd, const char* cmd);
void pop_cmd(App *cd);


const char *themes[] = {
  { "Adwaita" },
  { "HighContrast" },
  { "HighContrastInverse" }
};


int socket_connect(App *cd);
gboolean label_update_speed(App *app);
gboolean label_update_position(App *app);
gboolean connection_check(App *app);

void create_plots(App *app);



int init_widgets(App*app);

void throttle(App *ptr);


void init_treeview( App *app )
{
	GList *list = gtk_tree_view_get_columns(GTK_TREE_VIEW(app->treeview));
	
	const char * str;
	GtkCellRenderer *renderer;
	int attr = 0;

	for (GList * i = list; i!=NULL; i=g_list_next(i))
	{
		GtkTreeViewColumn *col = i->data;
		str = gtk_tree_view_column_get_title(col);		
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col,renderer,TRUE);
		gtk_tree_view_column_add_attribute(col, renderer, "text", attr++);
		//printf("%s: \n", str);
	}
	
	g_list_free(list);
	return ;
}

int main(int argc, char* argv[], char **envp)
{
	int status = 0;
	int timeout_id;
	GtkWidget *window;
	GtkWidget *window_label;
	GError *error = NULL;
	GtkBuilder*  gtkBuilder;
	gtk_init(&argc, &argv);

	App app;	
	memset(&app, 0, sizeof(App));

	Throttle t;
	app.throttle = &t;
	
	const char * ui_file = "app.ui";

	gtkBuilder = gtk_builder_new();	
	app.gtkBuilder = gtkBuilder;
	status = gtk_builder_add_from_file(gtkBuilder,ui_file,&error);

	gtk_builder_connect_signals(gtkBuilder, &app);
	
	app.main_window = window = gtk_builder_get_object(gtkBuilder, "main_window");
	/*app.window_label = */window_label = gtk_builder_get_object(gtkBuilder, "window_label");
	
	app.box0 = gtk_builder_get_object(gtkBuilder, "page0_box0");
	app.box1 = gtk_builder_get_object(gtkBuilder, "page0_box1");
	app.box2 = gtk_builder_get_object(gtkBuilder, "page0_box2");
	app.box3 =  gtk_builder_get_object(gtkBuilder, "page0_box3");
	app.button_box = gtk_builder_get_object(gtkBuilder, "button_box");
	app.start_button = gtk_builder_get_object(gtkBuilder, "start_button");
	app.stop_button = gtk_builder_get_object(gtkBuilder, "stop_button");
	app.set_speed_button = gtk_builder_get_object(gtkBuilder, "set_speed_button");
	app.set_direction_button = gtk_builder_get_object(gtkBuilder, "window");
	app.direction_combobox = gtk_builder_get_object(gtkBuilder, "direction_combobox");
	app.entry = gtk_builder_get_object(gtkBuilder, "entry");
	app.speed_display = gtk_builder_get_object(gtkBuilder, "speed_display");
	app.steps_entry = gtk_builder_get_object(gtkBuilder, "steps_entry");
	app.speed_entry = gtk_builder_get_object(gtkBuilder, "speed_entry");
	app.scrolled_window = gtk_builder_get_object(gtkBuilder, "scrolled_window");
	app.add_label_button = gtk_builder_get_object(gtkBuilder, "add_label_button");
	app.listbox = gtk_builder_get_object(gtkBuilder, "listbox");
	app.cmd_combobox = gtk_builder_get_object(gtkBuilder, "cmd_combobox");
	app.scale = gtk_builder_get_object(gtkBuilder, "scale");
	app.adjustment = gtk_builder_get_object(gtkBuilder, "adjustment1");
	app.about_dialog = gtk_builder_get_object(gtkBuilder, "about_dialog");
	app.change_theme_window = gtk_builder_get_object(gtkBuilder, "change_theme_window");
	app.main_window_headerbar = gtk_builder_get_object(gtkBuilder, "main_window_headerbar");
	app.main_window_stackswitcher = gtk_builder_get_object(gtkBuilder, "main_window_stackswitcher");
	app.flowbox = gtk_builder_get_object(gtkBuilder, "flowbox");
	app.select_widget_window_flowbox = gtk_builder_get_object(gtkBuilder, "select_widget_window_flowbox");
	app.select_widget_window = gtk_builder_get_object(gtkBuilder, "select_widget_window");
	app.debug_window = gtk_builder_get_object(gtkBuilder, "debug_window");
	app.progress_bar = gtk_builder_get_object(gtkBuilder, "progress_bar");
	app.actual_pos_label = gtk_builder_get_object(gtkBuilder, "actual_pos_label");
	app.set_target_pos_window = gtk_builder_get_object(gtkBuilder, "set_target_pos_window");
	app.target_pos_entry = gtk_builder_get_object(gtkBuilder, "target_pos_entry");
	app.target_pos_button = gtk_builder_get_object(gtkBuilder, "target_pos_button");
	app.treeview = 	gtk_builder_get_object(gtkBuilder, "treeview");
	app.liststore = gtk_builder_get_object(gtkBuilder, "liststore");
	app.select_date_and_time_window = gtk_builder_get_object(gtkBuilder, "select_date_and_time_window");
	app.calendar_window = gtk_builder_get_object(gtkBuilder, "calendar_window");
	app.calendar = gtk_builder_get_object(gtkBuilder, "calendar");
	app.time_window = gtk_builder_get_object(gtkBuilder, "time_window");
	app.date_button = gtk_builder_get_object(gtkBuilder, "date_button");
	app.time_button = gtk_builder_get_object(gtkBuilder, "time_button");
	app.hour_spin_button = gtk_builder_get_object(gtkBuilder, "hour_spin_button");
	app.min_spin_button = gtk_builder_get_object(gtkBuilder, "min_spin_button");
	app.throttle_state_combobox = gtk_builder_get_object(gtkBuilder, "throttle_state_combobox");
	app.manual_mode_toggle = gtk_builder_get_object(gtkBuilder, "manual_mode_toggle");
	app.auto_mode_toggle = gtk_builder_get_object(gtkBuilder, "auto_mode_toggle");
	app.set_throttle_full_circle_window = gtk_builder_get_object(gtkBuilder, "set_throttle_full_circle_window");
	app.set_throttle_rotation_speed_window = gtk_builder_get_object(gtkBuilder, "set_throttle_rotation_speed_window");
	app.set_throttle_full_circle_button = gtk_builder_get_object(gtkBuilder, "set_throttle_full_circle_button");
	app.set_throttle_rotation_speed_button = gtk_builder_get_object(gtkBuilder, "set_throttle_rotation_speed_button");
	app.throttle_full_circle_entry = gtk_builder_get_object(gtkBuilder, "throttle_full_circle_entry");
	app.throttle_rotation_speed_entry = gtk_builder_get_object(gtkBuilder, "throttle_rotation_speed_entry");
	app.set_location_window = gtk_builder_get_object(gtkBuilder, "set_location_window");
	app.location_entry = gtk_builder_get_object(gtkBuilder, "location_entry");

	init_throttle(&app);
	init_treeview(&app);
	
	app.provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_path (app.provider, "style.css", &error);
	gtk_style_context_add_provider_for_screen ( gdk_screen_get_default() ,
                                                  GTK_STYLE_PROVIDER ( app.provider ),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );
	

	g_signal_connect(G_OBJECT(app.main_window), "delete-event", G_CALLBACK(on_destroy), &app);
	timeout_id = g_timeout_add(1000 / FRAME_RATE, (GSourceFunc) label_update_speed, &app);
	timeout_id = g_timeout_add(1000 / FRAME_RATE, (GSourceFunc) label_update_position, &app);
	
	
	app.connection_ok=1;
	timeout_id = g_timeout_add(1000, (GSourceFunc) connection_check, &app);


	
	status = socket_connect(&app);
	if(status != 0)
	{
		return EXIT_FAILURE;
	}
	
	status = pthread_mutex_init (&app.imutex, NULL);
	if(status == -1)
	{
		return EXIT_FAILURE;
	}
    	
	status = pthread_mutex_init (&app.omutex, NULL);
	if(status == -1)
	{
	return EXIT_FAILURE;
	}
	
	status = pthread_mutex_init (&app.smutex, NULL);
	if(status == -1)
	{
		return EXIT_FAILURE;
	}
    
	status = pthread_mutex_init (&app.dmutex, NULL);
	if(status == -1)
	{
		return EXIT_FAILURE;
	}
    
	status = pthread_mutex_init (&app.pmutex, NULL);
	if(status == -1)
	{
		return EXIT_FAILURE;
	}
    
	status = pthread_create(&app.thread, NULL, &loop, &app);
	if(status == -1)
	{
		return EXIT_FAILURE;
	}
    	
	status = pthread_create(&app.throttle_thread, NULL, &throttle, &app);
	if(status == -1)
	{
		return EXIT_FAILURE;
	}


	create_plots(&app);
	init_widgets(&app);
	gtk_widget_show_all(window);
    	
	gtk_main();
    
	pthread_mutex_destroy(&app.pmutex);
	pthread_mutex_destroy(&app.dmutex);
	pthread_mutex_destroy(&app.smutex);
	pthread_mutex_destroy(&app.omutex);
	pthread_mutex_destroy(&app.imutex);
    
	return 0;
}
 
