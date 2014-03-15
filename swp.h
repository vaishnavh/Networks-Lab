#ifndef SWP_H_
#define SWP_H_

#include <semaphore.h>
#define MAX_BLOCK_SIZE 1000
#define CONTENT_SIZE 500
#define DISCONNECT_TIME 1 // 1 second
#define TIMEOUT 1
#define MAX_SEQ_NO 4294967295
#define MOD ( x ) ((x) % MAX_SEQ_NO)
//TODO: Take care of duplicate ACKS!
//TODO: Take care of timeouts?
struct Message{
	unsigned int seq_no; // sequence number
	int ack; // set as -1 if it is not an ack. otherwise set.
	char content[CONTENT_SIZE];
};



struct SenderSW{
	// Window details
	unsigned int LAR; // Last ack received
	unsigned int LFS; // Last frame sent
	unsigned int SWS; // sender window size
	struct Message* window;
	
};


struct ReceiverSW{
	uns igned int LFR; // Last frame received
	unsigned int LAF; // largest acceptable frame
	unsigned int RWS; // window size
	struct Message* window;
};

struct SWP{
	// Socket details
	struct sockaddr* addr;
	int addrlen;
	int sockfd;
	struct ReceiverSW* receiver;
	struct SenderSW* sender;
};
#endif
