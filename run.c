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
    }

int main(int argc, char *argv[]) {

	
	//Sanity check code
	if(argc<5){
		printf("usage: %s <server> <window_size> <p> <q>\n",argv[0]);
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
	  exit(1);
	}
	  
	/* bind any port */
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cliAddr.sin_port = htons(CLIENT_PORT);
	
	rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
	if(rc<0) {
	  exit(1);
	}

	struct SWP* swp = get_new_SWP(atoi(argv[2]), (struct sockaddr*)&remoteServAddr, sizeof(remoteServAddr), sd);
	
	memmove(command, "open\n",10);

  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/

	FILE* fp1 = fopen("temp.txt","w");
	int count = 6;
	//Sends "open" first
	while(count -- ){
		//Keep asking for commands until done
		
		//Send command
		if(send_command(swp, command) == -1){
			exit(0);
		}

		//Receive response
		if(command[0] == 'g' && command[1] == 'e' && command[2] == 't'){
			    int k = strlen(command);
		       	    char new_name[k - 2];
			    memcpy(new_name, command + 4, k - 4);
			    new_name[k - 5] = '0' + count;
			    new_name[k - 4] = '\0';
			    //rc = get_file_confirmation(swp);
			    rc = 1;
			    if(rc == 1){
				FILE *fp;
				fp = fopen(new_name, "wb");
				if(fp == NULL){
				}else{	
					start();
					rc = receive_message(swp, fp);
					struct timeval tm2;
				    gettimeofday(&tm2, NULL);

				    unsigned long long t = 1000000 * (unsigned long long)(tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec);
				    //printf("File transfer took %llu us\n", t)	;
				    					if(rc == -1){
						close(sd);
						fclose(fp);
						exit(1);
					}

					fclose(fp);
					char exec[11] = {"xxd a1 > c"};
				    exec[5] = '0'+count;
				    system(exec);
				    char pr[26]={"cmp -s c com && echo -n 1"};
				    pr[strlen(pr)-1] = count + '0';
				    system(pr);
				    char write[100];
				    int plus;
				    memcpy(write,"echo , ", 6);
				    memcpy(write + 6, argv[3],strlen(argv[3]));
					    plus = 6 + strlen(argv[3]);
				    memcpy(write + plus,", ",2);
				    plus += 2;
				    memcpy(write + plus, argv[4],strlen(argv[4]));
					    plus += strlen(argv[4]);
				    memcpy(write + plus,", ",2);
				    plus += 2;

				    char time[20];
				    snprintf(time, 20, "%llu\n", t);
			    	    //fputs(time, stdout);
				    memcpy(write + plus, time, strlen(time)+1);

				    system(write);

				}

			    }
		}
		else {  
			if(receive_message(swp, fp1) == -1){
				printf("Server unreachable. Exit\n");
				close(sd);
				exit(1);
			}

		}


		memmove(command, "get a\n",10);

	}


 	fclose(fp1);
	  return 1;

}
