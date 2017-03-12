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
    struct packet requestPacket, receivedPacket, ackPacket;

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
    
    //Three Way Handshake
    //generate SYN
    struct packet SYN;
    SYN.type = 4;
    SYN.seq = 0;
    if (sendto(clientSocket, &SYN, sizeof(SYN), 0, &serverAddr, serverlen) < 0)
      perror("Error in three way handhake\n");

    //receive SYNACK
    while (1) {
      memset((char*)&receivedPacket, 9, sizeof(receivedPacket));
      if (recvfrom(clientSocket, &receivedPacket, sizeof(receivedPacket), 0, &serverAddr, &serverlen) < 0)
	fprintf(stdout, "SYN ACK lost\n");
      else {
	if (receivedPacket.type == 2 && receivedPacket.ack == -1)
	  break;
	else {
	  fprintf(stdout, "Error in SYN ACK received\n");
	  exit(-1);
	}
      }
    }
    
    //generate request packet
    char* request = argv[3];
    requestPacket.type = 0;
    requestPacket.seq = 0;
    requestPacket.ack = -1;
    requestPacket.size = strlen(request);
    strcpy(requestPacket.data, request);
    fprintf(stdout, "file name: %s\n", requestPacket.data);

    int expectedSeq = 0;
    
    if (sendto(clientSocket, &requestPacket, sizeof(requestPacket), 0, &serverAddr, serverlen) < 0)
        perror("Error in sendto\n");
    fprintf(stdout, "Sending packet SYN\n");
    
    //file to be written to
    fp = fopen("received.data", "wb");
    if (fp == NULL)
        perror("Error in opening file\n");
    
    //ACK to be sent over
    ackPacket.type = 2;
    ackPacket.ack = 0;
    
    //get reply from server
    while (1) {
        memset((char*)&receivedPacket, 0, sizeof(receivedPacket));
        if (recvfrom(clientSocket, &receivedPacket, sizeof(receivedPacket), 0, &serverAddr, &serverlen) < 0)
            fprintf(stdout, "Packet lost\n");
        else {
	  fprintf(stdout, "type of packet received: %d\n", receivedPacket.type);
            if (receivedPacket.seq > expectedSeq) {
                fprintf("Received packet with seq num %d, expected seq num %d. Ignored\n", receivedPacket.seq, expectedSeq);
                continue;
            }
            else if (receivedPacket.seq < expectedSeq) {
                fprintf("Received packet with seq num %d, expected seq num %d. Resent ACK\n", receivedPacket.seq, expectedSeq);
                ackPacket.ack = receivedPacket.seq;
            }
            else {
                //DATA
                if (receivedPacket.type == 1) {
		  fprintf(stdout, "Received packet with seq num %d, expected seq num %d, and size %d. Correct packet\n", receivedPacket.seq, expectedSeq, receivedPacket.size);
                    fwrite(receivedPacket.data, 1, receivedPacket.size, fp);
		    ackPacket.ack = receivedPacket.seq;
                    expectedSeq++;
                }
                //FIN
                else if (receivedPacket.type == 3) {
                    printf("Received FIN packet\n");
                    break;
                }
            }
        }
        if (sendto(clientSocket, &ackPacket, sizeof(ackPacket), 0, &serverAddr, serverlen) < 0)
            perror("Error in sending ACK\n");
        fprintf(stdout, "Sending packet with ACK number %d\n", ackPacket.ack);
    }
    //FIN received
    struct packet FIN_ACK;
    FIN_ACK.type = 2;
    FIN_ACK.ack = receivedPacket.seq;
    
    if (sendto(clientSocket, &FIN_ACK, sizeof(FIN_ACK), 0, &serverAddr, serverlen) < 0)
        perror("Error in sending FIN ACK\n");
    fprintf(stdout, "Sending FIN ACK with ACK number %d\n", FIN_ACK.ack);
    fprintf(stdout, "Connection closed\n");
    fclose(fp);
    close(clientSocket);
    return 0;
}
