/* File: server.c
 * Trying out socket communication between processes using the Internet protocol family.
 * 
 * Author Donatello Piancazzo dpo16001 and Oskar Berglund obd16004 Alexander Andersson Tholin atn17004 2019/05/02
 * 
 * 
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include "rtp.h"

#define PORT 5555
#define MAXMSG 512

TransmissionInfo *transmissionInfo;
queue readQueue;
queue ackQueue;

void teardown() {
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);
  int state = FIN;

  while (1) {

    if (state != FIN) {
      getData(transmissionInfo, frame, 1);
    }

    switch (state) {

    case FIN:
      printf("Sending FIN + ACK\n");
      makePacket(frame, transmissionInfo->s_vars.next, transmissionInfo->r_vars.next, FIN + ACK, 0);
      sendData(transmissionInfo, frame);
      state = WAIT_ACK;
      break;

    case WAIT_ACK:
      if (frame->flags == ACK) {
        printf("ACK received, closing...\n");
        exit(EXIT_SUCCESS);
      }
      break;
    
    default:
      return;
      break;
    }
  }
}


void makeSocket() {
	transmissionInfo->host.sin_family = AF_INET;
	transmissionInfo->host.sin_port = htons(PORT);
	transmissionInfo->host.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//make a socket
	transmissionInfo->socket = socket (AF_INET, SOCK_DGRAM, 0);
  transmissionInfo->s_vars.window_size = WINDOW_SIZE;

  transmissionInfo->s_vars.is = randomSeq();
  transmissionInfo->s_vars.next = transmissionInfo->s_vars.is;
  transmissionInfo->s_vars.oldest = transmissionInfo->s_vars.is;

  if (transmissionInfo->socket == -1) {
    printf("socket error\n");
    exit(1);
  }
	
	//bind socket to address as given by the host struct in the TCB	
	bind(transmissionInfo->socket, (struct sockaddr *) &transmissionInfo->host, sizeof(transmissionInfo->host));
}

int main(int argc, const char *argv[]) { 
    transmissionInfo = (TransmissionInfo*)malloc(sizeof(TransmissionInfo));
    /* Create a socket */
    makeSocket();
  
    initState();

    return EXIT_SUCCESS;
}

void initState() {

  initQueue(&readQueue, transmissionInfo->s_vars.window_size);
  initQueue(&ackQueue, transmissionInfo->s_vars.window_size);
  int state = WAIT_SYN;
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);

  while (1) {

    getData(transmissionInfo, frame, 1);

    switch (state)
    {

    case WAIT_SYN:
      
      if (frame->flags == SYN) {

        printf("Received SYN, SEQ = %d\n", frame->seq);
        
        transmissionInfo->r_vars.is = frame->seq;
        transmissionInfo->r_vars.next = transmissionInfo->r_vars.is + 1;

        makePacket(frame, transmissionInfo->s_vars.is, transmissionInfo->r_vars.is, SYN + ACK, 0);
        printf("Sending ACK, SEQ = %d, ACK = %d\n", frame->seq, frame->ack);

        sendData(transmissionInfo, frame);
        state = WAIT_ACK;
      }
      break;

    case WAIT_ACK:
      if (frame->flags == ACK) {
        printf("ACK Received, SEQ = %d, ACK = %d\n", frame->seq, frame->ack);
        state = ESTABLISHED;
        printf("ESTABLISHED\n");
      }
      break;

    case ESTABLISHED:
      if (frame->flags == 0) {
        // Old packet
        if (frame->seq < transmissionInfo->r_vars.next) {
          printf("Old packet received, SEQ = %d, data = %s\n", frame->seq, frame->data);
        }
        // Expected packet
        else if (frame->seq == transmissionInfo->r_vars.next) {
          printf("Expected packet received, SEQ = %d, data = %s\n", frame->seq, frame->data);
          enqueue(transmissionInfo, &readQueue, *frame, RECEIVED);
          transmissionInfo->r_vars.next++;
        }
        // Future
        else if (frame->seq > transmissionInfo->r_vars.next) {
          printf("Future packet received, SEQ = %d, data = %s\n", frame->seq, frame->data);
          enqueue(transmissionInfo, &readQueue, *frame, RECEIVED);
        }

        makePacket(frame, transmissionInfo->s_vars.next, frame->seq, ACK, 0);
        printf("Sending ACK, SEQ = %d, ACK = %d, FLAG = %d\n", frame->seq, frame->ack, frame->flags);
        sendData(transmissionInfo, frame);
      }

      if (frame->flags == FIN) {
        teardown();
      }

      break;
    
    default:
      break;
    }

    sleep(1);
  }
}