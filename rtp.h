#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/select.h>

#define PORT 5555
#define DST "127.0.0.1"

#define WINDOW_SIZE 5

#define FRAME_SIZE 544
#define DATA_SIZE 512

#define INIT 0
#define WAIT_SYN 1
#define WAIT_SYNACK 2
#define WAIT_ACK 3
#define WAIT_FINACK 4
#define CLOSED 99

#define ESTABLISHED 1337

#define SYN 420
#define ACK 421
#define FIN 422

enum QueueType {
    SENT,
    RECEIVED,
    ACKNOWLEDGEMENT
};

typedef struct {
    int flags;
    int id;
    int ack; // Which sequence number are we ACKing
    int seq;
    int windowsize;
    int crc;
    char data[DATA_SIZE];
} rtp_h;

typedef struct
{
	rtp_h *queue;
	int size;
	int count;
} queue;

typedef struct Variables {
    int is; // initial sequence
    int oldest; // oldest unacked packet
    int next; // next expected (if frame->flags < next => old packet)
    int window_size;
} Variables;

typedef struct {
    struct sockaddr_in host;
    struct sockaddr_in dest;
    struct Variables s_vars;
    struct Variables r_vars;
    int socket;
} TransmissionInfo;

int randomSeq();
void makePacket(rtp_h *frame, int seq, int ack, int flag, char* data);
int getData(TransmissionInfo *ti, rtp_h *frame, int timeout);
int sendData(TransmissionInfo *ti, rtp_h *frame);
void initState();
void teardown();

void initQueue(queue* q, int len);
void enqueue(TransmissionInfo *transmissionInfo, queue *q, rtp_h frame, enum QueueType type);
rtp_h* dequeue(queue *q);
int isQueueFull(queue *q);
int isQueueEmpty(queue *q);

// void incrementSeq(TransmissionInfo *transmissionInfo);
// int getSeq(TransmissionInfo *transmissionInfo);