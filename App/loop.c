#include <time.h>
#include "app.h"

#define CONNECTION_TIMEOUT 500000

int read_timeout(int sfd, int t)
{
	fd_set set;
	struct timeval timeout;

	FD_ZERO (&set);
	FD_SET(sfd, &set);

	timeout.tv_sec = 0;
	timeout.tv_usec = t;

	switch(select(FD_SETSIZE, &set, NULL, NULL, &timeout))
	{

		case 1:
			return 0;
		case 0:
			fputs("no connection while reading from socket\n",stdout);
			/*fallthrough*/
		case -1:
			return 1;

	}
}

int write_timeout(int sfd, int t)
{
	fd_set set;
	struct timeval timeout;

	FD_ZERO (&set);
	FD_SET(sfd, &set);

	timeout.tv_sec = 0;
	timeout.tv_usec = t;

	switch(select(FD_SETSIZE, NULL, &set, NULL, &timeout))
	{

		case 1:
			return 0;
		case 0:
			fputs("no connection while writing to socket\n",stdout);
			/*fallthrough*/
		case -1:
			return 1;

	}
}

int accept_output(App *ptr)
{
	if(read_timeout(ptr->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}

	pthread_mutex_lock (&ptr->omutex);
	ptr->olen = read(ptr->sfd, ptr->output, BUF_SIZE);

	if(strncmp(ptr->output, "OK", 2) == 0)
	{
		pop_cmd(ptr);
	}

	pthread_mutex_unlock (&ptr->omutex);

	return 0;
}

int send_data(App* app, char *buf)
{
	int len;
	
	
	if(write_timeout(app->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}
	
	len = strlen(buf);

	if(write(app->sfd, buf, len) != len)
        {
           	fprintf(stderr, "partial/failed write\n");
		exit(EXIT_FAILURE);
        }

	return 0;
}

void update_time(App *ptr)
{
	float tp = clock();
	ptr->dt = tp - ptr->tp;
	ptr->tp = tp;
}

int update_speed(App *ptr)
{
	char buf[BUF_SIZE];
	int len;
	
	
	
	
	
	
	
	strcpy(buf, "getminspeed\r\n");
	len=strlen(buf);

	
	if(write_timeout(ptr->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}

	if(write(ptr->sfd, buf, len) != len)
	{
        	fprintf(stderr, "partial/failed write\n");
        	exit(EXIT_FAILURE);
    	}


	if(read_timeout(ptr->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}

	pthread_mutex_lock (&ptr->omutex);
    	//nread = 0;
	
	
	
	
	while((ptr->olen = read(ptr->sfd, ptr->output, BUF_SIZE)) < 0){}
    	//ptr->olen = nread;
                
	pthread_mutex_lock (&ptr->smutex);
	ptr->speed = atol(ptr->output); 
	//fprintf(stdout, "speed: %i\n", ptr->speed);
	
	update_time(ptr);

	pthread_mutex_unlock (&ptr->smutex);
	pthread_mutex_unlock (&ptr->omutex);

	return 0;
}

int update_direction(App *ptr)
{
	char buf[BUF_SIZE];
	int len;
	strcpy(buf, "getdirection\r\n");
	len=strlen(buf);
	
	if(write_timeout(ptr->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}
	
	if(write(ptr->sfd, buf, len) != len)
    	{
        	fprintf(stderr, "partial/failed write\n");
        	exit(EXIT_FAILURE);
	}


	if(read_timeout(ptr->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}

	pthread_mutex_lock (&ptr->omutex);
    	//nread = 0;
	while((ptr->olen = read(ptr->sfd, ptr->output, BUF_SIZE)) < 0){}
    	//ptr->olen = nread;
                
	pthread_mutex_lock (&ptr->dmutex);
	//ptr->speed = atol(ptr->output); 
	
	if(strncmp(ptr->output, "fwd", 3) == 0)
	{
		ptr->direction = DIRECTION_FWD;
	}
	else if(strncmp(ptr->output, "bwd", 3) == 0)
	{
		ptr->direction = DIRECTION_BWD;
	}
	
	//fprintf(stdout, "direction: %s\n", ptr->output);
	pthread_mutex_unlock (&ptr->dmutex);
	pthread_mutex_unlock (&ptr->omutex);

	return 0;
}

int update_position(App *ptr)
{
	char buf[BUF_SIZE];
	int len;
	
	strcpy(buf, "getpos\r\n");
	len=strlen(buf);
	
	if(write_timeout(ptr->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}
	
	if(write(ptr->sfd, buf, len) != len)
	{
        	fprintf(stderr, "partial/failed write\n");
        	exit(EXIT_FAILURE);
    	}

	if(read_timeout(ptr->sfd, CONNECTION_TIMEOUT))
	{
		return 1;
	}

	pthread_mutex_lock (&ptr->omutex);
    	//nread = 0;
	while((ptr->olen = read(ptr->sfd, ptr->output, BUF_SIZE)) < 0){}
    	//ptr->olen = nread;
                
	pthread_mutex_lock (&ptr->pmutex);
	//ptr->speed = atol(ptr->output); 
	
	ptr->position = atoi(ptr->output);
	
	//fprintf(stdout, "position: %s\n", ptr->output);
	pthread_mutex_unlock (&ptr->pmutex);

	pthread_mutex_unlock (&ptr->omutex);
}



int loop(App *ptr)
{
	int len;
	int sfd = ptr->sfd;
	int i = 0;
	int j = 0;
	//int nread;
	char buf[BUF_SIZE];
	//char cmd

	while(ptr->quit == false)
	{
 		//pthread_mutex_lock (&ptr->imutex);
		get_cmd(ptr, buf);

        	len = strlen(buf);
        	/* +1 for terminating null byte*/

        	if(len + 1 > BUF_SIZE)
        	{
            		fprintf(stderr, "Ignoring long message in an argument %d\n", j);
            		continue;
        	}

        	

		if(send_data(ptr, buf))
		{
			fputs("send data error\n",stdout);
			break;
		}
		
		//fputs("write cmd\n",stdout);
        	if(accept_output(ptr))
		{
			fputs("accept output error\n",stdout);
			break;
		}
		//fputs("output ok\n",stdout);
		
		if(update_speed(ptr))
		{
			fputs("update speed error\n",stdout);
			break;
		}
		//fputs("updated speed\n",stdout);
		
		if(update_direction(ptr))
		{
			fputs("update direction error\n",stdout);
			break;
		}
		//fputs("updated direction\n",stdout);
		
		if(update_position(ptr))
		{
			fputs("update position error\n",stdout);
			break;
		}
		//fputs("updated position\n",stdout);

		ptr->connection_ok=1;
		strcpy(buf, "==========");	

        	//if(nread == -1)
        	//{
            	//	perror("read");
            	//	exit(EXIT_FAILURE);
        	//}

    	}

	
	close(sfd);
	
	fputs("thread quit\n",stdout);
	printf("ptr->quit: %i\n", ptr->quit);
	ptr->connection_ok=0;

	return 0;
}

