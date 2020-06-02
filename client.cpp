#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h>
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <string.h> 
#include <thread>
#include <iostream>
using namespace std;

//Organiza nick dos usuarios
void OverwriteStdout(){
	printf("%s", "> ");
	fflush(stdout);
}

//Thread para receber mensagens
void ReceiveMessages(int NewSocket){

	char message[2050];
	memset(message, 0, sizeof message);

	while(true){
		//Recebendo mensagem do cliente
		int receive = read(NewSocket, message, sizeof message);

		//Se a mensagem chegou
    	if (receive > 0){
			//Escreve
      		printf("%s\n", message);
			OverwriteStdout();
		}
    	else if (receive == 0){
			break;
   	 	} 
		memset(message, 0, sizeof(message));
  	}
}


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
	
	printf("To connect to server type it: /connect\n");
	while(true){
		string input;
		getline(cin, input);
		if(input == "/connect")break;
		printf("Invalid command\n");
	}

	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(8080);
	//ServerAddress.sin_addr.s_addr = inet_addr("159.89.214.31");	
	//Conectando o cliente a porta 1048
	int retConnect = connect(NewSocket, (struct sockaddr*)&ServerAddress, sizeof ServerAddress);
	if(retConnect == -1){
		printf("Connection Failed\n");	
		return 0;
	}
	
	char buffer[4096]; //buffer para enviar e receber mensagens
	int ret;
	memset(buffer, 0, sizeof buffer); //zerando o buffer
	
	string nick;
	memset(buffer, 0, sizeof buffer);
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
		
	thread Receive(ReceiveMessages, NewSocket);
	Receive.detach();

	while(true){
		
		memset(buffer, 0, sizeof buffer); //reiniciando o buffer
		
		char c; //caractere auxiliar
		int i = 0;
		scanf("%c", &c);
		OverwriteStdout();
		while(c != '\n'){
			buffer[i] = c;	
			i = (i+1)%2048; //colocando os caracteres na mensagem de forma a limita'-la a 2048 caracteres
			if(strlen(buffer) == 2048){
				send(NewSocket, buffer, strlen(buffer), 0); //enviando a mensagem
				memset(buffer, 0, sizeof buffer); // reiniciando buffer para receber a resposta do servidor
			}
			scanf("%c", &c);
		}
		
		//se a palavra quit for enviada o programa deve encerrar
		if(strcmp(buffer, "/quit") == 0) break;	
		
		send(NewSocket, buffer, strlen(buffer), 0);
				
		memset(buffer, 0, sizeof buffer);
	}


	close(NewSocket); //fecha o socket

	return 0;
}
