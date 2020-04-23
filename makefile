all:
	gcc server.cpp -ansi -o server
	gcc client.cpp -ansi -o client

runs:
	./server

runc:
	./client
