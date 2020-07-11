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
#define DEBUG1 0
using namespace std;

//Este codigo ira mostar apenas as mensagens de um canal especifico.

int main(){

	int NewSocket;
	struct sockaddr_in ServerAddress;
	char buffer[2050];
	char r[2050];
	
	NewSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(1048);
	//ServerAddress.sin_addr.s_addr = inet_addr("159.89.214.31");	
	ServerAddress.sin_addr.s_addr = inet_addr("187.7.183.130");	
	
	
	//Conectando ao servidor.
	int retConnect = connect(NewSocket, (struct sockaddr*)&ServerAddress, sizeof ServerAddress);
	if(retConnect < 0){
		printf("Connection Failed\n");	
		return 0;
	}
	
	//Lendo a mensagem de boas vindas.
	read(NewSocket, buffer, sizeof buffer);
	
	//Printando a mensagem de boas vindas.
	printf("%s", buffer);
	
	while(true){
		//Limpando o buffer.
		memset(r, 0, sizeof r);
		
		//Lendo a mensagem enviada do servidor,
		int ret = read(NewSocket, r, sizeof r);
		
		//Verificando se é alguma palavra chave para desconectar.
		if(ret <= 0 or strcmp(r, "/disconnect") == 0 or strcmp(r, "/quit") == 0) break;	
		
		//Printando a mensagem.
		printf("%s\n",r);
	}
	
	//Fechando o socket.
	close(NewSocket);
		
	return 0;
}
