#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MAX_COMMAND_SIZE 500
#define MAX_RESPONSE_SIZE 500
#define MAX_BLOCK_SIZE 1000
#define CONTENT_SIZE 500
struct Message{
	char content[CONTENT_SIZE];
	int size;
	unsigned int seq_no;
};

#endif
