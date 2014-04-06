#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>
#include <unistd.h>
#include "swp.h"

#define SERVER_PORT 8000
#define MIM_PORT 8200


int execute_command(struct SWP* swp, char* command, int sockfd, struct sockaddr* dest_addr, int addrlen){
	FILE *fp;
	fp = popen(command,"r");
	if(fp == NULL){
		//printf("Something wrong with \"%s\"\n",command);
		close(sockfd);
		exit(1);
	}
	if(command[0]=='c' && command[1] == 'd'){
		return 0;
	}
	if(send_messages(swp, fp) == -1){
		//printf("Unable to reach client\n");
		close(sockfd);	
		exit(1);
	}
	pclose(fp);
	return 0;
}
	



int main(int argc, char *argv[]) {

  char msg[MAX_COMMAND_SIZE]; 
  int sd, rc, cliLen;
  struct sockaddr_in cliAddr, servAddr;

  //Sanity check code
  if(argc<2){
	printf("usage: %s <window_size>\n",argv[0]);
	exit(0);
  }


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
    printf("%s: cannot bind port number %d \n", argv[0], SERVER_PORT);
    exit(1);
  }

  //printf("%s: waiting for data on port UDP %u\n", argv[0],SERVER_PORT);
  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/
  /*----------------------------------------------------------------*/

  int started = 0;

  cliLen = sizeof(cliAddr);
  struct SWP* swp = get_new_SWP(atoi(argv[1]), (struct sockaddr*)&cliAddr, cliLen, sd);
  /* server infinite loop */
  while(1) {
	    
	    	
	    /* receive message */
	    while((rc = receive_command(swp, msg, 1)) != 1);
	    if(msg[strlen(msg)-1]=='\n'){
		    msg[strlen(msg)-1]='\0';
	    }
	    //printf("Command received: %s\n",msg);

	

	    if(!started){
		     if(strcmp(msg,"open") == 0){
			     //Remember to change this later
			    if(chdir("/home/vaish")==-1){
				    //printf("Cannot move to home directory.\n");
				    exit(1);
			    }else{
				    //printf("Moved to home directory.\n");
				    send_command(swp, "HOME\n");
				    started = 1;
			    }
		    }else{
			//printf("Cannot execute \"%s\". Type \"open\" to start connection.\n",msg);
		    }
	    }
	    
	    //ls command
	    else if(msg[0] == 'l' && msg[1] == 's'){
		    execute_command(swp, msg, sd, (struct sockaddr*) &cliAddr, cliLen);
	    }else if(msg[0] == 'c' && msg[1] == 'd'){
		    if(msg[2] == '\0'){
			    //A simple cd.
			    if(chdir("/")==-1){
				    //printf("Sending message: cannot move to home directory.\n");
				    send_command(swp, "Cannot move to home directory.\n");
			    }else{
				    //printf("Sending message: Moved to home directory.\n");
				    send_command(swp,"Moved to home directory\n");
			    }
		    }
		    else{
			    int i;
			    int k = strlen(msg);
			    for(i = 0; i <= k - 3; i++){
				    msg[i] = msg[i+3];
			    }
			    msg[k - 3] = '\0';
			    if(chdir(msg)==-1){
				    //printf("Sending message: cannot move to specified directory.\n");
				send_command(swp, "Cannot move to specified location.\n");
			    }else{
				    //printf("Sending message: successfully moved to specified location.\n");
			        send_command(swp, "Successfully moved to specified location.\n");
			    }
		    }
	    }else if(msg[0] == 'p' && msg[1] == 'w' && msg[2] == 'd'){
		    execute_command(swp, "pwd", sd, (struct sockaddr*) &cliAddr, cliLen);
	    }else if(msg[0] == 'g' && msg[1] == 'e' && msg[2] == 't'){
		    int i;
		    int k = strlen(msg);
		    for(i = 0; i <= k - 4; i++){
			    msg[i] = msg[i+4];
		    }
		    msg[k - 4] = '\0';
		    FILE *fp;
		    //is_exist_file(swp, msg);
		    fp = fopen(msg, "rb");
		    if(fp == NULL){
			    //printf("Unable to open file.\n");				   
		    } else{
			    //printf("Beginning file transfer.\n");	
			    send_messages(swp, fp);
			    fclose(fp);
		    }
           }else{
	   	send_command(swp, "Invalid command.\n");
	   }
		   
    }/* end of server infinite loop */

return 0;

}
