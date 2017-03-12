#define PACKET_SIZE 1024

struct packet {
  int type; //0: REQUEST, 1:DATA, 2: ACK, 3: FIN , 4: SYN
  int seq; //Sequence number of packet
  int ack; 
  int size; //Data size
  char data[PACKET_SIZE];
};
