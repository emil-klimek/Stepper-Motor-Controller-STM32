#include <netinet/in.h>
#include <netinet/udp.h>
#include "app.h" 

int socket_connect(App *cd)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s, j;

	ssize_t len;
	ssize_t nread;
	ssize_t nwrite;
	
	int res;
	char *input = NULL;

	char buf[512];

	char dip[] = { '1', '0', '.', '0', '.', '0', '.', '2', '\0' };
	char dport[] = {  '4', '9', '1', '5', '2', '\0' };

	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Alloc IPv4 or IPv6*/
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_UDP;      /* Udp protocol */

	s = getaddrinfo(dip, dport, &hints, &result);
	if(s != 0)
	{
		sprintf(buf, "getaddrinfo: %s\n", gai_strerror(s));
		
		fputs("could not connect\n",stdout);
		//show_error_msg(buf , GTK_MESSAGE_ERROR, NULL);
		return 1;
	}

	/*getaddrinfo() returns a list of address structures.
	Try each address until we successfully connect(2).
	If socket(2) (or connect(2)) fails, we (close the socket
	and) try then the next address.*/

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
	    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if(sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;   /*Success*/
		
		close(sfd);
	}

	if(rp == NULL || sfd == -1) /*No address succeeded*/
	{
		fprintf(stderr, "Could not connect\n");
		//show_error_msg(  "Could not connect\n"  , GTK_MESSAGE_ERROR, NULL);
		return 1;
	}

	
	freeaddrinfo(result);       /*No longer needed*/

	cd->sfd = sfd;
	cd->quit = false;
	

	return 0;
}

