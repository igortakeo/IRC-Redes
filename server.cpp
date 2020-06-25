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
#include <string>
using namespace std;


class Channel{
	
	public:
	
	//Id do canal.
	int idChannel;
	
	//Id do administador do canal.
	int idAdmin;
	
	//Numero de pessoas no canal
	int number;
	
	//Vetor que guarda o id de todos os usuários.
	vector<int>UsersChannel;		
	
	//O map guarda a relação de id do usuário com o seu respectivo ip.
	map<int, string>UsersIp;
	
	//O map Clients armazena o id do cliente e seu respectivo nickname. (Armazenar globalmente, lembrar de excluir)
	map<int, string>Clients; 
	
	void whois(int idClient){
		//Tem que retornar apenas para o Administrador do Canal
		printf("%s user ip: %s\n", Clients[idClient].c_str(), UsersIp[idClient].c_str()); // mandar para o administrador do canal
		 
	}
			
};

//O map Clients armazena a dupla id do cliente e o seu nickname.
// Esta parte será armazenada na classe channel, lembrar de apagar****
map<int, string>Clients; 

//O set armazena todos os nicknames
set<string>Nicknames;

//O vector IdClients armazena todos os ids dos clientes que estao conectados.
vector<int>IdClients; 

//A fila indica a chegada de um novo cliente.
queue<int>QueueClients;

//Conjunto de canais existentes.
set<int>SetChannels;

//O canal em que o id está conectado.
map<int, Channel>ConnectedChannel;

//Guarda a dupla idcanal e o canal.
map<int, Channel>IdChannel;

int NewServer;



void handler(int sig) {
    signal(SIGINT, handler);
	cout << "\t\rTo close server, use: Ctrl+D" << endl;
    fflush(stdout);
}

bool checkChannel(int id){
	
	//Verificando se o canal já existe.
	if(SetChannels.count(id) == 0) return false;
	return true;
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
		if(tries == 0){
			ClientQuit(i);
			close(i);
		}
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
	char message_joinchannel[200] = "Choose one channel to join\n(/list: channels list that already exist)\n(/join '#numberchannel': to join one channel)\n";
	char message_accept[50] = "Nickname accepted";
	char message_error_nickame[50] = "Nickname already exist\nInsert your Nickname: ";
	char message_emtpychannel[50] = "List empty !\n"; 
	char message_welcomechannel[50] = "Welcome to Channel #";
	char message_errorchannel[50] = "Error, type again\n";
	
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
			if(Nicknames.count(nick) == 0){
				
				//Mandando a mensagem que o nickname foi aceito
				send(NewClient, message_accept, strlen(message_accept), 0);
		
				break;
			}
			//Enviando a mensagem de erro para o cliente
			send(NewClient, message_error_nickame, strlen(message_error_nickame), 0);
			
			//Zera o buffer
			memset(buffer, 0, sizeof buffer);
			
			//Limpando o nick
			nick.clear();
		}
		
		//Zera o buffer.
		memset(buffer, 0, sizeof buffer);
		
		//Lendo a mensagem do cliente.
		read(NewClient, buffer, sizeof buffer);
		
		//Printando a mensagem.
		printf("%s", buffer);
			
		//Envia a mensagem para a escolha de um canal.
		send(NewClient, message_joinchannel, strlen(message_joinchannel), 0);
		
		string message, type, rest;
		int channel;
		
		while(true){
				
			//Zera o buffer.
			memset(buffer, 0, sizeof buffer);
			
			//Lendo a mensagem do cliente.
			int ret = read(NewClient, buffer, sizeof buffer);	
			
			//Transformando para std::string.
			message.append(buffer, buffer+strlen(buffer));
			type = message.substr(0,5);
		
			//Verificando se o cliente pediu para ver a lista de canais.
			if(message == "/list"){
				
				//Verificando se a lista esta vazia.
				if(SetChannels.empty()) 
					send(NewClient, message_emtpychannel, strlen(message_emtpychannel), 0);	
				else{
					string sendList = "";
					
					//Pegando a lista de canais.
					for(auto c : SetChannels){
						sendList += "Channel: #"+to_string(IdChannel[c].idChannel)+" -- Users Online: "+to_string(IdChannel[c].number)+'\n';
					}
					//Enviando a lista de canais para o cliente.	
					send(NewClient, sendList.c_str(), sendList.size(), 0);
				}	
			}
			else if(type == "/join" and message[6] == '#'){
				char ip[INET_ADDRSTRLEN]; 
				Channel c;
				rest = message.substr(7);
				
				//Transformando o numero do canal em int.
				channel = stoi(rest);
				
				//Pegando o ip do cliente.
				inet_ntop(AF_INET, &(SocketAddress.sin_addr), ip, INET_ADDRSTRLEN); 
				
				if(checkChannel(channel)){
				    c = IdChannel[channel];
					c.UsersChannel.push_back(NewClient);
					c.number++;
					c.UsersIp[NewClient] = ip;
					ConnectedChannel[NewClient] = c;
					IdChannel[channel] = c;
				}
				else{	
					c.idChannel = channel;
					c.idAdmin = NewClient;
					c.number = 1;
					c.UsersChannel.push_back(NewClient);
					c.UsersIp[NewClient] = ip;		
					ConnectedChannel[NewClient] = c;
					SetChannels.insert(channel);
					IdChannel[channel] = c;
				}
				
				
				string sendMessageWelcome;
				
				sendMessageWelcome.append(message_welcomechannel, message_welcomechannel+strlen(message_welcomechannel));
				
				sendMessageWelcome+=rest + '\n';
				
				//Mandando a mesnagem de boas vindas ao canal.
				send(NewClient, sendMessageWelcome.c_str(), sendMessageWelcome.size(), 0);
				
				break;
			}
			else 
				send(NewClient, message_errorchannel, strlen(message_errorchannel), 0);
			
			//Limpando as strings.	
			message.clear();
			type.clear();
			rest.clear();
		}
		
	
		//Relacionando o id do cliente com o seu nickname
		Clients[NewClient] = nick;
		
		//Guardando o id do cliente no vetor de clientes
		IdClients.push_back(NewClient);
		
		//Guardando no conjunto de nicks
		Nicknames.insert(nick);
		
		//Informando que o usuario conectou-se ao server
		cout << nick << " joined the channel #" << channel << endl;

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

