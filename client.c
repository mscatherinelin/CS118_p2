#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main() {
  int clientSocket, portNum, nBytes; 
  char buffer[1024];
  struct sockaddr_in serverAddr;
  socketlen_t addr_size;

  //create UDP socket
  clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
  
  //address struct
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons();
  serverAddr.sin_addr.s_addr = inet_addr();


}
