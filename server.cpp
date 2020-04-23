#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

int main(){

	int NewServer, NewSocket;
	struct sockaddr_in SocketAddress;
    	int addrlen = sizeof SocketAddress;

	//Create a socket
	NewServer = socket(AF_INET, SOCK_STREAM, 0);

	if(NewServer == 0){
		printf("Creating Failed\n");
		return 0;
	}

	//Bind the socket to a IP/Port
	SocketAddress.sin_family = AF_INET;
	SocketAddress.sin_addr.s_addr = INADDR_ANY;	
	SocketAddress.sin_port = htons(8080);
	
	if(bind(NewServer, (struct sockaddr*)&SocketAddress, sizeof SocketAddress) == -1){
		printf("Bind Failed\n");
		return 0;
	}

	//Mark he socket for listening in
	if(listen(NewServer, SOMAXCONN) == -1){
		printf("Error for Listening\n");
		return 0;
	}

	//Accept Client
	NewSocket = accept(NewServer, (struct sockaddr*)&SocketAddress, (socklen_t*)&addrlen);

	if(NewSocket == -1){
		printf("Accept Failed\n");
		return 0;
	}
	
		
	char welcome[20] = "Welcome to server";
	char buffer[4096];

	send(NewSocket, welcome, strlen(welcome), 0);

	while(true){
		
		memset(buffer, 0, sizeof buffer);
		
		int ret = read(NewSocket, buffer, sizeof buffer);

		if(ret <= 0){
			break;
		}

		printf("%s\n", buffer);

		send(NewSocket, buffer, strlen(buffer), 0);
		
	}

	return 0;
}
