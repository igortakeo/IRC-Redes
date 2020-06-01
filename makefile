all:
	g++ -std=c++11 -pthread server.cpp -o server
	g++ -std=c++11 -pthread client.cpp  -o client

runs:
	./server

runc:
	./client
