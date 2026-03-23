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
#include "widgets.h"


gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data);



void show_add_widget_window(GtkWidget *widget,
                        Tile *tile);


void show_add_widget_window(GtkWidget *widget,
                        Tile *tile)
{
	gtk_widget_show(tile->app->select_widget_window);
	tile->app->selected_tile = tile;
}

gboolean update_tile (Tile *tile)
{
	if( tile->update_func )
	{
		tile->update_func( tile );
	}

	return true;
}

static GtkWidget *button_new (Tile *tile, App *app)
{
	GtkWidget *button, *area;

	button = gtk_button_new ();
	area = gtk_drawing_area_new ();

	g_signal_connect(G_OBJECT(area), "draw", G_CALLBACK(draw_callback), tile );
	
	g_signal_connect(GTK_BUTTON(button), "clicked", G_CALLBACK(show_add_widget_window), tile );
	
	gtk_container_add( GTK_CONTAINER( button  ), GTK_WIDGET(area) );

	g_idle_add( (GSourceFunc)&update_tile, tile );

	//clicked connected
	return button;
}


gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	Tile *tile = data;
	tile->widget = widget;
	tile->cr = cr;
	if (tile->draw_func != NULL)
	{
		return tile->draw_func(tile);
	}

	return false;
}

int
widget_draw_callback (Tile *tile)
{
	const char *color_name = "blue";
	GdkRGBA rgba;
	if (gdk_rgba_parse (&rgba, color_name))
	{
		gdk_cairo_set_source_rgba (tile->cr, &rgba);
		cairo_paint (tile->cr);
	}


	return 0;
}



static GtkWidget *select_widget_window_button_new(
		const char *text,	
		int (*click_callback)(GtkWidget *widget, gpointer data),
		App *app)
{
	GtkWidget *button;

	button = gtk_button_new ();
	gtk_button_set_label(button, text);

	if(click_callback != NULL)
		g_signal_connect(GTK_BUTTON(button), "clicked", G_CALLBACK( click_callback ), app );

	

	return button;
}

int widget_engine_speed (Tile *tile);



int widget_throttle_state (Tile *tile)
{
	char buf[200];

	int dark = 0;

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

	sprintf(buf, "Throtle state: ");

	if(tile->app->throttle->state == STATE_OPEN)
	{
		strcpy(buf+strlen(buf), "open");
	}
	else if(tile->app->throttle->state == STATE_CLOSED)
	{
		strcpy(buf+strlen(buf), "closed");
	}
	
	cairo_show_text(tile->cr, buf);



	return 0;
}

int widget_engine_pos (Tile *tile)
{
	char buf[200];

	int position;
	int dark = 0;

	pthread_mutex_lock (&tile->app->pmutex);
	position = tile->app->position;
	pthread_mutex_unlock (&tile->app->pmutex);

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

	sprintf(buf, "Engine pos: %i", position);
	cairo_show_text(tile->cr, buf);



	return 0;
}

int widget_weather(Tile *tile);


int widget_draw_blue_background (Tile *tile)
{
	const char *color_name = "red";
	GdkRGBA rgba;
	if (gdk_rgba_parse (&rgba, color_name))
	{
		gdk_cairo_set_source_rgba (tile->cr, &rgba);
		cairo_paint (tile->cr);
	}


	return 0;
}




int widget_draw_red_background1 (Tile *tile)
{
	const char *color_name = "red";
	GdkRGBA rgba;
	if (gdk_rgba_parse (&rgba, color_name))
	{
		gdk_cairo_set_source_rgba (tile->cr, &rgba);
		cairo_paint (tile->cr);
	}


	return 0;
}



int widget_draw_green_background1 (Tile *tile)
{
	const char *color_name = "green";
	GdkRGBA rgba;
	if (gdk_rgba_parse (&rgba, color_name))
	{
		gdk_cairo_set_source_rgba (tile->cr, &rgba);
		cairo_paint (tile->cr);
	}


	return 0;
}



int select_widget_window_button_widget_none(GtkWidget *widget, gpointer data)
{
	App *app = data;
	
	if(app->selected_tile->destroy_func)
	{
		app->selected_tile->destroy_func(app->selected_tile);
		app->selected_tile->destroy_func=NULL;
	}

	app->selected_tile->draw_func = NULL;
	app->selected_tile->update_func = NULL;
	app->selected_tile=NULL;
		

	gtk_widget_hide(app->select_widget_window);
}

int select_widget_window_button_engine_speed(GtkWidget *widget, gpointer data);


int select_widget_window_button_throttle_state(GtkWidget *widget, gpointer data)
{
	App *app = data;
	
	if(app->selected_tile->destroy_func)
	{
		app->selected_tile->destroy_func(app->selected_tile);
		app->selected_tile->destroy_func=NULL;
	}

	app->selected_tile->draw_func = &widget_throttle_state ;
	app->selected_tile->update_func = NULL;
	app->selected_tile=NULL;
	gtk_widget_hide(app->select_widget_window);
}

int select_widget_engine_pos(GtkWidget *widget, gpointer data)
{
	App *app = data;
	
	if(app->selected_tile->destroy_func)
	{
		app->selected_tile->destroy_func(app->selected_tile);
		app->selected_tile->destroy_func=NULL;
	}

	app->selected_tile->draw_func = &widget_engine_pos ;
	app->selected_tile->update_func = NULL;
	app->selected_tile=NULL;
	gtk_widget_hide(app->select_widget_window);
}

int select_widget_window_button_weather(GtkWidget *widget, gpointer data);


int init_widgets(App*app)
{
	Tile *tile;
	for (int i =0; i < 5; i++)
	{
		for (int j = 0; j < 2; ++j)
		{
			tile = &app->tiles[i][j];
			tile->widget = button_new ( tile, app );
		
			if(j==0)
			{
				gtk_widget_set_margin_start ( tile->widget, 100 );
				gtk_widget_set_margin_end ( tile->widget, 20 );
			}

			if(j == 1)
			{
				gtk_widget_set_margin_start ( tile->widget, 20 );
				gtk_widget_set_margin_end ( tile->widget, 100 );
			}

			tile->app = app;
			
			gtk_flow_box_insert( GTK_FLOW_BOX(app->flowbox), tile->widget, -1);
		}
	}

	
	gtk_widget_show_all(app->flowbox);


	struct select_button
	{
		char *label;
		int (*click_callback)(GtkWidget *widget, gpointer data);
	} buttons[] =
	{
		{ .label = "None", .click_callback = &select_widget_window_button_widget_none  },
		{ .label = "Engine speed", .click_callback = &select_widget_window_button_engine_speed },
		{ .label = "Throttle state", .click_callback = &select_widget_window_button_throttle_state },
		{ .label = "Weather", .click_callback = &select_widget_window_button_weather  },
		{ .label = "Engine position", .click_callback = &select_widget_engine_pos  },
	};

#define NITEMS(A) (sizeof(A) / sizeof(A[0]))

	for (int i = 0; i < NITEMS(buttons); i++)
	{
		gtk_flow_box_insert( 
				GTK_FLOW_BOX(app->select_widget_window_flowbox),
				select_widget_window_button_new (buttons[i].label, buttons[i].click_callback, app),
				-1
				);
	}

	gtk_widget_show_all(app->select_widget_window_flowbox);

	//for (int i = 0; i<9 ; i++)
	//{
	//	gtk_flow_box_insert(GTK_FLOW_BOX(app->flowbox), button_new(colors[i]), -1);
	//}

	
	return 0;
}
