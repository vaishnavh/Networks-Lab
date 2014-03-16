#include "swp.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void print_message(struct Message* message){
	printf("Message Sequence Number:%d \n",message->seq_no);
	if(message->ack == -1){
		fputs(message->content, stdout);
		printf("\nMessage size = %d\n",message->size);
	}else{
		printf("Ack : %d\n",message->ack);
	}
}


int receive_with_timeout(struct SWP* swp, struct Message* buf){
	/*fd_set socks;
	struct timeval t;
	FD_ZERO(&socks);
	FD_SET(swp->sockfd, &socks);
	t.tv_sec = TIMEOUT;
	t.tv_usec = 0;
	int bool_1 = select(swp->sockfd + 1, &socks, NULL, NULL, &t);
	if(bool_1){
		//Something has been sent...
		//buf must have been allocated
		int n =  recvfrom(swp->sockfd, buf, sizeof(struct Message), 0, swp->addr, (socklen_t*) &swp->addrlen);
		return n;
	}

	//Nothing received within timeout
	return 0;*/
	int n = recvfrom(swp->sockfd, buf, sizeof(struct Message), 0, swp->addr, (socklen_t*) &swp->addrlen);
	return n;



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
	message->seq_no = sender->LFS;
	sender->window[pos] = message;
	printf("Adding this message at %d\n", pos);
	return 0;
}




struct SWP * get_new_SWP(unsigned int window_size, struct sockaddr* addr, int addrlen, int sockfd){
	struct SenderSW* sender = (struct SenderSW*)malloc(sizeof(struct SenderSW*));
	sender->LAR = -1;
	sender->LFS = -1;
	sender->SWS = window_size;
	sender->window = (struct Message**)malloc(window_size * sizeof(struct Message*));
	memset(sender->window, 0, window_size * sizeof(struct Message*));
	
	struct ReceiverSW* receiver = (struct ReceiverSW*)malloc(sizeof(struct ReceiverSW*));
	receiver->LFR = -1;
	receiver->LAF = window_size - 1;
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
	fputs("Sending this message: \n",stdout);
	print_message(message);
	printf("%d\n",sizeof(*message));
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
	if(sender->LFS < MOD(sender->LAR + 1)){
		//There's a wrap up
		if(ack_no <= sender->LFS){
			return 1;
		}else if(ack_no >= MOD(sender->LAR + 1) ){
			return 1;
		}
		return 0;
	}else{
		return (ack_no <= sender->LFS) && ((ack_no >= sender->LAR + 1));
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

int pos;
	for(pos = 0; pos < swp->sender->SWS && swp->sender->window[pos] != NULL; pos ++){
		deliver_message(swp, swp->sender->window[pos]);
	}

	//Receiving phase
	// Window was not empty. Look for receipt for a time-gap of zero seconds.
	int cumul_ack = -1;				
	struct Message* buf = (struct Message*) malloc(sizeof(struct Message));
	printf("Receive ack? \n");
	int n = receive_with_timeout(swp, buf);
	int count = 3;
	while(n < 0 && count > 0){
		printf("%d\n",n);
		n = receive_with_timeout(swp, buf);
		count -- ;
	}
	printf("n = %d \n",n);
	while(n >= 0){
		if(n > 0){
			struct Message* message = (struct Message*) malloc(sizeof(struct Message));
			memcpy(message, buf, sizeof(struct Message));
			printf("ACK: \n");
			print_message(message);
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
		printf("n = %d: \n",n);
	}

	printf("cumul_ack: %d \n",cumul_ack);
	if(cumul_ack != -1){
		//Remove all frames in the beginning	
		int last_removed;
		for(last_removed = 0; last_removed < swp->sender->SWS; last_removed ++){
			if(swp->sender->window[last_removed]->seq_no == cumul_ack){
				break;
			}
		}

		printf("%d\n",last_removed);
		int pos;
		for(pos = 0; pos < swp->sender->SWS; pos ++){
			if(pos + last_removed + 1 < swp->sender->SWS){
				swp->sender->window[pos] = swp->sender->window[pos + last_removed + 1];
			}else{
				swp->sender->window[pos] = NULL;
			}
		}

		swp->sender->LAR = cumul_ack;
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

		int count = 3;
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
	int count = 1;
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

	if(ret_val == -1){
		fputs("Ack not received.\n",stdout);
	}else if(ret_val == 0){
		fputs("Everything sent. \n",stdout);
	}
	return ret_val;

}



int is_valid_frame(struct ReceiverSW* receiver, int seq_no){
	// Checks on receiving end
	fputs("Checking validity of frame...\n",stdout);	
	printf("%d %d %d\n",receiver->LFR, receiver->LAF, seq_no);
	if(receiver->LFR > receiver->LAF){
		//Wrap up here
		return (seq_no <= receiver->LAF || seq_no >= receiver->LFR);
	}
	return (seq_no <= receiver->LAF && seq_no >= receiver->LFR);
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
				print_message(message);
				if(message->ack == -1 && is_valid_frame(swp->receiver, message->seq_no)){
					fputs("valid frame\n",stdout);
					unsigned int index;
					// Have to place at the right position
					// The last index = LAF
					// The first index = Last frame received + 1
					if ( message->seq_no < MOD(receiver->LFR+1)){
						//wrap-up
						index = receiver->RWS - (receiver->LAF - message->seq_no) - 1;
					}else{
						index = message->seq_no - receiver->LFR -1;
					}
					receiver->window[index] = message;					

					//printf("index: %d\n content:\n %s\n",index,message->content);
					printf("added to window at index = %d\n",index);
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
			printf("pos:  %d at receiver is NOT NULL\n",pos);
		}
		// pos is now at the index which has not been sent
		receiver->LFR = MOD(receiver->LFR + pos); 
		receiver->LAF = MOD(receiver->LFR + receiver->RWS);
		
		//Send ack for packet at pos-1
		struct Message* ack_message = (struct Message*) malloc(sizeof(struct Message));
		ack_message->ack = receiver->LFR;
		deliver_message(swp, ack_message);	

		int ind;
		printf("pos= %d\n",pos);
		for(ind = 0; ind < receiver->RWS; ind ++){
			if(ind < pos){
				printf("command = %s,  command_size = %d, copy this = %s, com_ind = %d\n",command, strlen(command),receiver->window[ind]->content, com_ind);
				
				memmove(command + com_ind, receiver->window[ind]->content, sizeof(char) * CONTENT_SIZE);
			printf("hi...\n");
				com_ind += CONTENT_SIZE;
				
			}
			if(ind + pos < receiver->RWS){
				receiver->window[ind] = receiver->window[ind + pos];
			}else{
				receiver->window[ind] = NULL;
			}
			if(receiver->window[ind] == NULL){
				printf("Receiver %d is NULL \n",ind);
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
		int count  = 3;
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
			printf("receiver window at %d not null\n",pos);
		}
		// pos is now at the index which has not been sent
		printf("POS:::::::::: %d \n",pos);
		receiver->LFR = MOD(receiver->LFR + pos); 
		receiver->LAF = MOD(receiver->LFR + receiver->RWS);
		
		//Send ack for packet at pos-1
		struct Message* ack_message = (struct Message*) malloc(sizeof(struct Message));
		ack_message->ack = receiver->LFR;
		deliver_message(swp, ack_message);	

		int ind;
		printf("OUTPUT STREAM ---------- till %d\n",pos);
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
		printf("OUTPUT STREAM  ENDS----------\n");

	}

	return 0;
}
