#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <iostream>
#include <csignal>
#include <string>
using namespace std;

//Este codigo ira mostar apenas as mensagens de um canal especifico.

void handler(int sig) {
    signal(SIGINT, handler);
    fflush(stdout);
}

int main(){
	
	signal(SIGINT, handler);
		
	int NewSocket;
	struct sockaddr_in ServerAddress;
	char buffer[2050];
	char r[2050];
	string ip;
	int port;
	
	NewSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	cin >> ip >> port;
	
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(port);
	ServerAddress.sin_addr.s_addr = inet_addr(ip.c_str());	

	//Conectando ao servidor.
	int retConnect = connect(NewSocket, (struct sockaddr*)&ServerAddress, sizeof ServerAddress);
	if(retConnect < 0){
		printf("Connection Failed\n");	
		return 0;
	}
	
	//Lendo a mensagem de boas vindas.
	read(NewSocket, buffer, sizeof buffer);
	
	printf("***All the messages are going to shown in this terminal !!***\n");
	
	//Printando a mensagem de boas vindas.
	printf("%s\n", buffer);
	
	while(true){
		//Limpando o buffer.
		memset(r, 0, sizeof r);
		
		//Lendo a mensagem enviada do servidor,
		int ret = read(NewSocket, r, sizeof r);
		
		if(strcmp(r, "/join") == 0){
			continue;
		}
		
		//Verificando se é alguma palavra chave para desconectar.
		if(ret <= 0 or strcmp(r, "/disconnect") == 0 or strcmp(r, "/quit") == 0) break;	
		
		//Printando a mensagem.
		printf("%s\n",r);
	}
	
	//Fechando o socket.
	close(NewSocket);
		
	return 0;
}
