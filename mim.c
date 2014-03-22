#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h> //For random number generation
#include "swp.h"

#define SERVER_PORT 8000
#define CLIENT_PORT 8500
#define MIM_SERVER_PORT 8100
#define MIM_CLIENT_PORT 8200

int make_choice(double threshold){
	//With a probability = threshold returns 1
	double sample = rand()/(RAND_MAX + 1.0);
	return (sample > threshold);
}

int main(int argc, char *argv[]) {

	//Sanity check code
	if(argc<4){
		printf("usage: %s <server> <p> <q>\n",argv[0]);
		exit(0);
	}

	double p, q;
	p = atof(argv[2]);
	q = atof(argv[3]);

	srand(time(NULL));
	// Setting up as a client for connecting to server
	
	int sd_cli, rc;
	struct sockaddr_in mim_cliAddr, remoteServAddr;
	struct hostent *h;

	  /* get server IP address (no check if input is IP address or DNS name */
	h = gethostbyname(argv[1]);
	if(h==NULL) {
	  printf("%s: unknown host '%s' \n", argv[0], argv[1]);
	  exit(1);
	}


	remoteServAddr.sin_family = h->h_addrtype;
	memcpy((char *) &remoteServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
	remoteServAddr.sin_port = htons(SERVER_PORT);

	  /* socket creation */
	sd_cli = socket(AF_INET,SOCK_DGRAM,0);
	if(sd_cli<0) {
	printf("%s: cannot open socket \n",argv[0]);
	  exit(1);
	}
	  
	/* bind any port */
	mim_cliAddr.sin_family = AF_INET;
	mim_cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mim_cliAddr.sin_port = htons(MIM_CLIENT_PORT);
	
	rc = bind(sd_cli, (struct sockaddr *) &mim_cliAddr, sizeof(mim_cliAddr));
	if(rc<0) {
	  printf("%s: cannot bind port\n", argv[0]);
	  exit(1);
	}

	  int sd_serv, remoteServLen, cliLen;
	  struct sockaddr_in cliAddr, mimServAddr;


	// Set up connection to client side
	  /* socket creation */
	  sd_serv=socket(AF_INET, SOCK_DGRAM, 0);
	  if(sd_serv<0) {
	    printf("%s: cannot open socket \n",argv[0]);
	    exit(1);
	  }

	  /* bind local server port */
	  mimServAddr.sin_family = AF_INET;
	  mimServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	  mimServAddr.sin_port = htons(MIM_SERVER_PORT);
	  rc = bind (sd_serv, (struct sockaddr *) &mimServAddr,sizeof(mimServAddr));
	  if(rc<0) {
	    printf("%s: cannot bind port number %d \n", 
		   argv[0], MIM_SERVER_PORT);
	    exit(1);
	  }

	int sent;
	struct Message* message = (struct Message*)malloc(sizeof(struct Message));
	int n;
	while(1){
		// Receive message from client side
	    	n = recvfrom(sd_serv, message, sizeof(struct Message), MSG_DONTWAIT, 
			 (struct sockaddr *) &cliAddr,(socklen_t*) &cliLen);
		
		// Send message to server
		if(n > 0){
			sent = 0;
			if(message->is_ack == -1)
				printf("Received message %u from client. ",message->seq_no);
			else printf("Receive acknowledgement %u from client. ",message->ack);

			if(make_choice(p)){
				printf("Trying to send to server. ");
				rc = sendto(sd_cli, message, sizeof(struct Message), 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
				if(rc > 0){
					sent = 1;
					printf("Sent to server.\n");
				}
			}
			if(!sent){
				printf("Not sent to server.\n");
			}
		}

		// Get message from server
	    	n = recvfrom(sd_cli, message, sizeof(struct Message), MSG_DONTWAIT, 
			 (struct sockaddr *) &remoteServAddr,(socklen_t*) &remoteServLen);

		// Send message to client
		if(n > 0){
			sent = 0;
			if(message->is_ack == -1)
				printf("Received message %u from server. ",message->seq_no);
			else printf("Received acknowledgement %u from server. ",message->ack);

			if(make_choice(q)){
				printf("Trying to send to client. ");
				rc = sendto(sd_serv, message, sizeof(struct Message), 0, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
				if(rc > 0){
					sent = 1;
					printf("Sent to client.\n");
				}else{
					printf("Couldn't send. %s \n", strerror(errno));
				}
			}
			if(!sent){
				printf("Not sent to client.\n");
			}

		}

		
	}

	

}
