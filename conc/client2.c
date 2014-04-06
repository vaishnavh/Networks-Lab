/* fpont 12/99 */
/* pont.net    */
/* tcpClient.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <stdlib.h>
#include <string.h> /* memset() */
#include "message.h"


#define CLIENT_PORT 8500
#define MIM_PORT 8100
int receive_message(int sd, FILE* fp){
	int read_size;
	struct Message* buff = (struct Message*) malloc(sizeof(struct Message));
	while((read_size = recv(sd, buff, sizeof(struct Message), MSG_PEEK )) <= 0);
	// Look for  message
	while((read_size = recv(sd, buff, sizeof(struct Message), 0 )) > 0){
		fwrite(buff->content, sizeof(char), buff->size, fp);
	}
	free(buff);
	return 1;
}
int main (int argc, char *argv[]) {

  int sd, rc;
  struct sockaddr_in cliAddr, remoteServAddr;
  struct hostent *h;
  
  if(argc < 2) {
    printf("usage: %s <server> \n",argv[0]);
    exit(1);
  }

  h = gethostbyname(argv[1]);
  if(h==NULL) {
    printf("%s: unknown host '%s'\n",argv[0],argv[1]);
    exit(1);
  }

  remoteServAddr.sin_family = h->h_addrtype;
  memcpy((char *) &remoteServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
  remoteServAddr.sin_port = htons(MIM_PORT);

  /* create socket */
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if(sd<0) {
    perror("cannot open socket ");
    exit(1);
  }

  /* bind any port number */
  cliAddr.sin_family = AF_INET;
  cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  cliAddr.sin_port = htons(CLIENT_PORT);
  
  rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
  if(rc<0) {
    printf("%s: cannot bind port TCP %u\n",argv[0],MIM_PORT);
    perror("error ");
    exit(1);
  }
				
  /* connect to server */
  rc = connect(sd, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
  if(rc<0) {
    perror("cannot connect ");
    exit(1);
  }


	struct timeval tv;

	tv.tv_sec = 0; 
	tv.tv_usec = 50000;  

	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

	char command[MAX_COMMAND_SIZE]	= {"done\0"};

	while(strcmp(command,"open\n")!=0){
		fputs("\n >> ", stdout);
		//Till the client enters "open" we don't proceed.
		fgets(command, MAX_COMMAND_SIZE, stdin); //We don't use scanf as we have to
		fputs("----------------------------------------\n", stdout);
		//accept spaces too.
		printf("Hello\n");
	}


    
	int seq_no = 0;
    
	//Sends "open" first
	while(strcmp(command,"done\n")){
		//Keep asking for commands until done
		
		//Send command
		printf("Sending\n");
		struct Message  *buff = (struct Message*) malloc(sizeof(struct Message));

		memcpy(buff->content, command, strlen(command) + 1);
		buff->size = strlen(command) + 1;
		buff->seq_no = seq_no ++;
    		rc = send(sd, buff, sizeof(struct Message), 0);
		printf("Sent\n");
		if(rc == -1){
			printf("Server not responding\n");
			exit(0);
		}


		//Receive response
		if(command[0] == 'g' && command[1] == 'e' && command[2] == 't'){
			    int k = strlen(command);
		       	    char new_name[k - 3];
			    memcpy(new_name, command + 4, k - 4);
			    new_name[k - 5] = '\0';
			    //rc = get_file_confirmation(swp);
			    rc = 1;
			    if(rc == 1){
				FILE *fp;
				fp = fopen(new_name, "wb");
				if(fp == NULL){
					printf("Unable to store file %s\n", command);
				}else{	
					rc = receive_message(sd, fp);
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
			if(receive_message(sd, stdout) == -1){
				printf("Server unreachable. Exit\n");
				close(sd);
				exit(1);
			}

		}

		fputs("=============================================\n >> \n", stdout);
		fgets(command, MAX_COMMAND_SIZE, stdin); //We don't use scanf as we have to
		
		fputs("---------------------------------------------\n", stdout);
	}

		fputs("=============================================\n", stdout);
 


return 0;
  
}

