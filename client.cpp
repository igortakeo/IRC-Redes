#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h>
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <string.h> 
#include <thread>
#include <iostream>
#include <csignal>
using namespace std;

void handler(int sig) {
    signal(SIGINT, handler);
	cout << "\t\rTo quit, use: /quit or Ctrl+D" << endl;
    fflush(stdout);
}


string getIP(){
	FILE *F;
	char ip[50];
	system("wget ifconfig.me -O ip.txt > /dev/null 2>&1");
	F = fopen("ip.txt", "r");
	fscanf(F, "%s", ip);
	system("rm ip.txt");
	fclose(F);
	string aux = ip;	
	return aux;
}

void ReceiveMessages(int NewSocket){

	char message[2050];
	memset(message, 0, sizeof message);

	while(true){
		//Recebendo a mensagem do servidor
		int receive = read(NewSocket, message, sizeof message);
    	
    	//Verificando a mensagen de fechamento do servidor
    	if(strcmp(message, "/disconnect") == 0){
			printf("Server Closed\n");
			printf("Bye!\n");		
			close(NewSocket);
			exit(0);
		}
		
    	//Printando a mensagem recebida
    	if (receive > 0){
      		printf("%s\n", message);
		}
		
    	else if (receive == 0){
			break;
   	 	}
   	 	
   	 	//Limpando o buffer 
		memset(message, 0, sizeof(message));
  	}
}


int main(){
	
	signal(SIGINT, handler);
	bool pong = false;
	int NewSocket;
	struct sockaddr_in ServerAddress;
	
	//Buffer para enviar mensagens
	char buffer[4096]; 
	int ret;
	string nick, message;	
	string eofnick = "/quit";
		
	//Criando um socket
	NewSocket = socket(AF_INET, SOCK_STREAM, 0);

	//Se a funcao anterior retorna <0 o novo servidor nao pode ser criado
	if(NewSocket < 0){
		printf("Creating Failed\n");
		return 0;
	}
	
	printf("To connect to server, type it: /connect\n");
	printf("To disconnect from server or close the app, type it: /quit or use Ctrl+D\n");
	while(true){
		string input;
		getline(cin, input);
		if(cin.eof() or input == "/quit"){
			cout << "Bye!" << endl;
			return 0;
		}
		if(input == "/connect") break;
		printf("Invalid command\n");
	}
	
	ServerAddress.sin_family = AF_INET;
	
	//Para conectar localhost
	//ServerAddress.sin_port = htons(8080);
		
	//Para conectar atraves da rede 	
	ServerAddress.sin_port = htons(1048);
	//ServerAddress.sin_addr.s_addr = inet_addr("159.89.214.31");	
	ServerAddress.sin_addr.s_addr = inet_addr("127.0.0.1");	
	
	int retConnect = connect(NewSocket, (struct sockaddr*)&ServerAddress, sizeof ServerAddress);
	if(retConnect < 0){
		printf("Connection Failed\n");	
		return 0;
	}
	
	//Zerando o buffer
	memset(buffer, 0, sizeof buffer);
	
	//Recebendo a mensagem de boas vindas do servidor para testes
	ret = read(NewSocket, buffer, sizeof buffer); 
	
	//Printando a mensagem
	printf("%s", buffer); 
	
	while(true){
		
		//Recebendo o nickname do cliente.
		getline(cin, nick);
		
		//Verificando se eh um sinal de EOF (Ctrl + D).
		if(cin.eof() or nick == "/quit"){
			send(NewSocket, eofnick.c_str(), eofnick.size(), 0);
			printf("\n");
			cout << "Bye!" << endl;
			close(NewSocket);
			return 0;
		}
		
		//Verificando se o nickname eh valido.
		if(nick.size() == 0){
			printf("Nickname invalid, type again\nInsert your Nickname(less or equal 50 characters ASCII): ");
			continue;
		}
	
		//Enviando para o servidor o nickname
		send(NewSocket, nick.c_str(), nick.size(), 0); 
		
		//Limpando o buffer
		memset(buffer, 0, sizeof buffer);
		
		//Recebendo a mensagem de validacao do nickname
		ret = read(NewSocket, buffer, sizeof buffer); 
		
		//Printando a resposta do servidor
		printf("%s", buffer);
		
		//Verificando se o nickname foi aceito
		if(strcmp(buffer,"Nickname accepted") == 0){
			printf("\n");
			printf("-------------------------------------------------------------------\n");
			string joinServer = nick + " joined the server\n";
			send(NewSocket, joinServer.c_str(), joinServer.size(), 0);
			break; 
		}
	}
	
	//Zera o buffer
	memset(buffer, 0, sizeof buffer);
	
	//Lendo a mensagem pra escolha do canal
	ret = read(NewSocket, buffer, sizeof buffer);
	
	//Printando a mensagem 
	printf("%s", buffer);
	
	string ip;
	ip = getIP();
	send(NewSocket, ip.c_str(), ip.size(), 0);
	
	while(true){
		
		//Zera o buffer.
		memset(buffer, 0, sizeof buffer);
		
		//Recebendo a mensagem do cliente.	
		getline(cin, message);
		
		//Verificando se eh um sinal de EOF (Ctrl + D).
		if(cin.eof() or message == "/quit"){
			send(NewSocket, eofnick.c_str(), eofnick.size(), 0);
			printf("\n");
			cout << "Bye!" << endl;
			close(NewSocket);
			return 0;
		}
		
		//Mandando a mensagem para o servidor.
		send(NewSocket, message.c_str(), message.size(), 0);
		
		//Lendo a mensagem de validacao do servidor.	
		read(NewSocket, buffer, sizeof buffer);
		
		//Printando a mensagem.
		printf("%s", buffer);
		
		string cutMessage;
		
		//Cortando a mensagem recebida pelo servidor.
		cutMessage.append(buffer, buffer+7);
		
		//Verificando se foi possivel entrar no canal.
		if(cutMessage == "Welcome") break;

	}
	
		
	thread Receive(ReceiveMessages, NewSocket);
	Receive.detach();

	while(true){
		
		//Limpando o buffer
		memset(buffer, 0, sizeof buffer);
		
		//Caractere auxiliar
		char c;
		int i = 0;
		
		//Para sair com Ctrl+D
		if(scanf("%c", &c) == EOF){
			memset(buffer, 0, sizeof buffer);
			strcpy(buffer, "/quit");
			c = '\n';
			cout << "Bye!" << endl;
		}
		
		while(c != '\n'){
			
			buffer[i] = c;	
			
			//Colocando os caracteres na mensagem de forma a limita-la a 2048 caracteres
			i = (i+1)%2048;
			if(strlen(buffer) == 2048){
				//Enviando a mensagem
				send(NewSocket, buffer, strlen(buffer), 0);
				
				//Limpando o buffer
				memset(buffer, 0, sizeof buffer); 
			}
			//Para sair com Ctrl+D
			if(scanf("%c", &c) == EOF){
				memset(buffer, 0, sizeof buffer);
				strcpy(buffer, "/quit");
				cout << "Bye!" << endl;
				break;
			}
		}
		
		//Se o comando "/quit" for enviado, o programa deve encerrar
		if(strcmp(buffer, "/quit") == 0){
			send(NewSocket, buffer, strlen(buffer), 0);
			break;
		}
		
		send(NewSocket, buffer, strlen(buffer), 0);
				
		memset(buffer, 0, sizeof buffer);
	}

	//Fecha o socket
	close(NewSocket); 
    
	return 0;
}
