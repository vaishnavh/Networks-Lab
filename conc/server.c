#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "message.h"
#include <string.h>

#define SERVER_PORT 8000
//#define MIM_PORT 8200
 
int send_message(int sd, char* msg, int size, int seq_no){
	struct Message* response = (struct Message*) malloc(sizeof(struct Message)) ;

	response->size = size;
	response->seq_no = seq_no;
	memcpy(response->content, msg, size);
	printf("Sending %d \n",seq_no);	
	fwrite(msg, sizeof(char), size, stdout);
	if(send(sd, response, sizeof(struct Message), 0) < 0){
		free(response);
		perror("could not send");
		exit(1);
	}
	free(response);
	return 0;
}

int send_messages(int sd, FILE* fp, int *seq_no){
	char buff[CONTENT_SIZE];
	int read_size;
	printf("Sending: message\n");
	while((read_size = fread(buff, sizeof(char), CONTENT_SIZE, fp)) > 0){
		printf("Sending: %s\n",buff);
		send_message(sd, buff, read_size, (*seq_no)++);
	}
	printf("Sent message\n");
	return 0;
}


int execute_command(int sd, char* command, int* seq_no){
	FILE *fp;
	fp = popen(command,"r");
	printf("Executing command \n");
	if(fp == NULL){
		printf("Bad output \n");
		close(sd);
		exit(1);
	}
	if(command[0]=='c' && command[1] == 'd'){
		pclose(fp);
		return 0;
	}
	send_messages(sd, fp, seq_no);
	pclose(fp);
	return 0;
}
	


int main()
{
 int sfd, cfd;
 socklen_t len;
 struct sockaddr_in saddr, caddr;
 
 sfd= socket(AF_INET, SOCK_STREAM, 0);
 
 saddr.sin_family=AF_INET;
 saddr.sin_addr.s_addr=htonl(INADDR_ANY);
 saddr.sin_port=htons(SERVER_PORT);
 
 bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr));
 
 listen(sfd, 5);  //listen upto 5 connections simultaneously
 signal(SIGCHLD, SIG_IGN);
 
 while(1) {
	 printf("Server waiting\n");
	 len=sizeof(caddr);
	 cfd=accept(sfd, (struct sockaddr *)&caddr, &len); //Returns descriptor
	 
	 if( fork() == 0) {
	 
		 close(sfd); // In the child process close this. Parent closes cfd
		 char msg[MAX_COMMAND_SIZE]; 
		 struct Message* buff = (struct Message*)malloc(sizeof(struct Message));
		 int started = 0;
		 int seq_no = 0;
		 int rc;
		 printf("Came into first connection");
		 while(1){
			 while((rc = recv(cfd, buff, sizeof(struct Message), MSG_DONTWAIT))<=0);
			 memcpy(msg, buff->content, buff->size);
			 if(msg[strlen(msg)-1]=='\n'){
			    msg[strlen(msg)-1]='\0';
			  }
			 printf("Received %s with rc = %d and started = %d\n",msg, rc, started);

			 if(!started){
			     if(strcmp(msg,"open") == 0){
				    if(chdir("/")==-1){
					    exit(1);
				    }else{
					    send_message(cfd, "HOME\0", 5, seq_no ++);
					    printf("Opened... ");
					    started = 1;
				    }
			    }
		    	  }
			    
			    //ls command
			    else if(msg[0] == 'l' && msg[1] == 's'){
				    execute_command(cfd, msg, &seq_no);
			    }else if(msg[0] == 'c' && msg[1] == 'd'){
				    if(msg[2] == '\0'){
					    //A simple cd.
					    if(chdir("/")==-1){
						    send_message(cfd, "Cannot move to home directory.\n\0", 32, seq_no ++);
					    }else{
						    execute_command(cfd, msg, &seq_no);
						    send_message(cfd, "Moved to home directory\n\0", 25, seq_no ++);
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
						send_message(cfd, "Cannot move to specified location.\n\0", 32, seq_no ++);
					    }else{
						send_message(cfd, "Successfully moved to specified location.\n\0", 41, seq_no ++);
					    }
				    }
			    }else if(msg[0] == 'p' && msg[1] == 'w' && msg[2] == 'd'){
				    execute_command(cfd, "pwd", &seq_no);
			    }else if(msg[0] == 'g' && msg[1] == 'e' && msg[2] == 't'){
				    int i;
				    int k = strlen(msg);
				    for(i = 0; i <= k - 4; i++){
					    msg[i] = msg[i+4];
				    }
				    msg[k - 4] = '\0';
				    FILE *fp;
				    fp = fopen(msg, "rb");
				    if(fp == NULL){
					    printf("Unable to open file.\n");				   
				    } else{
					    printf("Beginning file transfer.\n");	
					    send_messages(cfd, fp, &seq_no);
					    fclose(fp);
				    }
			   }else{
				send_message(cfd, "Invalid command.\n\0", 19, seq_no ++);
			   }
		}
		
		 free(buff);
		 close(cfd);
		 return 0;

	 }

	 
 
	 close(cfd); // Parent has closed this.
	 // As there is a bound on the number of fd's you can have.
 }

 
}
