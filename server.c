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
  int addrlen;
  struct sockaddr_in serveraddr;
  char buf[1024];
  
  if (argc != 2){
    perror("Incorrect number of arguments used.");
    exit(1);
  }
  portno = atoi(argv[1]);
  
  //create UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  
  if (sockfd == -1) {
    perror("Error opening socket");
    return 0;
  }

    
  //Configure setting in address struct
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(portno);
  serveraddr.sin_addr.s_addr = INADDR_ANY;
  memset(serveraddr.sin_zero, '\0', sizeof serveraddr.sin_zero);
  
  //Bind socket with address struct
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
    fprintf(stderr,"Error on binding.\n");
  
  //main loop wait for datagram
  addrlen = sizeof(serveraddr);
  
  while(1){
        bzero(buf, 1024);
    n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&serveraddr, addrlen);
    printf("msg from %s:%d (%d n)\n", inet_ntoa(serveraddr.sin_addr),ntohs(serveraddr.sin_port), n);
    sendto(sockfd, buf, n, 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  }
  close(sockfd);
}
