#Makefile

default: server.c client.c
	gcc server2.c -w -o server
	gcc client.c -w -o client
client: client.c
	gcc client.c -w -o client
server: server.c
	gcc server2.c -w -o server
clean:
	rm client server