#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "swp.h"

#define SERVER_PORT 8000
#define CLIENT_PORT 8500

int main(int argc, char *argv[]) {

	//Sanity check code
	if(argc<3){
		printf("usage: %s <server> <window_size>\n",argv[0]);
		exit(0);
	}

	char command[MAX_COMMAND_SIZE] = {"done\0"};


	int sd, rc;
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

	struct SWP* swp = get_new_SWP(atoi(argv[2]), (struct sockaddr*)&remoteServAddr, sizeof(remoteServAddr), sd);
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
		if(send_command(swp, command) == -1){
			printf("Server not responding\n");
			exit(0);
		}


		//Receive response
	
		if(command[0] == 'g' && command[1] == 'e' && command[2] == 't'){
			    int i;
			    int k = strlen(command);
			    for(i = 0; i <= k - 4; i++){
				    command[i] = command[i+4];
			    }
			    command[k - 4] = '\0';
			    FILE *fp;
			    fp = fopen(command, "wb");
			    if(fp == NULL){
				    printf("Unable to store file %s\n", command);				   
			    } else{

				receive_message(swp, fp);
				fclose(fp);
			    }

		}
		else {  
			receive_message(swp, stdout);
		}
		fputs("=============================================\n", stdout);
		fgets(command, MAX_COMMAND_SIZE, stdin);
		fputs("---------------------------------------------\n", stdout);
	}

		fputs("=============================================\n", stdout);
 
	  return 1;

}
