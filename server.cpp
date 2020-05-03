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

	//Cria um socket
	NewServer = socket(AF_INET, SOCK_STREAM, 0);

	if(NewServer == 0){
		printf("Creating Failed\n");
		return 0;
	}

	//Vincula o socket 'a porta 8080
	SocketAddress.sin_family = AF_INET;
	SocketAddress.sin_addr.s_addr = INADDR_ANY;	
	SocketAddress.sin_port = htons(8080);
	
	//Se falhar, retorna
	if(bind(NewServer, (struct sockaddr*)&SocketAddress, sizeof SocketAddress) == -1){
		printf("Bind Failed\n");
		return 0;
	}

	//Aguarda a entrada de um cliente
	if(listen(NewServer, SOMAXCONN) == -1){
		printf("Error for Listening\n");
		return 0;
	}

	//Aceita o cliente
	NewSocket = accept(NewServer, (struct sockaddr*)&SocketAddress, (socklen_t*)&addrlen);

	//Se falhar, retorna
	if(NewSocket == -1){
		printf("Accept Failed\n");
		return 0;
	}
	
	//Envia mensagem de boas vindas ao cliente
	char welcome[20] = "Welcome to server";
	char buffer[2050]; //Buffer para escrever e retornar as mensagens ao cliente

	send(NewSocket, welcome, strlen(welcome), 0);

	while(true){
		
		memset(buffer, 0, sizeof buffer); //Zera o buffer
		
		int ret = read(NewSocket, buffer, sizeof buffer); //Recebe a mensagem enviada pelo cliente

		if(ret <= 0){
			break;
		}

		printf("Client: %s\n", buffer); //Escreve a mensagem recebida

		send(NewSocket, buffer, strlen(buffer), 0); //A reenvia para o cliente
	}

	return 0;
}
