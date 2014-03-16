#include "swp.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void print_message(struct Message* message){
	printf("****************************\n");
	if(message->ack == -1){
		printf("Message Sequence Number: %u\n",message->seq_no);
		printf("---------------------------\n");
		fputs(message->content, stdout);
		printf("---------------------------\n");
		printf("\nMessage size = %d\n",message->size);
	}else{
		printf("Acknowledgement Number: %u\n",message->ack);
	}
	printf("****************************\n");
}


int receive_with_timeout(struct SWP* swp, struct Message* buf){

	return recvfrom(swp->sockfd, buf, sizeof(struct Message), 0, swp->addr, (socklen_t*) &swp->addrlen);
}


int add_message_to_sender(struct SenderSW* sender, char* buf, int ack, int size){
	//Assumes that the buffer has an extra position
	struct Message* message = (struct Message*) malloc(sizeof(struct Message));
	if (buf != NULL)
		memcpy(message->content, buf, CONTENT_SIZE);
	int pos;
	for(pos = sender->SWS - 1; pos >= 0; pos--){
		if(sender->window[pos] != NULL){
			break;
		}
	}
	pos += 1;
	message->ack = ack;
	message->size = size;
	sender->LFS = MOD(sender->LFS + 1);
#ifdef _DEBUG
	printf("sender: (LAR = %u, LFS = %u)\n",sender->LAR, sender->LFS);
#endif
	message->seq_no = sender->LFS;
	sender->window[pos] = message;
#ifdef _DEBUG
	printf("add_message_to_sender: adding message %u at %d\n", message->seq_no, pos);
#endif
	return 0;
}




struct SWP * get_new_SWP(unsigned int window_size, struct sockaddr* addr, int addrlen, int sockfd){
	struct SenderSW* sender = (struct SenderSW*)malloc(sizeof(struct SenderSW*));
	sender->LAR = MAX_SEQ_NO - 20;//-1;
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
	tv.tv_usec = TIMEOUT_USEC;
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
	if(message->ack == -1)
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


int is_valid_ack(struct SenderSW* sender, int ack_no){
	if(ack_no == -1){
		return 0;
	}
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
	// Returns 1 : if ack is successfully received
	// Returns -1: if no ack is received

	//Sending messages phase
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
	int cumul_ack = -1;				
	struct Message* buf = (struct Message*) malloc(sizeof(struct Message));
#ifdef _DEBUG
	printf("send_and_receive: trying to receive acknowledgements\n");
#endif
	int n = receive_with_timeout(swp, buf);
	int count = 1;
	while(n < 0 && count > 0){
		n = receive_with_timeout(swp, buf);
		count-- ;
	}
	while(n >= 0){
		if(n > 0){
			struct Message* message = (struct Message*) malloc(sizeof(struct Message));
			memcpy(message, buf, sizeof(struct Message));
			//Check for ack outside window!!
			if(is_valid_ack(swp->sender, message->ack)){
				if(cumul_ack == -1){
					// First ack in the lot
					cumul_ack = message->ack;
				}else if(cumul_ack - message->ack > swp->sender->SWS || message->ack - cumul_ack > swp->sender->SWS){
				// This means that the sliding window has a wrap up of sequence numbers
					if(cumul_ack > message->ack){
						cumul_ack = message->ack;
					}
				}else if(cumul_ack < message->ack){
					//No wrap up detected yet
					cumul_ack = message->ack;
				}
			}
		}
		n = receive_with_timeout(swp, buf);
	}

#ifdef _DEBUG
	printf("send_and_receive: cumulutive ack receive: %u\n",cumul_ack);
#endif
	if(cumul_ack != -1){
		//Remove all frames in the beginning	
		int last_removed;
		for(last_removed = 0; last_removed < swp->sender->SWS; last_removed ++){
			if(swp->sender->window[last_removed]->seq_no == cumul_ack){
				break;
			}
		}

		int pos;
		for(pos = 0; pos < swp->sender->SWS; pos ++){
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
	
	//Nothing received
	return -1;
}

//This is the high-level function called to send a message
int send_messages(struct SWP* swp, FILE* fp){
	int read_size = 1;
	char buf[CONTENT_SIZE];
	int ret_val;
	while(1){
		//Populating window stage
		while(is_sender_window_moved(swp) && (read_size = fread(buf, sizeof(char), CONTENT_SIZE, fp)) > 0){
			add_message_to_sender(swp->sender, buf, -1, read_size);//This is not an ack
		}

		int count = 2;
		while((ret_val = send_and_receive(swp)) != 1 && count > 0){
			// Nothing to send and receive
			count --;
		}
		if(count == 0){
			return ret_val;
		}

	}
	return 1;
}

int send_command(struct SWP* swp, char* command){
	char buf[CONTENT_SIZE];
	int i = 0;
	int ret_val = 1;
	while(ret_val == 1){
		//Populating window stage
		while(is_sender_window_moved(swp) && (i <= strlen(command))){
			if(i + CONTENT_SIZE > strlen(command)){
				memmove(buf, command + i, sizeof(char) * (strlen(command) - i + 1));
				add_message_to_sender(swp->sender, buf, -1, strlen(command) - i + 1);
				i = strlen(command) + 1;
				break;
			}else{
				memmove(buf, command + i, sizeof(char) * CONTENT_SIZE);
				add_message_to_sender(swp->sender, buf, -1, CONTENT_SIZE);
				i += CONTENT_SIZE;
			}
		}


		ret_val = send_and_receive(swp);

	}

#ifdef _DEBUG
	if(ret_val == -1){
		fputs("send_command: acknowledgement not received.\n",stdout);
	}else if(ret_val == 0){
		fputs("send_command: Everything sent. \n",stdout);
	}
#endif
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
	struct ReceiverSW* receiver = swp->receiver;
	struct Message* buf = (struct Message*)malloc(sizeof(struct Message));
	int n = receive_with_timeout(swp, buf);
	if(n >= 0){
		while(n >= 0){
			if(n > 0){
				struct Message* message = (struct Message*)malloc(sizeof(struct Message));
				memcpy(message, buf, sizeof(struct Message));
#ifdef _DEBUG
				printf("receive: message received\n");
				print_message(message);
#endif
				if(message->ack == -1 && is_valid_frame(swp->receiver, message->seq_no)){
#ifdef _DEBUG
					fputs("receive: frame is valid\n",stdout);
#endif
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
					printf("receive: adding to message at window index  %d\n", index);
#endif

					receiver->window[index] = message;					
				}
			}
			n = receive_with_timeout(swp, buf);
		}
		return 1;
	}
	return -1; //Nothing was received

}


int receive_command(struct SWP* swp, char command[MAX_COMMAND_SIZE]){
	// This SWP side is receiving a command. 
	// And sending an ack.
	struct ReceiverSW *receiver = swp->receiver;
	int com_ind = 0;
	int recvd = -1;
	while(1){
		
		// We try to keep receiving
		int recv_ret;
		if((recv_ret = receive(swp)) == -1){
			return recvd;
		}
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
#if _DEBUG 
		printf("receive_command: acknowledging till %u", receiver->LFR);
#endif
		ack_message->ack = receiver->LFR;
		deliver_message(swp, ack_message);	

		int ind;
		for(ind = 0; ind < receiver->RWS; ind ++){
			if(ind < pos){
				memmove(command + com_ind, receiver->window[ind]->content, sizeof(char) * CONTENT_SIZE);
				com_ind += CONTENT_SIZE;
				
			}
			if(ind + pos < receiver->RWS){
				receiver->window[ind] = receiver->window[ind + pos];
			}else{
				receiver->window[ind] = NULL;
			}
		}
	}
	return 0;

}

int receive_message(struct SWP* swp, FILE *fp){
	// This SWP side has sent a command. It will have to receives a message
	// or the output and write it to fp and send acks
	struct ReceiverSW *receiver = swp->receiver;
	while(1){
		
		// We have received for this round
		int count  = 2;
		while(count > 0 && receive(swp) == -1){
			//No response from the other side
			count --;
		}

		if(count == 0){
			return -1;
		}
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
		ack_message->ack = receiver->LFR;
		deliver_message(swp, ack_message);	

		int ind;
#ifdef _DEBUG
		printf("receive_message: output stream begins\n");
#endif
		for(ind = 0; ind < receiver->RWS; ind ++){
			if(ind < pos){
					fwrite(receiver->window[ind]->content, sizeof(char), receiver->window[ind]->size, fp);
			}
			if(ind + pos < receiver->RWS){
					receiver->window[ind] = receiver->window[pos];
			}else{
				receiver->window[ind] = NULL;
			}
		}
#ifdef _DEBUG
		printf("receive_message: output stream ends\n");
#endif

	}

	return 0;
}
