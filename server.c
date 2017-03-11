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
    if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0) {
      perror("Error in receiving packet request");
      continue;
    }
    
    if (packetReceived.type == 0)
        printf("Received request for file %s\n", packetReceived.data);
    filename = packetReceived.data;

    file = fopen(filename, "rb");
    if(file == NULL){
      perror("No such file exists.\n");
      continue;
    }
      
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

    while (current_packet < total_packets) {
        
      //Create packet
      memset((char*)&packetSent, 0, sizeof(packetSent));
      packetSent.type = 1; //Data packet
      packetSent.seq = current_seq;
      packetSent.size = (file_size - current_position < 1024) ? file_size - current_position : 1024; 
      memcpy(packetSent.data, buf + current_position, packetSent.size);

      if(sendto(sockfd, &packetSent, sizeof(packetSent), 0, (struct sockaddr *)&clientaddr,clientlen) == -1)
        perror("Error sending data packet.\n");
      printf("Sent type:%d, seq:%d, size:%d\n",packetSent.type, packetSent.seq, packetSent.ack, packetSent.size);
      current_packet++;
      current_seq++;
      current_position+= 1024;
        
      memset((char*)&packetReceived, 0, sizeof(packetReceived));
      if (recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen) < 0)
          fprintf(stdout,"Error receiving packet\n");
        
      if (packetReceived.type == 2)
          fprintf(stdout, "Received packet with ack number %d\n", packetReceived.ack);
      else if (packetReceived.type == 3) {
            
        }

    }
  }
}
