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
#include "widgets.h"

struct speed_widget
{
	int speed;
};


int widget_engine_speed (Tile *tile)
{
	char buf[200];

	int dark = 0;

	struct speed_widget *s = tile->data;

	if(
	    tile->app->theme == 0 && tile->app->dark_mode == 1 ||
	    tile->app->theme == 2
	    )
	{
		dark = 1;
	}
	
	cairo_select_font_face(
			tile->cr,
			"monospace",
			CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(tile->cr, 14);

	if(dark)
	{	
		cairo_set_source_rgb(tile->cr, 1,1,1);
	}
	else
	{
		cairo_set_source_rgb(tile->cr, 0,0,0);
	}

	cairo_move_to(tile->cr, 0, 50);

	sprintf(buf, "Engine speed: %i", s->speed);
	cairo_show_text(tile->cr, buf);



	return 0;
}

int update_select_widget_window_button_engine_speed(struct Tile *tile)
{
	struct speed_widget *s = tile->data;
	int new_speed;
	
	pthread_mutex_lock (&tile->app->smutex);
	new_speed = tile->app->speed; 
	pthread_mutex_unlock (&tile->app->smutex);

	if(new_speed != s->speed)
	{
		gtk_widget_queue_draw(tile->widget);
	}

	s->speed = new_speed;
	
	//fputs("updated speed button\n",stdout);
	return 0;
}

int destroy_select_widget_window_button_engine_speed(struct Tile *w)
{
	free(w->data);
	w->data = NULL;
	fputs("widget removed\n",stdout);
	return 0;
}

int select_widget_window_button_engine_speed(GtkWidget *widget, gpointer data)
{
	App *app = data;
	
	if(app->selected_tile->destroy_func)
	{
		app->selected_tile->destroy_func(app->selected_tile);
		app->selected_tile->destroy_func=NULL;
	}

	app->selected_tile->data = malloc(sizeof (struct speed_widget));
	memset(app->selected_tile->data, 0, sizeof(struct speed_widget));

	app->selected_tile->draw_func = &widget_engine_speed;
	app->selected_tile->update_func=&update_select_widget_window_button_engine_speed;
	app->selected_tile->destroy_func=&destroy_select_widget_window_button_engine_speed;
	app->selected_tile=NULL;

	gtk_widget_hide(app->select_widget_window);
}


