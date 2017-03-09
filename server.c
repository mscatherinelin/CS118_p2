#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
  int sockfd;
  int n;
  int portno;
  int clientlen;
  struct sockaddr_in serveraddr;
  struct sockaddr_in clientaddr;
  struct hostent *hostp;
  char buf[1024];
  char *hostaddrp;
  
  if (argc != 2){
    fprintf(stderr, "incorrect number of arguments used.");
    exit(1);
  }
  portno = atoi(argv[1]);
  
  //create UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  
  if (sockfd == -1) {
    fprintf(stderr,"Error opening socket");
    return 0;
  }
  
  //Configure setting in address struct
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons((unsigned short)portno);
  serveraddr.sin_addr.s_addr = inet_addr(INADDR_ANY);
  memset(serveraddr.sin_zero, '\0', sizeof serveraddr.sin_zero);

  //Bind socket with address struct

  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
    fprintf(stderr,"Error on binding.\n");

  //main loop wait for datagram
  clientlen = sizeof(clientaddr);
  while(1){
    bzero(buf, 1024);
    n = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      fprintf(stderr,"Error in recvfrom");

    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      fprintf(stderr,"ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      fprintf(stderr,"ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* 
     * sendto: echo the input back to the client 
     */
    n = sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      fprintf(stderr,"ERROR in sendto");
  }
}
