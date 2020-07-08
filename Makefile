all:
	g++ -std=c++11 -pthread server.cpp -o server
	g++ -std=c++11 -pthread client.cpp  -o client
	g++ -std=c++11 windowchannel.cpp -o windowchannel

runs:
	#gnome-terminal -e 'sh -c "echo Establishing connection...; ssh -R 10048:localhost:1048 serveo.net"' > /dev/null 2>&1
	./server
runc:
	./client

clean:
	rm server client windowchannel	
