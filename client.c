#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

int main(int argc, char* argv[]) {
  int clientSocket, portno;
  char* hostname; 
  struct sockaddr_in serverAddr;
  struct hostent *server;
  int serverlen, n;
  char buf[1024];

  if (argc != 4) {
    perror("Usage: client <hostname> <port> <filename>\n");
    exit(0);
  }

  hostname = argv[1];
  portno = atoi(argv[2]);

  //create UDP socket
  clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (clientSocket < 0) {
    perror("Error creating socket.\n");
    exit(0);
  }

  //gethostbyname
  server = gethostbyname(hostname);
  if (server == NULL) {
    perror("Error: no such host\n");
    exit(0);
  }
  
  //address struct
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  bcopy((char*)server->h_addr, (char*)&serverAddr.sin_addr.s_addr, server->h_length);
  serverAddr.sin_port = htons(portno);

  //send message to server
  bzero(buf, 1024);
  printf("Enter message: ");
  fgets(buf, 1024, stdin);
  serverlen = sizeof(serverAddr);
  n = sendto(clientSocket, buf, strlen(buf), 0, &serverAddr, &serverlen);
  if (n < 0) 
    perror("Error in sendto\n");

  //get reply from server
  n = recvfrom(clientSocket, buf, strlen(buf), 0, &serverAddr, &serverlen);
  if (n < 0) 
    perror("Error in recvfrom\n");
  printf("Reply from server: %s", buf);
  return 0;
}
