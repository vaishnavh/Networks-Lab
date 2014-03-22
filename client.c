#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "swp.h"

#define MIM_PORT 8100
#define CLIENT_PORT 8500


static struct timeval tm1;

static inline void start(){
    gettimeofday(&tm1, NULL);
}

static inline void stop(){
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t = 1000000 * (unsigned long long)(tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec);
    printf("File transfer took %llu us\n", t);
}

int main(int argc, char *argv[]) {

	//Sanity check code
	if(argc<3){
		printf("usage: %s <server> <window_size>\n",argv[0]);
		exit(0);
	}

	char command[MAX_COMMAND_SIZE]	= {"done\0"};


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
	remoteServAddr.sin_port = htons(MIM_PORT);

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
		fputs("\n >> ", stdout);
		//Till the client enters "open" we don't proceed.
		fgets(command, MAX_COMMAND_SIZE, stdin); //We don't use scanf as we have to
		fputs("----------------------------------------\n", stdout);
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
			    int k = strlen(command);
		       	    char new_name[k - 3];
			    memcpy(new_name, command + 4, k - 4);
			    new_name[k - 5] = '\0';
			    rc = get_file_confirmation(swp);
			    if(rc == 1){
				FILE *fp;
				fp = fopen(new_name, "wb");
				if(fp == NULL){
					printf("Unable to store file %s\n", command);
				}else{	
					start();
					rc = receive_message(swp, fp);
					stop();
					if(rc == -1){
						printf("Server unreachable. Exit\n");
						close(sd);
						fclose(fp);
						exit(1);
					}else if(rc == 0){
						printf("WARNING: incomplete response received. \n");
					}else{
						printf("Successfully received file.\n");
					}
					fclose(fp);
				}
			    }else if(rc == 0){
			    	printf("File does not exist.\n");
			    }

		}
		else {  
			if(receive_message(swp, stdout) == -1){
				printf("Server unreachable. Exit\n");
				close(sd);
				exit(1);
			}

		}

		fputs("=============================================\n >> ", stdout);
		fgets(command, MAX_COMMAND_SIZE, stdin);
		fputs("---------------------------------------------\n", stdout);
	}

		fputs("=============================================\n", stdout);
 
	  return 1;

}
