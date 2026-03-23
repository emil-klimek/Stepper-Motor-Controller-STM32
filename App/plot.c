#include <math.h>
#include <stdio.h>

#include "app.h"
#include <gtk/gtk.h>
#include <gtkdatabox_grid.h>
#include <gtkdatabox_lines.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_ruler.h>
#include <gtkdatabox.h>

#define POINTS 200
#define STEPS 50
#define BARS 25
#define MARKER 10

#define FRAME_RATE 20

#define PI 3.14159265359

static gfloat gridVert[]={100.0,300.0,500.0,700.0,900.0,1100.0,1300.0,1500.0,1700.0, 1900.0};
static gfloat gridHoriz[]={-100.0,-80.0,-60.0,-40.0,-20.0,0.0,20.0,40.0,60.0,80.0,100.0};


unsigned int counter;

float freq(float rpm)
{
	return rpm/3600;
}

float ang_velocity(float f)
{
	return 2*PI*f;
}



static gboolean
draw_func (App *app)
{

   gfloat off;
   gchar label[100];
   gint i;
   gint k;
   gfloat r;
   int speed;

   if (!GTK_IS_DATABOX (app->box_plt0))
      return FALSE;

   pthread_mutex_lock (&app->smutex);
   speed = app->speed; 
   pthread_mutex_unlock (&app->smutex);


   app->plt0Y[POINTS-1] = ang_velocity(freq(speed));
   for (i=0; i < POINTS-1; i++)
   {
     	app->plt0Y[i] = app->plt0Y[i+1];
   }
   
   gtk_widget_queue_draw (GTK_WIDGET (app->box_plt0));
   
   return TRUE;
}


void create_plot(GtkWindow *window, 
		 GtkWidget **box_plt,
		 GtkWidget **box,
		 float scale,
		 float **x,
		 float **y,
		 gboolean (*draw_func) (App *app),
		 void *user)
{
  GtkWidget *close_button;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *separator;
  GtkDataboxGraph *graph;
  GdkRGBA color;
  gint i;
  int timeout_id;

  *x = g_new0 (gfloat, POINTS);
  *y = g_new0 (gfloat, POINTS);

  //g_object_set(G_OBJECT(app->box_plt1), "expand", TRUE, NULL);
  //gtk_box_pack_start (GTK_BOX (app->box1), table, TRUE, TRUE, 0);
  //gtk_databox_set_bg_color (GTK_DATABOX (app->box_plt1), "#008");

  for (i = 0; i < POINTS; i++)
  {
     (*x)[i] = i;
     (*y)[i] = scale * sin (i * 2 * G_PI / POINTS);
  }

  gtk_databox_create_box_with_scrollbars_and_rulers (box_plt, &table,
  						      TRUE, TRUE, TRUE, TRUE);

  g_object_set(G_OBJECT(*box_plt), "expand", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (*box), table, TRUE, TRUE, 0);
  gtk_databox_set_bg_color (GTK_DATABOX (*box_plt), "#008");

  color.red = 0;
  color.green = 1;
  color.blue = 0;
  color.alpha = 1;

  //creating graph
  graph = gtk_databox_lines_new (POINTS, *x, *y, &color, 3);
  //adding graph
  gtk_databox_graph_add (GTK_DATABOX (*box_plt), graph);
  //scaling
  gtk_databox_auto_rescale (GTK_DATABOX (*box_plt), 0.1);

  for (i = 0; i < POINTS; i++)
  {
     (*x)[i] = i;
     (*y)[i] = 0;
  }

  color.red = 0;
  color.green = 0;
  color.blue = 1;
  color.alpha = 1;

  //grid

  graph = gtk_databox_grid_array_new (11, 10, gridHoriz, gridVert, &color, 1);
  gtk_databox_graph_add (GTK_DATABOX (*box_plt), graph);

  timeout_id = g_timeout_add (1000 / FRAME_RATE,
               (GSourceFunc) draw_func, user);
  
  gtk_widget_show_all( box[0] );
}

static gboolean
draw_func1 (App *app)
{
   gfloat off;
   gchar label[100];
   gint i;
   gint k;
   gfloat r;
   int speed;
   int direction;

   if (!GTK_IS_DATABOX (app->box_plt0))
      return FALSE;

   pthread_mutex_lock (&app->smutex);
   speed = app->speed; 
   pthread_mutex_unlock (&app->smutex);

   pthread_mutex_lock (&app->dmutex);
   //convert direction to [1,-1]
   direction = app->direction *-2.0f + 3; 
   pthread_mutex_unlock (&app->dmutex);

   app->plt1Y[POINTS-1] = ang_velocity(freq(speed)) * direction;
   for (i=0; i < POINTS-1; i++)
   {
     	app->plt1Y[i] = app->plt1Y[i+1];
   }
   
   gtk_widget_queue_draw (GTK_WIDGET (app->box_plt1));
   
   return TRUE;


}

static gboolean
draw_func2 (App *app)
{
   gfloat off;
   gchar label[100];
   gint i;
   gint k;
   gfloat r;
   float dt;
   float sp;
   int speed;

   if (!GTK_IS_DATABOX (app->box_plt0))
      return FALSE;

   pthread_mutex_lock (&app->smutex);
   speed = app->speed; 
   dt = app->dt / CLOCKS_PER_SEC;
   pthread_mutex_unlock (&app->smutex);

   sp=ang_velocity(freq(speed));
   
   if(dt != 0)
   {
   	app->plt2Y[POINTS-1] = abs(sp - app->sp)/dt;
   }
   app->sp = sp;
   
   for (i=0; i < POINTS-1; i++)
   {
     	app->plt2Y[i] = app->plt2Y[i+1];
   }
   
   gtk_widget_queue_draw (GTK_WIDGET (app->box_plt2));
   
   return TRUE;

}

static gboolean
draw_func3 (App *app)
{
	float *x = NULL;//&app->plt3X;
   	float *y = NULL;//&app->plt3Y;
   
   	float *x1 = NULL;//&app->plt3X;
   	float *y1 = NULL;

	float v;

	int i = 0;
	int speed;
	int direction;

	x=app->plt3X;
  	y=app->plt3Y;

  	x1=app->plt4X;
  	y1=app->plt4Y;

	pthread_mutex_lock (&app->smutex);
   	speed = app->speed; 
   	pthread_mutex_unlock (&app->smutex);

	pthread_mutex_lock (&app->dmutex);
   	direction = app->direction; 
   	pthread_mutex_unlock (&app->dmutex);


#define FULL_CIRCLE 2000



	for (i = 0; i < POINTS; i++)
  	{
     		v=((i) / (float)POINTS)*2-1;

     		x[i] = sin (v*2*PI);
     		y[i] = cos (v*2*PI);
  	
	     	x1[i] = (0.1 * sin (v*2*PI) + 0.7 * sin(app->angle));
     		y1[i] = (0.1 * cos (v*2*PI) + 0.7 * cos(app->angle));
	}

	if(speed != 0)
	{
		app->angle+=(360/20)/360.0f * 2*PI * (-direction * 2 + 3) * (speed / (float)FULL_CIRCLE);
	}

	gtk_widget_queue_draw (GTK_WIDGET (app->box_plt3));

	
	return TRUE;
}


void create_plot_(App* app,
		 GtkWindow *window,
	 	 GtkWidget **box_plt,	
		 GtkWidget **box,
		 float scale,
		 float **x_1,
		 float **y_1,
		 
		 float **x_2,
		 float **y_2,

		 gboolean (*draw_func) (gpointer data))
{

  //create_plot(app->main_window, 
	//	    &app->box_plt3,
	//	    &app->box3,
	//	    20.0f, 
	//	    &app->plt3X,
	//     	    &app->plt3Y,
	//	    draw_func3,
	//	    app);



    
   
   float *x = NULL;//&app->plt3X;
   float *y = NULL;//&app->plt3Y;
   
   float *x1 = NULL;//&app->plt3X;
   float *y1 = NULL;//&app->plt3Y;

   //gboolean (*draw_func) (App *app);
   void *user = app;




  GtkWidget *close_button;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *separator;
  GtkDataboxGraph *graph;
  GdkRGBA color;
  gint i;
  gfloat v;
  int timeout_id;

  //app->plt3X = g_new0 (gfloat, POINTS);
  //app->plt3Y = g_new0 (gfloat, POINTS);

  x = x_1[0] = g_new0 (gfloat, POINTS);
  y = y_1[0] = g_new0 (gfloat, POINTS);

  //app->plt4X = g_new0 (gfloat, POINTS);
  //app->plt4Y = g_new0 (gfloat, POINTS);

  x1 = x_2[0] = g_new0 (gfloat, POINTS);
  y1 = y_2[0] = g_new0 (gfloat, POINTS);


  //x=app->plt3X;
  //y=app->plt3Y;

  //x1=app->plt4X;
  //y1=app->plt4Y;


  //g_object_set(G_OBJECT(app->box_plt1), "expand", TRUE, NULL);
  //gtk_box_pack_start (GTK_BOX (app->box1), table, TRUE, TRUE, 0);
  //gtk_databox_set_bg_color (GTK_DATABOX (app->box_plt1), "#008");

  for (i = 0; i < POINTS; i++)
  {
     v=((i) / (float)POINTS)*2-1;

     x[i] = sin (v*2*PI);
     y[i] = cos (v*2*PI);
  }

  gtk_databox_create_box_with_scrollbars_and_rulers (box_plt, &table,
  						      TRUE, TRUE, TRUE, TRUE);

  g_object_set(G_OBJECT(*box_plt), "expand", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (*box), table, TRUE, TRUE, 0);
  gtk_databox_set_bg_color (GTK_DATABOX (*box_plt), "#008");

  color.red = 0;
  color.green = 1;
  color.blue = 0;
  color.alpha = 1;

  
  //creating graph
  graph = gtk_databox_lines_new (POINTS, x, y, &color, 3);
  //adding graph
  gtk_databox_graph_add (GTK_DATABOX (*box_plt), graph);
  //scaling
  gtk_databox_auto_rescale (GTK_DATABOX (*box_plt), 0.1);


   //scaling
  //gtk_databox_auto_rescale (GTK_DATABOX (*box_plt), 0.1);


  //
  //for (i = 0; i < POINTS; i++)
  //{
  //   (*x)[i] = i;
  //   (*y)[i] = 0;
  //}

  color.red = 0;
  color.green = 0;
  color.blue = 1;
  color.alpha = 1;

  //grid

  graph = gtk_databox_grid_array_new (11, 10, gridHoriz, gridVert, &color, 1);
  gtk_databox_graph_add (GTK_DATABOX (*box_plt), graph);

  for (i = 0; i < POINTS; i++)
  {
     v=((i) / (float)POINTS)*2-1;

     x1[i] = 0.5 * sin (v*2*PI);
     y1[i] = 0.5 * cos (v*2*PI);
  }

  color.red = 0;
  color.green = 1;
  color.blue = 0;
  color.alpha = 1;


  graph = gtk_databox_lines_new (POINTS, x1, y1, &color, 3);
  //adding graph
  gtk_databox_graph_add (GTK_DATABOX (*box_plt), graph);


  timeout_id = g_timeout_add (1000 / FRAME_RATE,
               (GSourceFunc) draw_func, user);
          
  gtk_widget_show_all( box[0] );

}


void create_plots(App *app)
{
	create_plot(app->debug_window, 
		    &app->box_plt0,
		    &app->box0,
		    20.0f,
		    &app->plt0X,
	       &app->plt0Y,
		    draw_func,
		    app);
	
	create_plot(app->main_window, 
		    &app->box_plt1,
		    &app->box1,
		    20.0f, 
		    &app->plt1X,
		    &app->plt1Y,
		    draw_func1,
		    app);

	create_plot(app->main_window, 
		    &app->box_plt2,
		    &app->box2,
		    200.0f,
		    &app->plt2X,
		    &app->plt2Y,
		    draw_func2,
		    app);


	create_plot_(
			app,
			app->debug_window,
			&app->box_plt3,
			&app->box3,
		        20.0f,
			&app->plt3X,
			&app->plt3Y,
			&app->plt4X,
			&app->plt4Y,
			&draw_func3
		    );

	
}
