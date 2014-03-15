#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "message.h"

#define SERVER_PORT 8000
#define CLIENT_PORT 8500

int recvfrom_with_timeout(int sockfd, char *buf, struct  sockaddr *src_addr, int* len){
	
	fd_set socks;
	struct timeval t;
	FD_ZERO(&socks);
	FD_SET(sockfd, &socks);
	t.tv_usec = 1000000;
	t.tv_sec = 0;
	int bool_1 = select(sockfd + 1, &socks, NULL, NULL, &t);
	if(bool_1){
		struct Message message;
		int n =  recvfrom(sockfd, (void*)&message, MAX_BLOCK_SIZE, 0, src_addr, (socklen_t*)len);
		strcpy(buf, message.content);
		if(n>=0){
			return 1;
		}else{
			return -1;
		}
	}

	return 0;

	//return	recvfrom(sockfd, buf, msg_len, 0, src_addr, (socklen_t*)len);
}


int main(int argc, char *argv[]) {

	//Sanity check code
	if(argc<2){
		printf("usage: %s <server>\n",argv[0]);
		exit(0);
	}

	char command[MAX_COMMAND_SIZE] = {"done\0"};
	char response[MAX_RESPONSE_SIZE]; 



	int sd, rc, remoteServLen, n;
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


	while(strcmp(command,"open\n")!=0){
		//Till the client enters "open" we don't proceed.
		fgets(command, MAX_COMMAND_SIZE, stdin); //We don't use scanf as we have to
		//accept spaces too.
	}

  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/

	//Sends "open" first
	while(strcmp(command,"done\n")){
		//Keep asking for commands until done
		
		//Send command
		rc = sendto(sd, command, strlen(command)+1, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
		if(rc < 0){
			printf("Unable to send data\n");
			close(sd);
			exit(1);
		}


		//Receive response
		int count = 0;
		n = recvfrom_with_timeout(sd, response, (struct sockaddr *) &remoteServAddr, &remoteServLen);
	
		if(command[0] == 'g' && command[1] == 'e' && command[2] == 't'){
			if(n > 0){  
			    int i;
			    int k = strlen(command);
			    for(i = 0; i <= k - 4; i++){
				    command[i] = command[i+4];
			    }
			    command[k - 4] = '\0';
			    FILE *fp;
			    fp = fopen(command, "w");
			    if(fp == NULL){
				    printf("Unable to store file %s\n", command);				   
			    } else{
				    while(n > 0){
					    fputs(response, fp);
					    n = recvfrom_with_timeout(sd, response, (struct sockaddr *) &remoteServAddr, &remoteServLen);
				    }
				    fclose(fp);
			    }
			    fputs("Received file.\n",stdout);

			}			
			else{
				printf("File not found\n");
			}
		}
		else {  
			while(n > 0){
				fputs(response, stdout);
				n = recvfrom_with_timeout(sd, response, (struct sockaddr *) &remoteServAddr, &remoteServLen);
				count++;
			}
		}
		fputs("=============================================\n", stdout);
		fgets(command, MAX_COMMAND_SIZE, stdin);
		fputs("---------------------------------------------\n", stdout);
	}

		fputs("=============================================\n", stdout);
 
	  return 1;

}
