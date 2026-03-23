#ifndef WIDGETS_H
#define WIDGETS_H

#include <gtk/gtk.h>
#include <freetype/freetype.h>

typedef struct App App;


typedef struct Tile
{
	int visible;
	GtkWidget *widget; 
	cairo_t *cr;
	void *data;

	App *app;

	int (*draw_func)(struct Tile *w);
	int (*update_func)(struct Tile *w);
	int (*destroy_func)(struct Tile *w);
} Tile;

#define BUF_SIZE 500
#define STR_SIZE 300
struct weather_widget
{
	time_t tp;
	int sfd;
	int flag;
	char location[STR_SIZE];
	char str[BUF_SIZE];
	FT_Face ft_face;
	cairo_font_face_t* face;
	cairo_scaled_font_t *scaled_font;
	PangoContext *context;
	PangoLayout *layout;
};

int init_widgets(App*app);


#endif
