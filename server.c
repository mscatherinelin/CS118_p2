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

struct packet timeoutFunc(char* buf) {
  struct packet retransmittedPacket;
  retransmittedPacket.type = 1;
  retransmittedPacket.seq = head->seq;
  memcpy(retransmittedPacket.data, buf + head->offset, head->size); 
  retransmittedPacket.size = head->size;
  return retransmittedPacket;
}

int main(int argc, char **argv) {
  int sockfd, portno, clientlen;
  struct sockaddr_in serveraddr, clientaddr;
  struct hostent *hostp;
  char *hostaddrp;
  struct packet packetReceived, packetSent, finPacket;
  char* filename;
  FILE* file;

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
  while (1) {
    //waiting for file request
    if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0) {
      perror("Error in receiving packet request");
      continue;
    }

    //Three way handshake
    if (packetReceived.type == 4){
      printf("Received SYN packet.\n");
      memset((char*)&packetSent, 0, sizeof(packetSent));
      packetSent.type = 2; //ACK
      packetSent.seq = 0;
      packetSent.ack = -1;
      if(sendto(sockfd, &packetSent, sizeof(packetSent), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
          perror("Error sending SYN ACK.\n");
      continue;
    }

    if (packetReceived.type == 0 && packetReceived.ack == -1)
        printf("Received request for file %s\n", packetReceived.data);
    filename = packetReceived.data;

    file = fopen(filename, "rb");
    if(file == NULL)
      perror("No such file exists.\n");
      
    //read file into buffer
    fseek(file, 0L, SEEK_END); //set pointer to end of file
    long file_size = ftell(file);
    printf("file size: %d\n", file_size);
    fseek(file, 0L, SEEK_SET); //set pointer to beginning of file
    char *buf = malloc(sizeof(char)* file_size);
    fread(buf, sizeof(char), file_size, file);

    fclose(file);

    int total_packets = file_size/1024 + (file_size % 1024 != 0); //add extra packet for remaining data
    int current_packet = 0;
    int current_seq = 0;
    int current_position = 0;
      
    head = malloc(sizeof(struct node));
    struct node* newPacket;

    while (current_packet < total_packets) {
        int sent = 0;
        while (sent <= 4 && current_position < file_size) {            
            //Create packet
            memset((char*)&packetSent, 0, sizeof(packetSent));
            packetSent.type = 1; //Data packet
            packetSent.seq = current_seq;
            packetSent.size = (file_size - current_position < 1024) ? file_size - current_position : 1024;
            memcpy(packetSent.data, buf + current_position, packetSent.size);
        
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
                    while (curr->next != NULL) {
                        curr = curr->next;
                    }
                    curr->next = newPacket;
                }
            }

            if(sendto(sockfd, &packetSent, sizeof(packetSent), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
                perror("Error sending data packet.\n");
           printf("Sent packet type %d with seq num %d and size %d\n",packetSent.type, packetSent.seq, packetSent.size);
           current_packet++;
           current_seq++;
           current_position += 1024;
           sent++;
        }
        
        int ackedPackets = 0;
        fd_set readSet;
        struct timespec timeout = {0,500};
 
        while (ackedPackets < sent) {	  
	  /*FD_ZERO(&readSet);
            FD_SET(sockfd, &readSet);

            if (select(sockfd + 1, &readSet, NULL, NULL, &timeout) < 0)
                perror("Error on select\n");
            else if (!FD_ISSET(sockfd, &readSet)) {
                fprintf(stdout, "Timeout\n");
                struct packet packetToSend = timeoutFunc(buf);
		if (sendto(sockfd, &packetToSend, sizeof(packetToSend), 0, (struct sockaddr*)&clientaddr, clientlen) == -1)
		    perror("Error in sending retransmitted packet\n");
		printf("Retransmitted packet with seq num %d and size %d\n", packetToSend.seq, packetToSend.size);
		ackedPackets++;
		continue;
	    }
	  */
	  
            memset((char*)&packetReceived, 0, sizeof(packetReceived));
            if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0)
                fprintf(stdout,"Error receiving packet\n");
        
            //ACK received
            if (packetReceived.type == 2) {
                fprintf(stdout, "Received packet with ack number %d\n", packetReceived.ack);
                if (head->seq == packetReceived.ack) {
                    head->ack = 1;
                    int count = 0;
                    while(head != NULL && head->ack == 1) {
                        head = head->next;
                        count++;
                    }
                    sent = sent - count;
                    fprintf(stdout, "Moving over window by %d\n", count);
                }
                else {
                    fprintf(stdout, "Not moving over window\n");
                    struct node* curr = head;
                    while (curr != NULL && curr->seq != packetReceived.ack)
                        curr = curr->next;
                    if (curr != NULL)
                        curr->ack = 1;
                }
            }
	}
    }
    //send FIN
    memset((char*)&finPacket, 0, sizeof(finPacket));
    finPacket.type = 3;
    finPacket.seq = current_seq;
    fprintf(stdout, "Sending FIN packet\n");
    if(sendto(sockfd, &finPacket, sizeof(finPacket), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
        perror("Error sending FIN packet.\n");

    //set FIN timeout
    memset((char*)&packetReceived, 0, sizeof(packetReceived));
    if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0)
        fprintf(stdout,"Error receiving packet\n");
    if(packetReceived.type == 2 && packetReceived.ack == finPacket.seq ){
      printf("Received FIN ACK with ACK number: %d\n", packetReceived.ack);
      fd_set readSetFin;
      FD_ZERO(&readSetFin);
      FD_SET(sockfd, &readSetFin); 
      struct timespec FINtimeout = {0,1000};
      if (select(sockfd + 1, &readSetFin, NULL, NULL, &FINtimeout) < 0)
          perror("Error on select\n");
      else if (!FD_ISSET(sockfd, &readSetFin)) {
          fprintf(stdout, "Connection closed\n");
      }  
    }
  }
}

