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

#define BUFSIZE 1024

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  struct packet packetReceived, packetSent;
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

  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

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
    n = recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0){
      perror("Error in receiving packet request");
      continue;
    }

    //Check to see if the packet is a SYN
    if(packetReceived.type != 0){
      perror("Received non-request packet. Ignored\n");
      continue;
    }
    if(packetReceived.type == 2){
      printf("Received ack\n");
      exit(0);
    }

    printf("Received type:%d, seq:%d, ack:%d, size:%d\n", packetReceived.type, packetReceived.seq, packetReceived.ack, packetReceived.size);
    filename = packetReceived.data;

    file = fopen(filename, "r");
    if(file == NULL){
      perror("No such file exists.\n");
      continue;
    }
    //read file into buffer
    fseek(file, 0L, SEEK_END); //set pointer to end of file
    long file_size = ftell(file);
    fseek(file, 0L, SEEK_SET); //set pointer to beginning of file
    char *buf = malloc(sizeof(char)* file_size);
    fread(buf, sizeof(char), file_size, file);

    fclose(file);

    //generate packetSent
    memset((char*)&packetSent, 0, sizeof(packetSent));
    packetSent.type = 1;
    packetSent.seq = 0;
    packetSent.size = 1024; //hardcoded for now
    memcpy(packetSent.data, buf, packetSent.size);
    if(sendto(sockfd, &packetSent, sizeof(packetSent), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
      perror("Error sending data packet.\n");
    printf("Sent type:%d, seq:%d, ack:%d, size:%d\n",packetSent.type, packetSent.seq, packetSent.ack, packetSent.size);



    
    n = sendto(sockfd, packetReceived.data, packetReceived.size, 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
        perror("ERROR in sendto");

  }
}
