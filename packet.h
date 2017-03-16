#include <time.h>

struct packet {
  int type; //0: REQUEST, 1:DATA, 2: ACK, 3: FIN , 4: SYN
  int seq; //Sequence number of packet
  int ack; 
  int size; //Data size
  char data[1024];
  struct timespec timer;
  int seqInBytes;
  int ackInBytes;
};
