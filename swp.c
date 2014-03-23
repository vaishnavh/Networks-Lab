#include "swp.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void print_hex(const char *s, int l)
{
  while(l--)
    printf("%02x", (unsigned int) *s++);
  printf("\n");
}

void print_message(struct Message* message){
#if _DEBUG > 1
	printf("****************************\n");
	if(message->is_ack == -1){
		printf("Message Sequence Number: %u\n",message->seq_no);
		printf("----------------------------\n");
		fputs(message->content, stdout);
		printf("----------------------------\n");
		printf("\nMessage size = %d\n",message->size);
	}else{
		printf("Acknowledgement Number: %u\n",message->ack);
	}
	printf("****************************\n");
#endif
}


void free_message(struct Message* m){
	if(m!=NULL){
#if _DEBUG > 1
		printf("Freeing ... %p\n",m);
		print_message(m);
#endif
		free(m);
	}
}


int receive_with_timeout(struct SWP* swp, struct Message* buf){

	int n =  recvfrom(swp->sockfd, buf, sizeof(struct Message), 0, swp->addr, (socklen_t*) &swp->addrlen);
#if _DEBUG > 1
//	printf("receive_with_timeout: %d\n",n);
#endif
	return n;
}


int add_message_to_sender(struct SenderSW* sender, char* buf, int size){
	//Assumes that the buffer has an extra position
	struct Message* message = (struct Message*) malloc(sizeof(struct Message));
	if (buf != NULL)
		memmove(message->content, buf, size);
	int pos;
	for(pos = sender->SWS - 1; pos >= 0; pos--){
		if(sender->window[pos] != NULL){
			break;
		}
	}
	pos += 1;
	message->is_ack = -1; 
	message->ack = 0;
	message->size = size;
	sender->LFS = MOD(sender->LFS + 1);
#ifdef _DEBUG
	printf("sender: (LAR = %u, LFS = %u)\n",sender->LAR, sender->LFS);
#endif
	message->seq_no = sender->LFS;
	sender->window[pos] = message;
#ifdef _DEBUG
	printf("add_message_to_sender: adding message %u at %d\n", message->seq_no, pos);
	print_message(message);
#endif
	return 0;
}




struct SWP * get_new_SWP(unsigned int window_size, struct sockaddr* addr, int addrlen, int sockfd){
	struct SenderSW* sender = (struct SenderSW*)malloc(sizeof(struct SenderSW*));
	sender->LAR = -1;
	sender->LFS = sender->LAR;
	sender->SWS = window_size;
	sender->window = (struct Message**)malloc(window_size * sizeof(struct Message*));
	memset(sender->window, 0, window_size * sizeof(struct Message*));
	
	struct ReceiverSW* receiver = (struct ReceiverSW*)malloc(sizeof(struct ReceiverSW*));
	receiver->LFR = sender->LAR;
	receiver->LAF = window_size + sender->LAR;
	receiver->RWS = window_size;
	receiver->window = (struct Message**)malloc(window_size * sizeof(struct Message*));
	memset(receiver->window, 0, window_size * sizeof(struct Message*));
	struct SWP* swp = (struct SWP*)malloc(sizeof(struct SWP));
	swp->addr = addr;
	swp->addrlen = addrlen;
	swp->sockfd = sockfd;
	swp->receiver = receiver;
	swp->sender = sender;

	struct timeval tv;
	tv.tv_usec = TIMEOUT_USEC/TRIAL_NUM;
	tv.tv_sec = TIMEOUT_SEC;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}
	return swp;
}

int is_sender_window_moved(struct SWP* swp){
	struct SenderSW* sender = swp->sender;
	return (sender->window[sender->SWS - 1] == NULL);
}

int is_sender_empty(struct SWP* swp){
	return (swp->sender->window[0] == NULL);	
	
}


int deliver_message(struct SWP* swp, struct Message* message){
	int rc = sendto(swp->sockfd, message, sizeof(struct Message), 0, swp->addr, swp->addrlen);
#ifdef _DEBUG
	if(message->is_ack == -1)
		printf("deliver_message: sending message %u\n",message->seq_no);
	else
		printf("deliver_message: sending acknowledgement %u\n",message->ack);
#endif
	if(rc < 0){
		printf("Error sending  data\n");
		perror(strerror(errno));
		close(swp->sockfd);
		exit(1);
	}	
	return 1;	
}


int is_valid_ack(struct SenderSW* sender, unsigned int ack_no){
#ifdef _DEBUG
	printf("is_valid_ack: --- %u %u %u\n",sender->LFS, sender->LAR, ack_no);
#endif
	if(sender->LFS < MOD(sender->LAR)){
		//There's a wrap up
		return (ack_no <= sender->LFS) || (ack_no > sender->LAR);
	}else{
		return (ack_no <= sender->LFS) && (ack_no > sender->LAR);
	}
}


int send_and_receive(struct SWP* swp){
	//For sending big messages and commands
	//ONE LOOP of sending and receiving
	// Returns 0 : if window is empty
	// Returns 2 : if there is no valid ack received
	// Returns 1 : if ack is successfully received
	// Returns -1: if no ack is received

	// Tries sending once and receiving once!
	// Time interval  of 0.2 seconds
	// Sending messages phase
	if(is_sender_empty(swp)){
		return 0; // The window is empty. Nothing to send	
	}

#ifdef _DEBUG
	printf("send_and_receive: sending all frames\n");
#endif
	int pos;
	for(pos = 0; pos < swp->sender->SWS && swp->sender->window[pos] != NULL; pos ++){
		deliver_message(swp, swp->sender->window[pos]);
	}

	//Receiving phase
	// Window was not empty. Look for receipt for a time-gap of zero seconds.
	unsigned int cumul_ack;
	int ack_recvd = -1;	
	struct Message* buf = (struct Message*) malloc(sizeof(struct Message));
#ifdef _DEBUG
	printf("send_and_receive: trying to receive acknowledgements\n");
#endif
	int n = receive_with_timeout(swp, buf);
	int response = 0;
	while(n > 0){
		if(n > 0){
			response = 1;
			//Check for ack outside window!!
			if(buf->is_ack == 1 && is_valid_ack(swp->sender, buf->ack)){
#ifdef _DEBUG
				printf("send_and_receive: received valid acknowledgement: %u\n",buf->ack);
#endif
				if(ack_recvd == -1){
					// First ack in the lot
					cumul_ack = buf->ack;
					ack_recvd = 1;
				}else if(cumul_ack - buf->ack > swp->sender->SWS || buf->ack - cumul_ack > swp->sender->SWS){
				// This means that the sliding window has a wrap up of sequence numbers
					if(cumul_ack > buf->ack){
						cumul_ack = buf->ack;
					}
				}else if(cumul_ack < buf->ack){
					//No wrap up detected yet
					cumul_ack = buf->ack;
				}
			}
#ifdef _DEBUG
			else {
				//Invalid ack
				printf("send_and_receive: received invalid acknowledgement with seq no: %u, ack_no: %u, is_ack: %d\n",buf->seq_no, buf->ack, buf->is_ack);
			}
#endif
	
		}
		n = receive_with_timeout(swp, buf);
	}

	free_message(buf);

	if(ack_recvd != -1){
#ifdef _DEBUG
		printf("send_and_receive: cumulative ack received: %u\n",cumul_ack);
#endif
	//Remove all frames in the beginning	
		int last_removed;
		for(last_removed = 0; last_removed < swp->sender->SWS; last_removed ++){
			if(swp->sender->window[last_removed]->seq_no == cumul_ack){
				break;
			}
		}

		int pos;
#if _DEBUG > 1
		printf("send_and_receive: moving window pos = %d\n",last_removed);

#endif
		for(pos = 0; pos < swp->sender->SWS; pos ++){
			if(pos <= last_removed){
				free_message(swp->sender->window[pos]);
			}

			if(pos + last_removed + 1 < swp->sender->SWS){
				swp->sender->window[pos] = swp->sender->window[pos + last_removed + 1];
			}else{
				swp->sender->window[pos] = NULL;
			}

		}

		swp->sender->LAR = cumul_ack;
#ifdef _DEBUG
	printf("sender: (LAR = %u, LFS = %u)\n",swp->sender->LAR, swp->sender->LFS);
#endif
		return 1;
	}

	else if(response == 1){
#ifdef _DEBUG
		printf("send_and_receive: No valid acknowledgement received.\n");
#endif
		return 2;
	}
#ifdef _DEBUG
	printf("send_and_receive: No response received. \n");
#endif
	
	//Nothing received
	return -1;
}

//This is the high-level function called to send a message
int send_messages(struct SWP* swp, FILE* fp){
	int read_size = 1;
	char buf[CONTENT_SIZE];
	int ret_val;
	int no_response = 0;
	int message_over = 0;	
	//Send ack for packet at pos-1
	struct Message* ack_message = (struct Message*) malloc(sizeof(struct Message));
	ack_message->seq_no = 0;
	ack_message->ack = swp->receiver->LFR;
	ack_message->is_ack = 1;

	while(1){
		//Populating window stage
		deliver_message(swp, ack_message);	
		if(!message_over){

			while(is_sender_window_moved(swp) && (read_size = fread(buf, sizeof(char), CONTENT_SIZE, fp)) >= 0){
				if(read_size > 0){
					add_message_to_sender(swp->sender, buf, read_size);//This is not an ack
				}
				else{
					add_message_to_sender(swp->sender, NULL, 0);
					message_over = 1;
					break;
				}
			}
		}

		if((ret_val = send_and_receive(swp)) == 0){
			//Nothing was sent anyway
			free_message(ack_message);
			return 1;
		}

		if(ret_val == -1){
			//No acknowledgement for more than 1 second
			no_response ++;
			if(no_response == TRIAL_NUM){
				free_message(ack_message);
				return -1;
			}
		}else{
			no_response = 0;
		}

	}
	free_message(ack_message);
	return 1;
}

int send_command(struct SWP* swp, char* command){
	char buf[CONTENT_SIZE];
	int i = 0;
	int ret_val = 5;
	int no_response = 0;
	int command_over = 0;
	//One while loop is expected to be 0.2 seconds
		
	//Send ack for packet at pos-1
	struct Message* ack_message = (struct Message*) malloc(sizeof(struct Message));
#ifdef _DEBUG
	printf("send_command: acknowledging till %u\n", swp->receiver->LFR);
#endif
	ack_message->seq_no = 0;
	ack_message->ack = swp->receiver->LFR;
	ack_message->is_ack = 1;


	while(ret_val != 0 && no_response < TRIAL_NUM){
		//Populating window stage
		deliver_message(swp, ack_message);	
		while(is_sender_window_moved(swp) && !command_over){
			if(i == strlen(command)	+ 1){	
					//Ending message
#ifdef _DEBUG
				printf("send_command: ending message.\n");
#endif
				add_message_to_sender(swp->sender, NULL, 0);
				command_over = 1;

			}
			else if(i + CONTENT_SIZE > strlen(command)){
				memmove(buf, command + i, sizeof(char) * (strlen(command) - i + 1));
				
				add_message_to_sender(swp->sender, buf, strlen(command) - i + 1);
				i = strlen(command) + 1;
				
				
			}else if(i <= strlen(command)){
				memmove(buf, command + i, sizeof(char) * CONTENT_SIZE);
				add_message_to_sender(swp->sender, buf, CONTENT_SIZE);
				i += CONTENT_SIZE;
			}
					
		}
		ret_val = send_and_receive(swp);
		if(ret_val == -1){
			no_response ++;
		}else{
			no_response = 0;
		}
	}

#ifdef _DEBUG
	if(no_response == TRIAL_NUM){
		fputs("send_command: acknowledgement not received.\n",stdout);
	}else if(ret_val == 0){
		fputs("send_command: Everything sent. \n",stdout);
	}
#endif
	free_message(ack_message);
	return ret_val;

}



int is_valid_frame(struct ReceiverSW* receiver, int seq_no){
	// Checks on receiving end
	if(receiver->LFR > receiver->LAF){
		//Wrap up here
		return (seq_no <= receiver->LAF || seq_no > receiver->LFR);
	}
	return (seq_no <= receiver->LAF && seq_no > receiver->LFR);
}

int receive(struct SWP* swp){
	// Tries to receive a non-ack message into the receiver side of the swp
	// Time interval of 0.5 seconds
	// Returns -1 if no response is received
	// Returns 0 if bad response is received
	// Returns 1 if valid messages are received
	struct ReceiverSW* receiver = swp->receiver;
	struct Message* buf = (struct Message*)malloc(sizeof(struct Message));
	int n = receive_with_timeout(swp, buf);
	int valid = 0;
	if(n > 0){
		while(n > 0){
			if(n > 0){
				struct Message* message = (struct Message*)malloc(sizeof(struct Message));
				memcpy(message, buf, sizeof(struct Message));

#ifdef _DEBUG
				printf("receive: message received\n");
				print_message(message);
#endif
				if(message->is_ack == -1 && is_valid_frame(swp->receiver, message->seq_no)){
#ifdef _DEBUG
					fputs("receive: frame is valid\n",stdout);
#endif
					valid = 1;
					unsigned int index;
					// Have to place at the right position
					// The last index = LAF
					// The first index = Last frame received + 1
					if ( message->seq_no < MOD(receiver->LFR+1)){
						//wrap-up
						index = receiver->RWS - (receiver->LAF - message->seq_no) - 1;
					}else{
						index = message->seq_no - receiver->LFR - 1;

					}

#ifdef _DEBUG
					printf("receive: adding message %u at window index  %d\n", message->seq_no, index);
#endif

					receiver->window[index] = message;					
				}else{
#ifdef _DEBUG
					printf("receive: message received is invalid ack_no = %u, seq_no = %u, is_ack = %d.\n",message->ack, message->seq_no, message->is_ack);
#endif
				}
			}
			n = receive_with_timeout(swp, buf);
		}
		return valid;
	}
	return -1; //Nothing was received

}


int receive_command(struct SWP* swp, char command[MAX_COMMAND_SIZE], int server_side){
	// This SWP side is receiving a command. 
	// And sending an ack.
	// Returns 1 on successful receipt of command
	// Returns 0 on incomplete receipt of command
	// Returns -1 on lack of server response
	struct ReceiverSW *receiver = swp->receiver;
	int com_ind = 0;
	int recvd = 0;
	int recv_ret;
	int no_response = 0;
	int command_over = 0;
	while(1){
		
		// We try to keep receiving
		// Will return if two consecutive receive tries didn't work.
		if((recv_ret = receive(swp)) == -1){
			no_response ++;
			if(recvd == 0){
				if(server_side == 1)
					return -1;
				else if(no_response == TRIAL_NUM){
					return -1;
				}
					
			}
			if(no_response == TRIAL_NUM){
				//Send message to client about incomplete command
				//only if received messages are valid!
				send_command(swp, "WARNING: Try again. Incomplete transfer of message.\n");
				return 0;
			}
		}
		if(recv_ret == 1)
			recvd = 1;
		// Now send acks and write things down
		int pos;
		for(pos = 0; pos < receiver->RWS && receiver->window[pos] != NULL; pos++){
#if _DEBUG > 1
			printf("receive_command: frame at %d is not NULL\n",pos);
#endif
		}

		// pos is now at the index which has not been sent
		receiver->LFR = MOD(receiver->LFR + pos); 
		receiver->LAF = MOD(receiver->LFR + receiver->RWS);

#ifdef _DEBUG
		printf("receiver: (LFR = %u, LAF = %u)\n",receiver->LFR, receiver->LAF);
#endif


		
		//Send ack for packet at pos-1
		struct Message* ack_message = (struct Message*) malloc(sizeof(struct Message));
#ifdef _DEBUG
		printf("receive_command: acknowledging till %u\n", receiver->LFR);
#endif
		ack_message->ack = receiver->LFR;
		ack_message->is_ack = 1;
		deliver_message(swp, ack_message);	
		free_message(ack_message);

		int ind;
		for(ind = 0; ind < receiver->RWS; ind ++){
			if(ind < pos && !command_over){
				if(receiver->window[ind]->size == 0){
					command_over = 1;
				}else{
					memmove(command + com_ind, receiver->window[ind]->content, sizeof(char) * CONTENT_SIZE);
					com_ind += CONTENT_SIZE;
				}
				free_message(receiver->window[ind]);
			}
			if(ind + pos < receiver->RWS){
				receiver->window[ind] = receiver->window[ind + pos];
			}else{
				receiver->window[ind] = NULL;
			}
		}

		if(command_over == 1){
#ifdef _DEBUG
			printf("receive_command: received whole command.\n");
#endif
			return 1;
		}	
	}
	return 0;

}

int receive_message(struct SWP* swp, FILE *fp){
	// This SWP side has sent a command. It will have to receive a message
	// or the output and write it to fp and send acks
	// Returns 1 if complete message was read
	// Returns -1 if the sender stopped responding and no message was received
	// Returns 0 if incomplete message was read
	// THIS IS DIFFERENT FROM receive_command. Why?
	// Receive command doesn't timeout 
	struct ReceiverSW *receiver = swp->receiver;
	int recvd = -1;
	int no_response = 0,  recv_ret;
	int message_over = 0;
	while(1){
		
		// We have received for this round
		//int count  = 1;
		if((recv_ret = receive(swp)) == -1){
			//No response from the other side
			// for 1 s
			no_response ++;
			if(no_response == TRIAL_NUM)
				return recvd;
			//count --;
		}else{
			no_response = 0;
		}


		recvd = 0;

		// Now send acks and write things down
		int pos;
		for(pos = 0; pos < receiver->RWS && receiver->window[pos] != NULL; pos ++){
#if _DEBUG > 1
			printf("receive_message: window at %d not NULL\n",pos);
#endif
		}
		// pos is now at the index which has not been sent
#if _DEBUG > 1
		printf("receive_message: all frames just before %d sent \n",pos);
#endif
		receiver->LFR = MOD(receiver->LFR + pos); 
		receiver->LAF = MOD(receiver->LFR + receiver->RWS);
#ifdef _DEBUG
		printf("receiver: (LFR = %u, LAF = %u)\n",receiver->LFR, receiver->LAF);
#endif


		//Send ack for packet at pos-1
		struct Message* ack_message = (struct Message*) malloc(sizeof(struct Message));
#ifdef _DEBUG
		printf("receive_message: acknowledging till %u\n",receiver->LFR);
#endif
		ack_message->ack = receiver->LFR;
		ack_message->is_ack = 1;
		deliver_message(swp, ack_message);	
		free_message(ack_message);

		int ind;
#ifdef _DEBUG
		printf("receive_message: output stream begins\n");
		if(pos == 0){
			printf("receive_message: nothing to output\n");
		}
#endif
		for(ind = 0; ind < receiver->RWS; ind ++){
			if(ind < pos && !message_over){
					if(receiver->window[ind]->size == 0){
						//Reached end of message
#ifdef _DEBUG
						printf("receive_message: Message %u of size 0 read\n",receiver->window[ind]->seq_no);
#endif
						message_over = 1;
					}else{
						fwrite(receiver->window[ind]->content, sizeof(char), receiver->window[ind]->size, fp);
#ifdef _WRITE
						printf("receive_message: Writing %u of size %d\n",receiver->window[ind]->seq_no, receiver->window[ind]->size);
#endif

#ifdef _DEBUG
						printf("receive_message: Writing %u \n", receiver->window[ind]->seq_no);
#endif
						}

					free_message(receiver->window[ind]);
			}
			if(ind + pos < receiver->RWS){
					receiver->window[ind] = receiver->window[pos + ind];
			}else{
				receiver->window[ind] = NULL;
			}
		}
#ifdef _DEBUG
		printf("receive_message: output stream ends\n");
#endif
		if(message_over == 1){
#ifdef _DEBUG
			printf("receive_message: OUTPUT OVER\n");
#endif
			return 1;
		}

	}

	//never comes here
	return 0;
}

int is_exist_file(struct SWP* swp, char* file_name){
	// Returns 0 if file doesn't exist
	// Return value is not really useful
	FILE* fp = fopen(file_name, "r");
	if(fp == NULL){
		send_command(swp, "Unable to open file.\n");
		return 0;
	}
	else{
		fclose(fp);
		send_command(swp,"1");
	}
	return 1;
}

int get_file_confirmation(struct SWP* swp){
	char exist[MAX_COMMAND_SIZE];
	receive_command(swp, exist, 0);
	if(exist[0] == '1'){
		return 1;
	}
	return 0;
}

int wait_for_stdin(int usec, int sec)
{
    fd_set set;
    struct timeval timeout = { sec , usec };
 
    FD_ZERO(&set);
    FD_SET(0, &set);
    return select(1, &set, NULL, NULL, &timeout) == 1;
}
 
int get_user_command(struct SWP* swp, char* command){
	struct Message* ack_message = (struct Message*) malloc(sizeof(struct Message));
	ack_message->seq_no = 0;
	ack_message->ack = swp->receiver->LFR;
	ack_message->is_ack = 1;
			
	while(1){
		if(wait_for_stdin(0,TIMEOUT_USEC/TRIAL_NUM)){
			fgets(command, MAX_COMMAND_SIZE, stdin);
			free(ack_message);
			return 1;
		}else{
			deliver_message(swp, ack_message);
		}
	}
	return 0;
}
