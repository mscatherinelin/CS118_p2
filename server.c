#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "packet.h"

typedef struct node {
  int seq;
  int ack;
  int offset;
  int size;
  struct node* next;
};

struct node* head;

int nextSeqNum(int currSeq, int offset) {
    if (offset != 1024) {
        return ((currSeq % 30)+1 * 1024) + offset;
    }
    if (currSeq == 0)
      return (currSeq+1) * 1024;
    else {
        if (currSeq % 30 == 0)
            return 30720;
        return ((currSeq % 30)+1) * 1024;
    }
}

int main(int argc, char **argv) {
  int sockfd, portno, clientlen;
  struct sockaddr_in serveraddr, clientaddr;
  struct hostent *hostp;
  char *hostaddrp;
  struct packet packetReceived, packetSent, finPacket;
  char* filename;
  FILE* file;
  fd_set readSet;
    
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    perror("ERROR opening socket");

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    perror("ERROR on binding");

  clientlen = sizeof(clientaddr);

 START:
  //waiting for SYN
  while(1){
    memset((char*)&packetReceived, 0, sizeof(packetReceived));
    if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0)
      perror("Error in receiving SYN\n");
    if (packetReceived.type == 4)
        break;
  }
    
  //send SYN ACK
  fprintf("Received packet %d\n" packetReceived.ackInBytes);
  memset((char*)&packetSent, 0, sizeof(packetSent));
  packetSent.type = 2; //ACK
  packetSent.seq = 0;
  packetSent.ack = -1;
  if(sendto(sockfd, &packetSent, sizeof(packetSent), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
    perror("Error sending SYN ACK.\n");
  fprintf(stdout, "Sending packet 0 5120 SYN\n");
    
  struct timespec requestTimeout = {3, 0};
  struct packet** timers = malloc(5*sizeof(struct packet*));
    
  while (1) {
    FD_ZERO(&readSet);
    FD_SET(sockfd, &readSet);
    
    if (select(sockfd + 1, &readSet, NULL, NULL, &requestTimeout) < 0)
        perror("Error on select\n");
    else if (!FD_ISSET(sockfd, &readSet)) {
        //resend SYN ACK
        if(sendto(sockfd, &packetSent, sizeof(packetSent), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
            perror("Error sending SYN ACK.\n");
        fprintf(stdout, "Sending packet 0 5120 Retransmission\n");
        continue;
    }

    memset((char*)&packetReceived, 0, sizeof(packetReceived));
    if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0) 
      perror("Error in receiving packet request");

    if (packetReceived.type == 0 && packetReceived.ack == -1)
        filename = packetReceived.data;

    file = fopen(filename, "rb");
    if(file == NULL)
      perror("No such file exists.\n");
      
    //read file into buffer
    fseek(file, 0L, SEEK_END); //set pointer to end of file
    long file_size = ftell(file);
    fseek(file, 0L, SEEK_SET); //set pointer to beginning of file
    char *buf = malloc(sizeof(char)* file_size);
    fread(buf, sizeof(char), file_size, file);

    fclose(file);

    int total_packets = file_size/1024 + (file_size % 1024 != 0); //add extra packet for remaining data
    int current_packet = 0;
    int current_seq = 0;
    int current_seq_in_bytes = 0;
    int current_position = 0;
      
    head = malloc(sizeof(struct node));
    struct node* newPacket;

    while (current_packet < total_packets) {
        int sent = 0;
	    int i = 0;
        while (sent <= 4 && current_position < file_size) {            
            //Create packet
            memset((char*)&packetSent, 0, sizeof(packetSent));
            packetSent.type = 1; //Data packet
            packetSent.seq = current_seq;
            packetSent.seqInBytes = current_seq_in_bytes;
            packetSent.size = (file_size - current_position < 1024) ? file_size - current_position : 1024;
            memcpy(packetSent.data, buf + current_position, packetSent.size);

            //Add packet with timer to array
            struct packet* packetWithTimer = malloc(sizeof(struct packet));
            packetWithTimer->seq = packetSent.seq;
            packetWithTimer->seqInBytes = packetSent.seqInBytes;
            packetWithTimer->type = 1;
            packetWithTimer->size = packetSent.size;
            memcpy(packetWithTimer->data, packetSent.data, packetSent.size);
            clock_gettime(CLOCK_REALTIME, &(packetWithTimer->timer));
            timers[i] = packetWithTimer;

            if (current_seq == 0) {
                head->seq = packetSent.seq;
                head->ack = 0;
                head->offset = 0;
                head->size = packetSent.size;
                head->next = NULL;
            }
            else {
                newPacket = malloc(sizeof(struct node));
                newPacket->seq = packetSent.seq;
                newPacket->ack = 0;
                newPacket->offset = current_position;
                newPacket->size = packetSent.size;
                newPacket->next = NULL;
                if (head == NULL)
                    head = newPacket;
                else {
                    struct node* curr = head;
                while (curr->next != NULL)
                    curr = curr->next;
                curr->next = newPacket;
                }
            }

            if(sendto(sockfd, &packetSent, sizeof(packetSent), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
                perror("Error sending data packet.\n");
            fprintf(stdout, "Sending packet %d 5120\n", packetSent.seqInBytes);
            current_packet++;
            current_position += 1024;
            current_seq_in_bytes = nextSeqNum(current_seq, packetSent.size);
            current_seq++;
            sent++;
            i++;
        }
        
        int ackedPackets = 0;
        struct timespec timeout = {0,500};
 
        while (ackedPackets < sent) {	 
            struct timespec endTime;
            clock_gettime(CLOCK_REALTIME, &endTime);
            int i;
            for (i = 0; i < 5; i++) {
                if (timers[i] != NULL) {
                    int diff = endTime.tv_sec - timers[i]->timer.tv_sec;
                    if (diff >= 0.5) {
                        if (sendto(sockfd, timers[i], sizeof(*timers[i]), 0, (struct sockaddr*)&clientaddr, clientlen) == -1)
                            perror("Error in retransmitted packet\n");
                        fprintf(stdout, "Sending packet %d 5120 Retransmission\n", timers[i]->seqInBytes);
                    }
                    timers[i]->timer = endTime;
                }
            }
            FD_ZERO(&readSet);
            FD_SET(sockfd, &readSet);

            if (select(sockfd + 1, &readSet, NULL, NULL, &timeout) < 0)
                perror("Error on select\n");
            else if (!FD_ISSET(sockfd, &readSet)) {
                continue;
            }
	  
            memset((char*)&packetReceived, 0, sizeof(packetReceived));
            if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0)
                fprintf(stdout,"Error receiving packet\n");
        
            //ACK received
            if (packetReceived.type == 2) {
                fprintf(stdout, "Receiving packet %d\n", packetReceived.ackInBytes);
                if (head->seq == packetReceived.ack) {
                    head->ack = 1;
                    int count = 0;
                    while(head != NULL && head->ack == 1) {
                        head = head->next;
                        count++;
                    }
                    sent = sent - count;
                }
                else {
                    struct node* curr = head;
                    while (curr != NULL && curr->seq != packetReceived.ack)
                        curr = curr->next;
                    if (curr != NULL)
                        curr->ack = 1;
                }
                for (i = 0; i < 5; i++) {
                    if (timers[i] != NULL && timers[i]->seq == packetReceived.ack) {
                        timers[i] = NULL;
                    }
                }
            }
        }
    }
    //send FIN
    memset((char*)&finPacket, 0, sizeof(finPacket));
    finPacket.type = 3;
    finPacket.seq = current_seq;
    finPacket.seqInBytes = current_seq_in_bytes;
    fprintf(stdout, "Sending packet %d 5120 FIN\n", finPacket.seqInBytes);
    if(sendto(sockfd, &finPacket, sizeof(finPacket), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
        perror("Error sending FIN packet.\n");
    
    //waiting for FIN ACK
    int n = 0;
    while (1) {
        fd_set readSetFin;
        FD_ZERO(&readSetFin);
        FD_SET(sockfd, &readSetFin);
        struct timespec FINtimeout = {0,500};
        if (select(sockfd + 1, &readSetFin, NULL, NULL, &FINtimeout) < 0)
            perror("Error on select\n");
        else if (!FD_ISSET(sockfd, &readSetFin) && n < 5) {
            if(sendto(sockfd, &finPacket, sizeof(finPacket), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
                perror("Error sending FIN packet.\n");
            fprintf(stdout, "Sending packet %d 5120 FIN Retransmission\n", finPacket.seqInBytes);
            n++;
        }
        else {
            if(packetReceived.type == 2 && packetReceived.ack == finPacket.seq)
                printf("Receiving packet %d\n", packetReceived.ackInBytes);
            sleep(0.5);
            fprintf(stdout, "Connection closed\n");
            break;
        }
    }
    int i;
    for (i = 0; i < 5; i++)
        timers[i] = NULL;
    goto START;
  }
}

