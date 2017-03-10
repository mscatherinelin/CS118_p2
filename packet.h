#define PACKET_SIZE 1024

struct packet {
  int type; //0: SYN, 1:DATA, 2: ACK, 3: FIN
  int seq; //Sequence number of packet
  int ack; 
  int size; //Data size
  char data[PACKET_SIZE];
};
