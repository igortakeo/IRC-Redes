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
#include <fstream>
using namespace std;

//Salvando o ip e a porta em um arquivo .txt para a conexao do windowchannel.
void saveIPPort(string ip_connect, string port){
	ofstream MyFile("saveIPaddress.txt");
	MyFile << ip_connect << ' ' << port << endl;
	MyFile.close();
}

//Verificando se porta recebida pelo cliente.
bool verifyPort(string port){
	for(int i=0; i<port.size(); i++){
		if((int)port[i] < 48 or (int)port[i] > 57)
			return true;
	}
	return false;
}

void handler(int sig) {
    signal(SIGINT, handler);
	cout << "\t\rTo quit, use: /quit or Ctrl+D" << endl;
    fflush(stdout);
}

void ChooseChannelClient(int NewSocket){
	char buffer[4096];
	string message;
	string  eofnick = "/quit";
	
	while(true){
		
		//Zera o buffer.
		memset(buffer, 0, sizeof buffer);
		
		//Recebendo a mensagem do cliente.	
		getline(cin, message);
		
		//Verificando se eh um sinal de EOF (Ctrl + D).
		if(cin.eof() or message == "/quit"){
			send(NewSocket, eofnick.c_str(), eofnick.size(), 0);
			if(message != "/quit")printf("\n");
			cout << "Bye!" << endl;
			close(NewSocket);
			exit(0);
		}
		
		//Mandando a mensagem para o servidor.
		send(NewSocket, message.c_str(), message.size(), 0);
		
		//Lendo a mensagem de validacao do servidor.	
		read(NewSocket, buffer, sizeof buffer);
		
		string RetMessage;
		
		//Cortando a mensagem recebida pelo servidor.
		RetMessage.append(buffer, buffer+strlen(buffer));
		
		//Verificando se foi possivel entrar no canal.
		if(RetMessage == "Welcome") break;

		//Printando a mensagem.
		printf("%s", buffer);	
	}
}

void ReceiveMessages(int NewSocket){
	
	char message[2050];
	string message_channel = "/choosechannel";
	memset(message, 0, sizeof message);
	
	while(true){
		//Recebendo a mensagem do servidor
		int receive = read(NewSocket, message, sizeof message);
    
    	//Verificando a mensagen de fechamento do servidor
    	if(strcmp(message, "/disconnect") == 0){
			printf("\nServer Closed\n");
			printf("Bye!\n");		
			close(NewSocket);
			exit(0);
			continue;
		}
		
    	//Printando a mensagem recebida
    	if (receive > 0){
      		printf("%s\n", message);
		}
		
		if(strcmp(message,  "\nYou was kicked !!\n-------------------------------------------------------------------\n") == 0){
				send(NewSocket, message_channel.c_str(), message_channel.size(), 0);
				int receive = read(NewSocket, message, sizeof message);
				printf("%s\n", message);
		}
		
    	else if (receive == 0){
			break;
   	 	}
   	 	
   	 	//Limpando o buffer 
		memset(message, 0, sizeof(message));
  	}
}

void SendMessages(int NewSocket){
	char buffer[4096];	
	bool flag_barra_n= false;

	while(true){
		
		//Limpando o buffer
		memset(buffer, 0, sizeof buffer);
		
		//Caractere auxiliar
		char c;
		int i = 0;
		
		//Para sair com Ctrl+D
		if(scanf("%c", &c) == EOF){
			memset(buffer, 0, sizeof buffer);
			flag_barra_n = true;
			strcpy(buffer, "/quit");
			c = '\n';
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
				printf("\n");
				cout << "Bye!" << endl;
				return;
			}
		}
		
		//Se o comando "/quit" for enviado, o programa deve encerrar
		if(strcmp(buffer, "/quit") == 0){
			send(NewSocket, buffer, strlen(buffer), 0);
			if(flag_barra_n) printf("\n");
			cout << "Bye!" << endl;
			return;
		}
		
		send(NewSocket, buffer, strlen(buffer), 0);
				
		memset(buffer, 0, sizeof buffer);
	}	
}

int main(){
	
	signal(SIGINT, handler);
	bool pong = false;
	int NewSocket;
	struct sockaddr_in ServerAddress;	
	char buffer[4096];
	int ret;
	string nick,  ip_connect, port;
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
	
	printf("(To connect localhost use ip: 127.0.0.1)\n");
	
	while(true){		
		
		
		printf("Server IP addres: ");
		
		//Recebendo o ip para conexao.
		getline(cin, ip_connect);
		
		//Verificando se eh um sinal de EOF (Ctrl + D).
		if(cin.eof() or ip_connect == "/quit"){
			if(ip_connect != "/quit")printf("\n");
			cout << "Bye!" << endl;
			close(NewSocket);
			return 0;
		}
		
		printf("Port to connection: ");
		
		//Recebendo porta para conexao.
		getline(cin, port);
		
		//Verificando se eh um sinal de EOF (Ctrl + D).
		if(cin.eof() or port == "/quit"){
			if(port != "/quit")printf("\n");
			cout << "Bye!" << endl;
			close(NewSocket);
			return 0;
		}
		
		//Verificando porta.
		if(verifyPort(port)){
			printf("Connection Failed\n");
			continue;
		}
		
		//Declaracao da porta e ip.
		ServerAddress.sin_port = htons(stoi(port));
		ServerAddress.sin_addr.s_addr = inet_addr(ip_connect.c_str());	
		
		//Estabelecendo conexao.	
		int retConnect = connect(NewSocket, (struct sockaddr*)&ServerAddress, sizeof ServerAddress);
		if(retConnect < 0)printf("Connection Failed\n");
		else break;
	}
	
	//Funcao para salvar o ip e porta.
	saveIPPort(ip_connect, port);
	
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
			if(nick != "/quit") printf("\n");
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
	 read(NewSocket, buffer, sizeof buffer);
	
	//Printando a mensagem 
	printf("%s", buffer);
	
	thread Receive(ReceiveMessages, NewSocket);
	Receive.detach();
	
	thread SendMess(SendMessages, NewSocket);
	SendMess.join();

	//Fecha o socket
	close(NewSocket); 
    
	return 0;
}
