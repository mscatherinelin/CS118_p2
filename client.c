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
    fd_set readSet;
    struct timespec timeout = {5,0};

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

    //Send SYN
    struct packet SYN;
    SYN.type = 4;
    SYN.seq = 0;
    if (sendto(clientSocket, &SYN, sizeof(SYN), 0, &serverAddr, serverlen) < 0)
      perror("Error in three way handhake\n");

    //receive SYNACK
    struct timespec SYNACKtimeout = {3, 0};
    while (1) {
        FD_ZERO(&readSet);
        FD_SET(clientSocket, &readSet);
        
        if (select(clientSocket + 1, &readSet, NULL, NULL, &SYNACKtimeout) < 0)
            perror("Error on select\n");
        else if (!FD_ISSET(clientSocket, &readSet))
            continue;
        if (recvfrom(clientSocket, &receivedPacket, sizeof(receivedPacket), 0, &serverAddr, &serverlen) < 0)
            perror("Error in receiving SYN ACK\n");
        if (receivedPacket.type == 2 && receivedPacket.ack == -1) {
            break;
        }
    }
    
    //generate request packet
    char* request = argv[3];
    requestPacket.type = 0;
    requestPacket.seq = 0;
    requestPacket.ack = -1;
    requestPacket.size = strlen(request);
    strcpy(requestPacket.data, request);
    
    int expectedSeq = 0;
    struct packet** buf = malloc(5*sizeof(struct packet*));
    int i;
    for (i = 0; i < 5; i++)
      buf[i] = NULL;
    int bufCount = 0;

    if (sendto(clientSocket, &requestPacket, sizeof(requestPacket), 0, &serverAddr, serverlen) < 0)
        perror("Error in sendto\n");
    fprintf("Sending packet 0 \n");
    
    //file to be written to
    fp = fopen("received.data", "wb");
    if (fp == NULL)
        perror("Error in opening file\n");
    
    //ACK to be sent over
    ackPacket.type = 2;
    ackPacket.ack = 0;
    ackPacket.ackInBytes = 0;
    
    //get reply from server
    while (1) {
        memset((char*)&receivedPacket, 0, sizeof(receivedPacket));
        if (recvfrom(clientSocket, &receivedPacket, sizeof(receivedPacket), 0, &serverAddr, &serverlen) < 0) {
            perror("Error in packet received\n");        }
        
        //SYN ACK --> lost request
        if (receivedPacket.type == 2 && receivedPacket.ack == -1) {
            if (sendto(clientSocket, &requestPacket, sizeof(requestPacket), 0, &serverAddr, serverlen) < 0)
                perror("Error in sendto\n");
            fprintf(stdout, "Sending packet 0 Retransmission\n");
            continue;
        }
        //FIN
        if (receivedPacket.type == 3) {
            printf("Receiving packet %d\n", receivedPacket.seqInBytes);
            break;
        }
        
        //DATA
        if (receivedPacket.type == 1) {
            fprintf(stdout, "Receiving packet %d\n", receivedPacket.seqInBytes);
            int min = expectedSeq;
            int max = expectedSeq + 4;
            int prevWinMin = expectedSeq - 5;
            int prevWinMax = expectedSeq - 1;
            if (prevWinMax < 0)
                prevWinMax = 4;
            if (prevWinMin < 0)
                prevWinMin = 0;
            
            //write to file if in order
            if (receivedPacket.seq == expectedSeq) {
                fwrite(receivedPacket.data, 1, receivedPacket.size, fp);
                ackPacket.ack = receivedPacket.seq;
                ackPacket.ackInBytes = receivedPacket.seqInBytes;
                expectedSeq++;
                
                for (i = 0; i < 5; i++) {
                    if (buf[i] != NULL && buf[i]->seq == expectedSeq) {
                        fwrite(buf[i]->data, 1, buf[i]->size, fp);
                        buf[i] = NULL;
                        i = -1;
                        expectedSeq++;
                    }
                }
            //adjust buffer
            struct packet** buf2 = malloc(5*sizeof(struct packet*));
            bufCount = 0;
            for (i = 0; i < 5; i++)
                buf2[i] = NULL;
            for(i = 0; i < 5; i++) {
                if (buf[i] != NULL) {
                    buf2[bufCount] = buf[i];
                    bufCount++;
                }
            }
            buf = buf2;
            }
        //buffer if in window
        else if (receivedPacket.seq > min && receivedPacket.seq <= max) {
            if (bufCount < 5) {
                struct packet* packetToBuffer = malloc(sizeof(struct packet));
                packetToBuffer->seq = receivedPacket.seq;
                packetToBuffer->size = receivedPacket.size;
                memcpy(packetToBuffer->data, receivedPacket.data, receivedPacket.size);
                    buf[bufCount]= packetToBuffer;
                    bufCount++;
                    ackPacket.ack = receivedPacket.seq;
                    ackPacket.ackInBytes = receivedPacket.seqInBytes;
                }
            }
            //send ACK if in previous window
            else if (receivedPacket.seq < expectedSeq) {
                ackPacket.ack = receivedPacket.seq;
                ackPacket.ackInBytes = receivedPacket.seqInBytes;
            }
            else
                continue;
            if (sendto(clientSocket, &ackPacket, sizeof(ackPacket), 0, &serverAddr, serverlen) < 0)
                perror("Error in sending ACK\n");
            fprintf(stdout, "Sending packet %d\n", ackPacket.ackInBytes);
        }
    }
    
    //FIN received
    struct packet FIN_ACK;
    FIN_ACK.type = 2;
    FIN_ACK.ack = receivedPacket.seq;
    FIN_ACK.ackInBytes = receivedPacket.seqInBytes;
    
    if (sendto(clientSocket, &FIN_ACK, sizeof(FIN_ACK), 0, &serverAddr, serverlen) < 0)
        perror("Error in sending FIN ACK\n");
    fprintf(stdout, "Sending packet %d FIN\n", FIN_ACK.ackInBytes);
    fprintf(stdout, "Connection closed\n");
    fclose(fp);
    close(clientSocket);
    return 0;
}
