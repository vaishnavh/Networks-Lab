#ifndef SWP_H_
#define SWP_H_

#include <sys/socket.h>
#include <stdio.h>
#define MAX_BLOCK_SIZE 10000
#define CONTENT_SIZE 16000
#define MAX_COMMAND_SIZE 32000
#define DISCONNECT_TIME 1 // 1 second
#define TIMEOUT_USEC 1000000
#define TRIAL_NUM 10
#define TIMEOUT_SEC 0
//define _DEBUG 2
//define _WRITE 1
#define MAX_SEQ_NO 4294967295
#define MOD(x) (x)

//TODO: Disconnection code
struct Message{
	unsigned int  seq_no; // sequence number
	unsigned int ack; // set as -1 if it is not an ack. otherwise set.
	int is_ack;
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
int receive_command(struct SWP*, char*, int);
int is_exist_file(struct SWP*, char*);
int get_file_confirmation(struct SWP*);
int get_user_command(struct SWP*, char* command);
#endif
