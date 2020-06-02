#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h>
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <string.h> 
#include <iostream>
using namespace std;

int main(){

	int NewSocket;
	struct sockaddr_in ServerAddress;

	//Criando um socket
	NewSocket = socket(AF_INET, SOCK_STREAM, 0);

	//Se a funcao anterior retorna -1 o novo servidor nao pode ser criado
	if(NewSocket == -1){
		printf("Creating Failed\n");
		return 0;
	}

	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(5000);
	ServerAddress.sin_addr.s_addr = inet_addr("159.89.214.31");	
	//Conectando o cliente a porta 8080
	int retConnect = connect(NewSocket, (struct sockaddr*)&ServerAddress, sizeof ServerAddress);

	if(retConnect == -1){
		printf("Connection Failed\n");
		return 0;
	}
	
	
	string nick;	
	char buffer[4096]; //buffer para enviar e receber mensagens
	bool flag = false;
	int ret;
	memset(buffer, 0, sizeof buffer); //zerando o buffer
	
	ret = read(NewSocket, buffer, sizeof buffer); //recebendo a mensagem de boas vindas do servidor para testes
	printf("%s\n", buffer); // printando a mensagem
	
	
	while(true){
		cin >> nick;
		send(NewSocket, nick.c_str(), nick.size(), 0); //Enviando para o servido o nickname
		scanf("%*c");	
		memset(buffer, 0, sizeof buffer); //zerando o buffer	
		ret = read(NewSocket, buffer, sizeof buffer); //recebendo a mensagem de boas vindas do servidor para testes
		printf("%s\n", buffer);
		if(strcmp(buffer,"Nickname accepted") == 0) break; // Verificando se o Nickname foi aceito
	}
	
	while(true){
		
		memset(buffer, 0, sizeof buffer); //reiniciando o servidor

		char c; //caractere auxiliar
		int i = 0;
		printf("Client: ");
		scanf("%c", &c);
		
		while(c != '\n'){
			buffer[i] = c;	
			i = (i+1)%2048; //colocando os caracteres na mensagem de forma a limita'-la a 2048 caracteres
			if(strlen(buffer) == 2048){
				//se a palavra quit for enviada o programa deve encerrar
				if(strcmp(buffer, "quit") == 0){
					flag = true;
					break;	
				}
				send(NewSocket, buffer, strlen(buffer), 0); //enviando a mensagem

				memset(buffer, 0, sizeof buffer); // reiniciando buffer para receber a resposta do servidor
				
				int ret1 = read(NewSocket, buffer, sizeof buffer); //le a resposta
				if(ret1 <= 0){
					flag = true;
					break;
				}
				printf("Server: %s\n",buffer); //escreve a resposta do servidor
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

	close(NewSocket); //fecha o socket

	return 0;
}
