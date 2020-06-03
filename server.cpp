#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <thread>
#include <map>
#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include <csignal>
using namespace std;


//O map Clients armazena a dupla id do cliente e o seu nickname.
map<int, string>Clients; 
//O set armazena todos os nicknames
set<string>Nicknames;
//O vector IdClients armazena todos os ids dos clientes que estao conectados.
vector<int>IdClients; 
//A fila indica a chegada de um novo cliente
queue<int>QueueClients;
int NewServer;

void handler(int sig) {
    signal(SIGINT, handler);
	cout << "\t\rTo close server, use: Ctrl+D" << endl;
    fflush(stdout);
}

void ClientQuit(int id){

	int size = IdClients.size();
	for(int i = 0; i < size; i++){
		if(id == IdClients[i]){
			//Apagando ID do vector.
			IdClients.erase(IdClients.begin() + i); 
		}
	}

	string nick = Clients[id];
	
	//Informando que o usuario desconectou
	cout << nick << " disconnected from server" << endl; 
	
	//Apagando nick do set Nicknames.
	Nicknames.erase(nick);
	
	//Apagando do map a dupla id/nick
	Clients.erase(id);

}

//Enviando mensagens para todos os clientes
void SendMessages(string buffer){
	vector<int>AuxIdClients = IdClients;
	for(auto i : AuxIdClients){
		int tries = 5;
		while(tries){
			if(send(i, buffer.c_str(), buffer.size(), 0) == -1) tries--;
			else break;
		}
		
		if(buffer == "/disconnect") ClientQuit(i);		
		if(tries == 0) ClientQuit(i);
	}
}

void ThreadMessageClients(int id){

	char buffer[2050];	
	
	while(true){
		//Zera o buffer
		memset(buffer, 0, sizeof buffer);

		//Recebe a mensagem enviada pelo cliente
		int ret  = read(id, buffer, sizeof buffer); 
		
		if(ret <= 0) break;
		if(strcmp(buffer, "/ping") == 0){
			string pong = "pong";
			printf("Sending pong to %s\n", Clients[id].c_str());
			send(id, pong.c_str(), pong.size(), 0); 	
			continue;
		}
		if(strcmp(buffer, "/quit") == 0){
			ClientQuit(id);
			break;
		}
		
		 //Escreve a mensagem recebida
		printf("%s: %s\n", Clients[id].c_str(), buffer);
		
		//Inserindo o nick do cliente que mandou a mensagem
		string NewBuffer = Clients[id]+ ": ";
		
		for(int i=0; i<strlen(buffer); i++) NewBuffer += buffer[i];
		
		//Reenvia a mensagem  para os clientes
		SendMessages(NewBuffer);
	}		
}	

void MessageClients(){
	while(true){
		if(QueueClients.empty()) continue;
		int NewClient = QueueClients.front();
		QueueClients.pop();
		thread TClient(ThreadMessageClients, NewClient);
		TClient.detach();
	}
}

void AcceptClient(int NewServer, struct sockaddr_in SocketAddress, int addrlen){
	char message_welcome[50] = "Welcome to server\nInsert your Nickname: ";
	char message_accept[50] = "Nickname accepted";
	char message_error_nickame[50] = "Nickname already exist\nInsert your Nickname: ";
	char buffer[2050];
	while(true){
		
		//Zera o buffer
		memset(buffer, 0, sizeof buffer);
		
		//Aceita o cliente
		int NewClient = accept(NewServer, (struct sockaddr*)&SocketAddress, (socklen_t*)&addrlen);
		
		//Se falhar, retorna
		if(NewClient == -1){
			printf("Accept Failed\n");
			continue;
		}
		
		//Envia a mensagem de boas vindas ao cliente
		send(NewClient, message_welcome, strlen(message_welcome), 0);
			
		string nick;
		
		while(true){
			//Lendo nickname do cliente
			int ret = read(NewClient, buffer , sizeof buffer);
			
			//Transformando pra std::string
			nick.append(buffer, buffer+strlen(buffer));
			
			//Verificando se o nickname ja existe	
			if(Nicknames.count(nick) == 0) break;
			
			//Enviando a mensagem de erro para o cliente
			send(NewClient, message_error_nickame, strlen(message_error_nickame), 0);
		}	
	
		//Relacionando o id do cliente com o seu nickname
		Clients[NewClient] = nick;
		
		//Guardando o id do cliente no vetor de clientes
		IdClients.push_back(NewClient);
		
		//Guardando no conjunto de nicks
		Nicknames.insert(nick);
		
		//Informando que o usuario conectou-se ao server
		cout << nick << " joined the server" << endl;

		//Mandando a mensagem que o nickname foi aceito
		send(NewClient, message_accept, strlen(message_accept), 0);
		
		//Inserindo o cliente na fila
		QueueClients.push(NewClient);	
	}
	
}

void MessagesStdin(){
	string s;
	while(true){
		
		//Verificando se existe alguma mensagem do servidor
		getline(cin, s);
		
		//Verificando se foi dado o sinal de EOF(Ctrl + D)
		if(cin.eof()){
			//Mensagem para fechar o servidor
			printf("Closing Server...\n");
			
			//Enviando a mensagem para os clientes disconectarem
			SendMessages("/disconnect");
			
			//Fechando o servidor
			close(NewServer);
			
			break;
		}
		else printf("Invalid Comand\n");	
	}
	
	exit(0);
}


int main(){
	
	signal(SIGINT, handler);
	
	struct sockaddr_in SocketAddress;
	int addrlen = sizeof SocketAddress;

	//Cria um socket
	NewServer = socket(AF_INET, SOCK_STREAM, 0);

	if(NewServer == 0){
		printf("Creating Failed\n");
		return 0;
	}
	
	//Vincula o socket a porta 8080
	SocketAddress.sin_family = AF_INET;
	SocketAddress.sin_addr.s_addr = INADDR_ANY;
	SocketAddress.sin_port = htons(8080);
	
	//Se falhar, retorna
	if(bind(NewServer, (struct sockaddr*)&SocketAddress, sizeof SocketAddress) == -1){
		printf("Bind Failed\n");
		return 0;
	}
	
	//Mensagem de sucessdo na abertura do servidor
	printf("Open Server (To close server, use: Ctrl + D)\n");

	//Aguarda a entrada de um cliente
	if(listen(NewServer, SOMAXCONN) == -1){
		printf("Error for Listening\n");
		return 0;
	}
	
	thread ThreadAccept(AcceptClient, NewServer, SocketAddress, addrlen);
	thread ThreadMessages(MessageClients);
	thread ThreadMessagesStdin(MessagesStdin);
	ThreadAccept.join();
	ThreadMessages.join();
	ThreadMessagesStdin.join();
	
	return 0;
}
