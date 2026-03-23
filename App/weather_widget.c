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

#include <time.h>

#include "app.h"
#include "widgets.h"



int sock_connect( const char * addr,  int port  )
{
    int              sfd, s;
    size_t           len;
    ssize_t          nread;
    struct addrinfo  hints;
    struct addrinfo  *result, *rp;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP ;          /* Protocol TCP */


    s = getaddrinfo("wttr.in", "80", &hints, &result);
    
    if (s != NULL) {
        return -1;
    	//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        //exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;
    
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */
    
        close(sfd);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL)
	{               /* No address succeeded */   
    	return -1;
    }

    return sfd;
}

struct sock_data
{
	const char *addr;
	int port;
	int ret;
};

int sock_connect1( struct sock_data *ptr )
{
	ptr->ret = sock_connect(ptr->addr, ptr->port);
	return 0;
}

int sock_connect_timeout( const char * addr,  int port, double timeout  )
{
	int thread;
	time_t now;
	
	struct sock_data s = { .addr=addr, .port=port, .ret=-1 };
	now=time(0);
	pthread_create(&thread, NULL, sock_connect1, &s);

	double d;

	while(1)
	{
		d = difftime(time(0), now);
		if( d > timeout )
		{
			pthread_cancel(thread);
			break;
		}
	}
		
	return s.ret;
	
}

int sendrecv(
		int sfd,
		char *in,
		char *out,
		int insize,
		int outsize)
{
    int nread;
    
   if(write_timeout(sfd, 500000))
   {
		return -1;
   } 
    
    if (write(sfd, in, insize) != insize)
	{
        return -1;
    }

    if(read_timeout(sfd, 500000))
    {
    	return -1;
    }

    nread = read(sfd, out, outsize);
    return nread;

	//return -1;
}

char *weather_info(char *str)
{
	char *c = str;
	char *prev_line = str;
	char *line = str;

	int prev_c;

	int number = 0;
	int cf = 0;
	int d;

	while(*c != 0)
	{
		
		if(isdigit(*c))
		{
			number = 1;
		}

		if(strncmp(c, "°", strlen("°")) == 0)
		{
			d = 1;	
		}

		if(number && (*c == 'F' || *c == 'C'))
		{
			cf = 1;
		}

		if(number && cf && d)
		{
			return line;
		}

		if(*c == '\n')
		{
			number = 0;
			cf = 0;
			d = 0;

			prev_line = line;
			line = c+1;
		}


		prev_c = *c;
		++c;
	}
	
	return str;
}








int widget_weather(Tile *tile)
{
	struct weather_widget *w = tile->data;
	int error;
	char buf[200];

	int dark = 0;

	if(
	    tile->app->theme == 0 && tile->app->dark_mode == 1 ||
	    tile->app->theme == 2
	    )
	{
		dark = 1;
	}
	
	if(w->layout == NULL)
	{
		
		//error = FT_New_Face( tile->app->ft_library,
    	        //        "fonts/Arial Unicode.ttf",
    	        //         0,
    	        //         &w->ft_face );

		//if ( error == FT_Err_Unknown_File_Format )
		//{
		//	fputs("... the font file could be opened and read, but it appears"
  		//	"that its font format is unsupported",
		//	stdout);
		//	exit(EXIT_FAILURE);
		//}
		//else if ( error )
		//{
		//	fputs(
		//	"... another error code means that the font file could not"
		//	"... be opened or read, or that it is broken...",
		//	stdout);
		//	exit(EXIT_FAILURE);
		//}
		//
		
		//w->face = cairo_ft_font_face_create_for_ft_face(w->ft_face, 0);
		//cairo_set_font_face(tile->cr, w->face);

		//w->context = pango_context_new();
		w->layout = pango_cairo_create_layout(tile->cr);	
	}

	
	//cairo_select_font_face(
	//		tile->cr,
	//		"Arial",
	//		CAIRO_FONT_SLANT_NORMAL,
	//		CAIRO_FONT_WEIGHT_NORMAL);

	

	//cairo_ft_font_face_create_for_ft_face (ft_face, 0);

	//cairo_ft_font_face_create_for_ft_face();
	//cairo_ft_font_face_create_for_ft_face() 
	// or cairo_ft_font_face_create_for_pattern().) 
	// The resulting font face could then be used with cairo_scaled_font_create() 
	// and cairo_set_scaled_font().

	//cairo_set_font_size(tile->cr, 14);

	//if(dark)
	//{	
	//	cairo_set_source_rgb(tile->cr, 1,1,1);
	//}
	//else
	//{
	//	cairo_set_source_rgb(tile->cr, 0,0,0);
	//}

	cairo_move_to(tile->cr, 0, 50);

	sprintf(buf, "Weather: %s", w->str+18);
	//pango_layout_set_text(w->layout, w->str, -1);
	//pango_cairo_show_layout(tile->cr, w->layout);

	//cairo_show_text(tile->cr, w->str);

	pango_layout_set_text(w->layout, w->str, -1);
	pango_cairo_show_layout(tile->cr,w->layout);


	return 0;

}

char *weather_line(char *str)
{
	char *r = str;
	char *p = str;
	char *l = str;

	while(p[0] != 0 && p[0] != '\n' )
	{
		++p;
	}

	p[0] = 0;

	while (l[0] != 0)
	{
		if(l[0] == ':')
		{
			++l;
			break;
		}

		++l;
	}

	return l;
}

int update_select_widget_window_button_weather(struct Tile *tile)
{
	int sfd, nread;
	char buf[BUF_SIZE];
	char str[BUF_SIZE];
	char l[BUF_SIZE];
	struct weather_widget *w = tile->data;
	time_t now = time(NULL);
	float d = difftime(now,w->tp);

	stpcpy(
		stpcpy(stpcpy(str, "GET /"), w->location), 
		"?format=3 HTTP/1.1\r\nHost: wttr.in\r\nUser-Agent: curl/8.11.1-DEV\r\nAccept: */*\r\n\r\n" );
	
	
	
	if(d > 1.0f)
	{
		if(w->flag == 2)
		{
			if(w->sfd != -1)
			{
				close(w->sfd);
				w->sfd = -1;
				w->flag = 0;
			}
		}
		else if(w->flag == 0)
		{
			if(w->sfd == -1)
			{
				//w->sfd = sock_connect_timeout( "wttr.in",  80, 1.0 );
				w->sfd = sock_connect("wttr.in", 80);
			}
			
			if(w->sfd != -1)
			{
				fputs("request: ",stdout);
				fputs(str,stdout);
				fputs("\n",stdout);
				nread = sendrecv(w->sfd, str, buf, strlen(str), BUF_SIZE);
				if (nread != -1)
    			{
					//fputs(weather_info(func(weather_info(buf))), stdout);
    				
					const char *p = weather_line(weather_info(buf));
					strcpy(l, w->location);
					l[0] = toupper(l[0]);
					sprintf(w->str, "%s: %s", l, p );


					//printf("buf: %s", buf);
					//fputs(w->str,stdout);
					//w->str = weather_info(buf);
					//fputs(w->str, stdout);

					w->flag = 1;
				}
			}

			//fputs("ok",stdout);
		}
		//sprintf(w->str, "%s", "weather test");
		//sfd = sock_connect_timeout( "wttr.in",  80, 1 );
		//printf("sfd: %i", sfd);
		//if (sfd != -1)
    	//{
		//	nread = sendrecv(sfd, str, buf, strlen(str), BUF_SIZE);
    	//	if (nread != -1)
    	//	{
		//		//fputs(weather_info(func(weather_info(buf))), stdout);
    	//		sprintf(w->str, "Warsaw: %s",  func(weather_info(buf)));
		//		//fputs(w->str,stdout);
		//		//w->str = weather_info(buf);
		//		//fputs(w->str, stdout);
		//	}
		//
   		//}



		//printf("Updating weather \n");
		w->tp = now;
	}

	//
	//struct speed_widget *s = tile->data;
	//int new_speed;
	
	//pthread_mutex_lock (&tile->app->smutex);
	//new_speed = tile->app->speed; 
	//pthread_mutex_unlock (&tile->app->smutex);

	//if(new_speed != s->speed)
	//{
	//	gtk_widget_queue_draw(tile->widget);
	//}

	//s->speed = new_speed;
	
	//printf("time distance: %f\n", d);

	//fputs("updated weather button\n",stdout);
	return 0;
}



int destroy_select_widget_window_button_weather(struct Tile *w)
{
	free(w->data);
	w->data = NULL;
	fputs("weather widget removed\n",stdout);
	return 0;
}

int set_location_confirm_clicked(GtkWidget* widget, App*app)
{
	const char *str = gtk_entry_get_text(app->location_entry);

	//fputs("str: ",stdout);
	//fputs(str, stdout);
	//fputs("\n",stdout);

	struct weather_widget *w;

	if(app->selected_weather_widget)
	{
		w = app->selected_weather_widget->data;
		strcpy(w->location, str);
		w->flag = 2;
		//fputs("location: ",stdout);
		//fputs(w->location,stdout);
		//fputs("\n",stdout);
	}

	gtk_widget_hide(GTK_WIDGET(app->set_location_window));
	
}

int select_widget_window_button_weather(GtkWidget *widget, App *app)
{

	//App *app = data;
	struct weather_widget *w;

	if(app->selected_tile->destroy_func)
	{
		app->selected_tile->destroy_func(app->selected_tile);
		app->selected_tile->destroy_func=NULL;
	}

	app->selected_tile->data = malloc(sizeof (struct weather_widget));
	memset(app->selected_tile->data, 0, sizeof(struct weather_widget));


	app->selected_tile->draw_func = &widget_weather ;
	app->selected_tile->update_func = &update_select_widget_window_button_weather;
	app->selected_tile->destroy_func=&destroy_select_widget_window_button_weather;
	
	app->selected_weather_widget = app->selected_tile;
	gtk_widget_show(GTK_WIDGET(app->set_location_window));	

	w = app->selected_tile->data;
	w->sfd = -1;
	
	strcpy(w->location, "warsaw");
	sprintf(w->str, "%s", "No connection");
	
	app->selected_tile=NULL;
	gtk_widget_hide(app->select_widget_window);
}
