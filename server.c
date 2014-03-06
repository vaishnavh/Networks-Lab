#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h>

#define SERVER_PORT 8000
#define CLIENT_PORT 8500

int main(int argc, char *argv[]) {

  char msg[5]; 
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

  /* server infinite loop */
  while(1) {
    


    /* receive message */
    cliLen = sizeof(cliAddr);
    n = recvfrom(sd, msg, 5, 0, 
		 (struct sockaddr *) &cliAddr, &cliLen);

    if(n<0) {
      printf("%s: cannot receive data \n",argv[0]);
      continue;
    }

    scanf("%s",msg);
    rc = sendto(sd, msg, strlen(msg)+1, 0, (struct sockaddr *) &cliAddr, cliLen);
    if(rc < 0){
	printf("Unable to send data\n");
	close(sd);
	exit(1);
    }

  }/* end of server infinite loop */

return 0;

}
