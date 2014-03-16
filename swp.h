#ifndef SWP_H_
#define SWP_H_

#include <sys/socket.h>
#include <stdio.h>
#define MAX_BLOCK_SIZE 10000
#define CONTENT_SIZE 16000
#define MAX_COMMAND_SIZE 32000
#define DISCONNECT_TIME 1 // 1 second
#define TIMEOUT_USEC 550000
#define TIMEOUT_SEC 0
#define MAX_SEQ_NO 4294967295
#define _DEBUG 1
#define MOD(x) (x)

//TODO: check with greater window size
//TODO: fix timeouts
//TODO: get wrapup?
//TODO: Disconnection code
struct Message{
	unsigned int seq_no; // sequence number
	int ack; // set as -1 if it is not an ack. otherwise set.
	int size;
	char content[CONTENT_SIZE];
};



struct SenderSW{
	// Window details
	unsigned int LAR; // Last ack received
	unsigned int LFS; // Last frame sent
	unsigned int SWS; // sender window size
	struct Message** window;
	
};


struct ReceiverSW{
	unsigned int LFR; // Last frame received
	unsigned int LAF; // largest acceptable frame
	unsigned int RWS; // window size
	struct Message** window;
};

struct SWP{
	// Socket details
	struct sockaddr* addr;
	socklen_t addrlen;
	int sockfd;
	struct ReceiverSW* receiver;
	struct SenderSW* sender;
};

struct SWP * get_new_SWP(unsigned int, struct sockaddr*, int, int);
int send_messages(struct SWP*, FILE*);
int send_command(struct SWP*, char*);
int receive_message(struct SWP*, FILE*);
int receive_command(struct SWP*, char*);
#endif
