#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

int main(){

	int NewSocket;
	struct sockaddr_in ServerAddress;

	//Create a socket
	NewSocket = socket(AF_INET, SOCK_STREAM, 0);

	if(NewSocket == -1){
		printf("Creating Failed\n");
		return 0;
	}

	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(8080);

	//Connect a Socket
	int retConnect = connect(NewSocket, (struct sockaddr*)&ServerAddress, sizeof ServerAddress);

	if(retConnect == -1){
		printf("Connection Failed\n");
		return 0;
	}

	char buffer[2050];
	bool flag = false;
	memset(buffer, 0, sizeof buffer);
	int ret = read(NewSocket, buffer, sizeof buffer);

	printf("%s\n", buffer);

	while(true){
		memset(buffer, 0, sizeof buffer);

		char c;
		int i = 0;
		
		printf("Client: ");
		scanf("%c", &c);
		
		while(c != '\n'){
			buffer[i] = c;	
			i = (i+1)%2048;
			if(strlen(buffer) == 2048){
				if(strcmp(buffer, "quit") == 0){
					flag = true;
					break;	
				}
				send(NewSocket, buffer, strlen(buffer), 0);

				memset(buffer, 0, sizeof buffer);
				
				int ret1 = read(NewSocket, buffer, sizeof buffer);
				if(ret1 <= 0){
					flag = true;
					break;
				}
				
				printf("Server: %s\n",buffer);
				memset(buffer, 0, sizeof buffer);
			}
			scanf("%c", &c);	
		}
		
		if(strcmp(buffer, "quit") == 0)flag = true;
		if(flag) break;
		
		send(NewSocket, buffer, strlen(buffer), 0);
				
		memset(buffer, 0, sizeof buffer);

		int ret = read(NewSocket, buffer, sizeof buffer);
		if(ret <= 0){
			break;
		}
		printf("Server: %s\n", buffer);
	}

	close(NewSocket);

	return 0;
}
