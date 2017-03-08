#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

int main(){
  int sd, connfd, len, bytes_read;
  
  struct sockaddr_in serverAddr, clientAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size, client_addr_size;
  
  //create UDP socket
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  
  if (sd == -1) {
    printf("socket not created in server");
    return 0;
  }
  
  //Configure setting in address struct
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(argv[1]);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  //Bind socket with address struct
  bind(sd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));






}
