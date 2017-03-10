#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include "packet.h"

int main(int argc, char* argv[]) {
    int clientSocket, portno;
    char* hostname;
    struct sockaddr_in serverAddr;
    struct hostent *server;
    int serverlen, n;


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

  serverlen = sizeof(serverAddr);
    
  //generate request packet
    char* request = argv[3];
    struct packet requestPacket;
    requestPacket.type = 0;
    requestPacket.seq = 0;
    requestPacket.size = strlen(request);
    strcpy(requestPacket.data, request);
    
    n = sendto(clientSocket, &requestPacket, sizeof(requestPacket), 0, &serverAddr, serverlen);
    if (n < 0)
        perror("Error in sendto\n");
    fprintf(stdout, "Sending packet SYN\n");
    
    char buf[1024];
    //get reply from server
    n = recvfrom(clientSocket, buf, 1024, 0, &serverAddr, &serverlen);
    if (n < 0)
        perror("Error in recvfrom\n");
    printf("Reply from server: %s\n", buf);
    return 0;
}
