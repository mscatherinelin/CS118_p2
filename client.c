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
    FILE* fp;


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
    
    int expectedSeq = 0;
    
    if (sendto(clientSocket, &requestPacket, sizeof(requestPacket), 0, &serverAddr, serverlen) < 0)
        perror("Error in sendto\n");
    fprintf(stdout, "Sending packet SYN\n");
    
    //file to be written to
    fp = fopen(request, "w+");
    if (fp == NULL)
        perror("Error in opening file\n");
    
    //ACK to be sent over
    struct packet ackPacket;
    ackPacket.type = 2;
    ackPacket.ack = 1;
    
    //get reply from server
    struct packet receivedPacket;
    if (recvfrom(clientSocket, &receivedPacket, sizeof(receivedPacket), 0, &serverAddr, &serverlen) < 0)
        fprintf(stdout, "Packet lost\n");
    else {
        if (receivedPacket.seq > expectedSeq) ;
        else if (receivedPacket.seq < expectedSeq) {
            ackPacket.ack = receivedPacket.seq;
        }
        else {
            //DATA
            if (receivedPacket.type == 1) {
                fprintf(stdout, "Received packet %d\n", receivedPacket.seq);
                fwrite(receivedPacket.data, 1, receivedPacket.size, fp);
                expectedSeq++;
            }
            //FIN
            else if (receivedPacket.type == 3) {
                
            }
        }
    }
    fprintf(stdout, "Sending packet %d\n", ackPacket.ack);
    if (sendto(clientSocket, &ackPacket, sizeof(ackPacket), 0, &serverAddr, serverlen) < 0)
        perror("Error in sending ACK\n");
    return 0;
}
