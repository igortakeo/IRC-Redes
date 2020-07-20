all:
	g++ -std=c++11 -pthread server.cpp -o server
	g++ -std=c++11 -pthread client.cpp  -o client
	g++ -std=c++11 windowchannel.cpp -o windowchannel

runs:
	./server
runc:
	./client

clean:
	rm server client windowchannel saveIPaddress.txt	
