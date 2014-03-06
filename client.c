#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>


#define SERVER_PORT 8000
#define CLIENT_PORT 8500

int recvfrom_with_timeout(int sockfd, char *buf, int msg_len, struct  sockaddr *src_addr, int* len){
	fd_set socks;
	struct timeval t;
	FD_ZERO(&socks);
	FD_SET(sockfd, &socks);
	t.tv_sec = 100000;
	if(select(sockfd + 1, &socks, NULL, NULL, &t) && \
		       	recvfrom(sockfd, buf, msg_len, 0, src_addr, len)!=-1	)
		return 1;
	return 0;
}


int main(int argc, char *argv[]) {

	//Sanity check code
	if(argc<2){
		printf("usage: %s <server>\n",argv[0]);
		exit(0);
	}

	char command[5] = {"done"}; //Takes the command from the client side. 4 chars max.
	while(strcmp(command,"open")!=0){
		//Till the client enters "open" we don't proceed.
		scanf("%s",command);
	}


	int sd, rc, i, remoteServLen, n;
	struct sockaddr_in cliAddr, remoteServAddr;
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
	sd = socket(AF_INET,SOCK_DGRAM,0);
	if(sd<0) {
	printf("%s: cannot open socket \n",argv[0]);
	  exit(1);
	}
	  
	/* bind any port */
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cliAddr.sin_port = htons(CLIENT_PORT);
	
	rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
	if(rc<0) {
	  printf("%s: cannot bind port\n", argv[0]);
	  exit(1);
	}

	//Sends "open" first
	while(strcmp(command,"done")){
		//Keep asking for commands until done
		
		//Send command
		rc = sendto(sd, command, strlen(command)+1, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
		if(rc < 0){
			printf("Unable to send data\n");
			close(sd);
			exit(1);
		}

		//Receive response
		n = recvfrom_with_timeout(sd, command, 5, (struct sockaddr *) &remoteServAddr, &remoteServLen);
		printf("Recvd: %s\n",command);
		scanf("%s",command);
	}

	  
	  return 1;

}