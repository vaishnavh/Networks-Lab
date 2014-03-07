#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>
#include <unistd.h>

#define SERVER_PORT 8000
#define CLIENT_PORT 8500
#define MAX_COMMAND_SIZE 500
#define MAX_BLOCK_SIZE 500



int send_response(int sockfd, char* buf, struct sockaddr* dest_addr, int addrlen){
	int rc = sendto(sockfd, buf, strlen(buf)+1, 0, dest_addr, addrlen);
	  if(rc < 0){
		printf("Unable to send data\n");
		close(sockfd);
		exit(1);
	    }
	return 0;
}

int execute_command(char* command, int sockfd, struct sockaddr* dest_addr, int addrlen){
	FILE *fp;
	char output[MAX_BLOCK_SIZE];
	fp = popen(command,"r");
	if(fp == NULL){
		printf("Something wrong with \"%s\"\n",command);
		exit(1);
	}
	while(fgets(output, sizeof(output) - 1, fp) != NULL){
		 send_response(sockfd, output, dest_addr, addrlen);
	}
	return 0;
}
	



int main(int argc, char *argv[]) {

  char msg[MAX_COMMAND_SIZE]; 
  int sd, rc, n, cliLen;
  struct sockaddr_in cliAddr, servAddr;

  /* socket creation */
  sd=socket(AF_INET, SOCK_DGRAM, 0);
  if(sd<0) {
    printf("%s: cannot open socket \n",argv[0]);
    exit(1);
  }

  /* bind local server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(SERVER_PORT);
  rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));
  if(rc<0) {
    printf("%s: cannot bind port number %d \n", 
	   argv[0], SERVER_PORT);
    exit(1);
  }

  printf("%s: waiting for data on port UDP %u\n", 
	   argv[0],SERVER_PORT);
  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/

  int started = 0;
  /* server infinite loop */
  while(1) {
	    
		
	    /* receive message */
	    cliLen = sizeof(cliAddr);
	    n = recvfrom(sd, msg, MAX_BLOCK_SIZE, 0, 
			 (struct sockaddr *) &cliAddr,(socklen_t*) &cliLen);

	    if(n<0) {
	      printf("%s: cannot receive data \n",argv[0]);
	      continue;
	    }

	    if(msg[strlen(msg)-1]=='\n'){
		    msg[strlen(msg)-1]='\0';
	    }

	    printf("Command received: %s\n",msg);
	    if(!started){
		     if(strcmp(msg,"open") == 0){
			    if(chdir("/")==-1){
				    printf("Cannot move to home directory.\n");
				    exit(1);
			    }else{
				    printf("Moved to home directory.\n");
				    send_response(sd, "HOME\n", (struct sockaddr *) &cliAddr, cliLen);
				    started = 1;
			    }
		    }else{
			printf("Cannot execute \"%s\". Type \"open\" to start connection.\n",msg);
		    }
	    }
	    
	    //ls command
	    else if(msg[0] == 'l' && msg[1] == 's'){
		    execute_command(msg, sd, (struct sockaddr*) &cliAddr, cliLen);
	    }else if(msg[0] == 'c' && msg[1] == 'd'){
		    if(msg[2] == '\0'){
			    //A simple cd.
			    if(chdir("/")==-1){
				    printf("Cannot move to home directory.\n");
				    exit(1);
			    }
			    printf("Moved to home directory\n");
		    }
		    else{
			    int i;
			    int k = strlen(msg);
			    for(i = 0; i <= k - 3; i++){
				    msg[i] = msg[i+3];
			    }
			    msg[k - 3] = '\0';
			    if(chdir(msg)==-1){
				printf("Cannot move to specified location.\n");
			    }
			    printf("Moved to \"%s\"\n",msg);
		    }
	    }else if(msg[0] == 'p' && msg[1] == 'w' && msg[2] == 'd'){
		    execute_command("pwd", sd, (struct sockaddr*) &cliAddr, cliLen);
	    }else if(msg[0] == 'g' && msg[1] == 'e' && msg[2] == 't'){
		    int i;
		    int k = strlen(msg);
		    for(i = 0; i <= k - 4; i++){
			    msg[i] = msg[i+4];
		    }
		    msg[k - 4] = '\0';
		    FILE *fp;
		    printf("Sending %s\n",msg);
		    fp = fopen(msg, "r");
		    if(fp == NULL){
			    printf("Unable to open file %s\n", msg);				   
		    } else{
			    char output[MAX_BLOCK_SIZE];
			    while(fgets(output, sizeof(output) - 1, fp) != NULL){
		 	        send_response(sd, output, (struct sockaddr *) &cliAddr, cliLen);
		            }
			    fclose(fp);
		    }
           }
		   
    }/* end of server infinite loop */

return 0;

}
