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
    
    struct packet packetReceived;
      
    n = recvfrom(sockfd, &packetReceived, sizeof(packetReceived), 0,(struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      perror("Error in receiving packet");
    //    fprintf(stdout, "%s\n", packetReceived.data);
    
    n = sendto(sockfd, packetReceived.data, packetReceived.size, 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
        perror("ERROR in sendto");

  }
}
