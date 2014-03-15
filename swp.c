#include "swp.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

int add_message_to_sender(struct SenderSW* sender, char* buf, int ack){
	//Assumes that the buffer has an extra position
	struct Message message;
	if (buf != NULL)
		strcpy(message->content, buf);
	else
		message->content = NULL;
	int pos;
	for(pos = sender->SWS - 1; pos >= 0; pos--){
		if(sender->window[pos] != NULL){
			break;
		}
	}
	pos += 1;
	message->ack = ack;
	sender->LFS = (sender->LFS + 1)%sender->SWS;
	message->seq_no = sender->LFS;
	sender[pos] = message;
	return 0;
}




struct SWP * get_new_SWP(unsigned int window_size, struct sockaddr* addr, int addrlen, int sockfd){
	struct SenderSW* sender = (struct SenderSW*)malloc(sizeof(struct SenderSW*));
	sender->LAR = -1;
	sender->LFS = -1;
	sender->SWS = SWS;
	sender->window = (struct Message*)malloc(SWS * sizeof(struct Message));
	memset(sender->window, 0, SWS * sizeof(struct Message));
	
	struct SenderSW* receiver = (struct ReceiverSW*)malloc(sizeof(struct ReceiverSW*));
	recevier->LFR = -1;
	receiver->LAF = -1;
	receiver->RWS = window_size;
	receiver->window = (struct Message*)malloc(RWS * sizeof(struct Message));
	memset(receiver->window, 0, RWS * sizeof(struct Message));
	struct SWP* swp = (struct SWP*)malloc(sizeof(SWP));
	swp->addr = addr;
	swp->addrlen = addrlen;
	swp->sockfd = sockfd;
	swp->receiver = receiver;
	swp->sender = sender;
	swp->curr_size = 0;
	return swp;
}

int is_sender_window_moved(struct SWP swp){
	struct SenderSW sender = *(swp->sender);
	if(sender->top != NULL){
			//No wrap up yet
			return ((sender->LAR + sender->SWS + 1) % MAX_SEQ_NO == sender->LFS);
	}
	return 1;
}

int is_sender_empty(struct SWP swp){
	return (swp->sender->window[0] == NULL);	
	
}


int deliver_message(struct SWP* swp, struct Message* message){
	int rc = sendto(swp->sockfd, message, sizeof(struct Message), 0, (struct sockaddr *) &addr, addrlen);
	if(rc < 0){
		printf("Error sending data");
		close(sd);
		exit(1);
	}	
	
}


int is_valid_ack(struct SenderSW sender, int ack_no){
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

//This is the high-level function called to send a message
int send_messages(struct SWP swp, FILE* fp){
	int read_size = 1;
	char buf[CONTENT_SIZE];
	int ret_val;
	while(1){
		//Populating window stage
		while(is_sender_window_moved(swp) && (read_size = fread(buf, sizeof(char), CONTENT_SIZE, fp)) > 0){
			add_message_to_sender(swp->sender, buf, -1);//This is not an ack
		}

		if((ret_val = send_and_receive(swp)) != 1){
			// Nothing to send and receive
			return ret_val;
		}

	}
	return 1;
}

int send_command(char* command){
	char buf[CONTENT_SIZE];
	i = 0;
	int ret_val;
	while(1){
		//Populating window stage
		while(is_sender_window_moved(swp) && (i <= strlen(command))){
			if(i + CONTENT_SIZE > strlen(command)){
				memcpy(buf, command + i, sizeof(char) * (strlen(command) - i + 1));
				add_message_to_sender(swp->sender, buf, -1);
				i = strlen(command) + 1;
				break;
			}else{
				memcpy(buf, command + i, sizeof(char) * CONTENT_SIZE);
				add_message_to_sender((command + i)[]);
				i += CONTENT_SIZE;
			}
		}

		if((ret_val = send_and_receive(swp)) != 1){
			// Nothing to send and receive
			return ret_val;
		}
	}
}




int send_and_receive(struct SWP swp){

	//Sending messages phase

	if(swp->is_sender_empty()){
		return 0; // The window is empty. Nothing to send	
	}

	int pos;
	for(pos = 0; pos < swp->sender->SWS && swp->sender->window[pos] != NULL; pos ++){
		deliver_message(swp, swp->sender->window[pos]);
	}


	//Receiving phase
	// Window was not empty. Look for receipt for a time-gap of zero seconds.
	int cumul_ack = -1;				
	fd_set socks;
	struct timeval t;
	FD_ZERO(&socks);
	FD_SET(swp->sockfd, &socks);
	t.tv_usec = 0;
	t.tv_sec = TIMEOUT;
	int bool_1 = select(sockfd + 1, &socks, NULL, NULL, &t);
	if(bool_1){
		struct Message message;
		int n =  recvfrom(sockfd, (void*)&message, MAX_BLOCK_SIZE, 0, src_addr, (socklen_t*)len);
		while(n >= 0){
			if(n > 0){
				memcpy(buf, &message, sizeof(struct Message));
				if(message->ack != -1 && is_valid_ack(swp->sender, message->ack)){
					//Check for ack outside window!!
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
			n =  recvfrom(sockfd, (void*)&message, MAX_BLOCK_SIZE, 0, src_addr, (socklen_t*)len);
		}
	}

	if(cumul_ack != -1){
		//Remove all frames in the beginning	
		int last_removed;
		for(last_removed = 0; last_removed < swp->sender->SWS; last_removed ++){
			if(swp->sender->SWS[last_removed] == cumul_ack){
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
	}else{
		//Nothing received
		return -1;
	}
	return 1;
}

int is_valid_frame(struct SWP *swp, int seq_no){
	// Checks on receiving end
	if(swp->receiver->LFR > swp->receiver->LAF){
		//Wrap up here
		return (seq_no >= swp->receiver->LAF || seq_no <= swp->recever->LFR);
	}else{
		return (seq_no >= swp->receiver->LAF && seq_no <= swp->recever->LFR);
	}
	return -1;
}



int receive_message(struct SWP* swp, FILE *fp){
	// This SWP side has sent a command. It will have to receive a message
	// or the output and write it to fp and send acks
	struct ReceiverSW *receiver = swp->receiver;
	while(1){
		fd_set socks;
		struct timeval t;
		FD_ZERO(&socks);
		FD_SET(swp->sockfd, &socks);
		t.tv_usec = TIMEOUT;
		t.tv_sec = 0;
		int bool_1 = select(swp->sockfd + 1, &socks, NULL, NULL, &t);
		if(bool_1){
			int n =  recvfrom(swp->sockfd, buf, MAX_BLOCK_SIZE, 0, src_addr, (socklen_t*)len);
			while(n >= 0){
				if(n > 0){
					struct Message* message = (struct Message*)malloc(sizeof(struct Message));
					memcpy(buf, &message, sizeof(struct Message));
					if(message->ack == -1 && is_valid_frame(swp->sender, message->seq_no)){
						// Have to place at the right position
						// The last index = LAF
						// The first index = Last frame received + 1
						if ( message->seq_no < MOD(receiver->LFR+1)){
							//wrap-up
							index = receiver->RWS - (receiver->LAF - message->seq_no) - 1
						}else{
							index = message->seq_no - receiver->LFR;
						}
						receiver->message->window[index] = message;					
					}
				}
			}
		return 0;
		}

		// We have received for this round
		// Now send acks and write things down
		int pos;
		for(pos = 0; pos < receiver->RWS && receiver->window[pos] != NULL; pos ++);
		// pos is now at the index which has not been sent
		receiver->LFR = MOD(receiver->LFR + pos); 
		receiver->LAF = MOD(receiver->LFR + receiver->RWS);
		
		//Send ack for packet at pos-1
		struct Message ack_message;
		ack_message->ack_no = receiver->LFR;
		deliver_message(swp, ack_message);	

		int ind;
		for(ind = 0; ind < receiver->RWS; ind ++){
			if(ind < pos){
				fwrite(receiver->window[ind]->content, sizeof(char), CONTENT_SIZE, fp);
				if(ind + pos < receiver->RWS){
					receiver->window[ind] = receiver->window[pos];
				}
			}else{
				receiver->window[ind] = NULL;
			}
		}

	}
}
