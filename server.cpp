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
#include <algorithm>
using namespace std;

//Declaracoes das funcoes.
void handler(int sig);
void ClientQuit(int id);
bool checkChannel(int id);
void SendMessages(string buffer, int channel);
void ThreadMessageClients(int id);
void MessageClients();
void AcceptClient(int NewServer, struct sockaddr_in SocketAddress, int addrlen);
void MessagesStdin();

//O map Clients armazena a dupla id do cliente e o seu nickname.
map<int, string>Clients; 

//O map Clients armazen a dupla nickname e id do cliente.
map<string, int>ClientsReverse;

//O set armazena todos os nicknames
set<string>Nicknames;

//O vector IdClients armazena todos os ids dos clientes que estao conectados.
vector<int>IdClients; 

//A fila indica a chegada de um novo cliente.
queue<int>QueueClients;


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
	
	//O map guarda a relacao de id do usuario e se ele esta mutado ou nao.
	map<int, bool>UsersMute;
		
	string whois(int id){
		string sendIP = Clients[id]+" user ip: "+UsersIp[id];
		return sendIP;
	}
	
	void kick(int id){
		if(id == idAdmin){
			string errorAdmin = "Error, command not allowed !!";
			send(id, errorAdmin.c_str(), errorAdmin.size(), 0);		
		}
		else{ 
			string messageDisconnect = "/disconnect";
			send(id, messageDisconnect.c_str(), messageDisconnect.size(), 0);
			ClientQuit(id);
		}	
	}
	
	void mute(int id){
		if(id == idAdmin){
			string errorAdmin = "Error, command not allowed !!";
			send(id, errorAdmin.c_str(), errorAdmin.size(), 0);					
		}
		else{
			UsersMute[id] = true;
			string messageMuted = Clients[id]+" muted";
			string messageErrorMuted = "You are muted !!";
			send(idAdmin, messageMuted.c_str(), messageMuted.size(), 0);
			send(id, messageErrorMuted.c_str(), messageErrorMuted.size(), 0);
		}
	}
	
	void unmute(int id){
		if(id == idAdmin){
			string errorAdmin = "Error, command not allowed !!";
			send(id, errorAdmin.c_str(), errorAdmin.size(), 0);		
		}
		else{
			UsersMute[id] = false;
			string messageMute = Clients[id]+" unmuted";
			string messageNoErrorMuted = "You are not muted !!";
			send(idAdmin, messageMute.c_str(), messageMute.size(), 0);
			send(id, messageNoErrorMuted.c_str(), messageNoErrorMuted.size(), 0);
		}
	}
			
};


//Conjunto de canais existentes.
set<int>SetChannels;

//O canal em que o id está conectado.
map<int, int>ConnectedChannel;

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
			break;
		}
	}

	string nick = Clients[id];
	
	//Informando que o usuario desconectou.
	cout << nick << " disconnected from server" << endl; 
	
	//Apagando nick do set Nicknames.
	Nicknames.erase(nick);
	
	//Apagando do map a dupla id/nick.
	Clients.erase(id);
	
	//Apagando do map a dupla nick/id.
	ClientsReverse.erase(nick);
	
	//Apagando do map de ips a dupla id/ip.
	IdChannel[ConnectedChannel[id]].UsersIp.erase(id);
	
	//Apagando o ID do vector do canal.
	for(int i=0; i<IdChannel[ConnectedChannel[id]].UsersChannel.size(); i++){
		if(id == IdChannel[ConnectedChannel[id]].UsersChannel[i]){
			IdChannel[ConnectedChannel[id]].UsersChannel.erase(IdChannel[ConnectedChannel[id]].UsersChannel.begin()+i);
			break;
		}
	}

}

//Enviando mensagens para todos os clientes
void SendMessages(string buffer, int channel){
	
	vector<int>AuxIdClients;
	
	//Pegando o vetor do canal especificado, se channel for igual a -1 quer dizer que o servidor esta fechando.
	if(channel == -1) AuxIdClients = IdClients;
	else AuxIdClients = IdChannel[channel].UsersChannel;
	
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
		else if(strcmp(buffer, "/ping") == 0){
			string pong = "pong";
			printf("Sending pong to %s\n", Clients[id].c_str());
			send(id, pong.c_str(), pong.size(), 0); 	
			continue;
		}
		else if(strcmp(buffer, "/quit") == 0){
			ClientQuit(id);
			break;
		}
		
		//Verificando os comandos do admin.
		else if(buffer[0] == '/'){
	
			string message = "", user = ""; 
			string messageError = "You no are admin !";
			string messageErrorUserExist = "User not exist";
			string messageErrorUserChannel = "User not belongs in this channel";
			
			int i = 0;
			
			for(i=0; i<strlen(buffer); i++){
				if(buffer[i] == ' ') break;
				else message += buffer[i];
			}
			
			for(i=i+1; i<strlen(buffer); i++) user+=buffer[i];
				
			if(message == "/whois"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(id, messageError.c_str(), messageError.size(), 0);
				}	
				else{
					if(Nicknames.count(user) != 0){
						int idUser = ClientsReverse[user];
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							string sendIP = IdChannel[ConnectedChannel[id]].whois(idUser);
							send(id, sendIP.c_str(), sendIP.size(), 0);
						}
						else send(id, messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);
					}
					else send(id, messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
					
				}	
				continue;
			}
			else if(message == "/kick"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(id, messageError.c_str(), messageError.size(), 0);
				}
				else{
					if(Nicknames.count(user) != 0){	
						int idUser = ClientsReverse[user];
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							IdChannel[ConnectedChannel[id]].kick(idUser);
						}
						else send(id, messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);
					}
					else send(id, messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
				}
				continue;	
			}
			else if(message == "/mute"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(id, messageError.c_str(), messageError.size(), 0);
				}
				else{
					if(Nicknames.count(user) != 0){
						int idUser = ClientsReverse[user];
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							IdChannel[ConnectedChannel[id]].mute(idUser); 
						}
						else send(id, messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);	
					
					}
					else send(id, messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
					
				}
				continue;				
			}
			else if(message == "/unmute"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(id, messageError.c_str(), messageError.size(), 0);		
				}
				else{
					if(Nicknames.count(user) != 0){
						int idUser = ClientsReverse[user];	
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							IdChannel[ConnectedChannel[id]].unmute(idUser);
						}
						else send(id, messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);								
					}
					else send(id, messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
				} 
				
				continue;				
			}	
			
			
		}
		
		if(IdChannel[ConnectedChannel[id]].UsersMute[id]){
			string messageMuted = "You are muted !!";
			send(id, messageMuted.c_str(), messageMuted.size(), 0);
			continue;
		}
		
		 //Escreve a mensagem recebida
		printf("%s-#%d: %s\n", Clients[id].c_str(), IdChannel[ConnectedChannel[id]].idChannel, buffer);
		
		//Inserindo o nick do cliente que mandou a mensagem
		string NewBuffer = Clients[id]+"-#"+to_string(IdChannel[ConnectedChannel[id]].idChannel)+": ";
		
		for(int i=0; i<strlen(buffer); i++) NewBuffer += buffer[i];
		
		//Reenvia a mensagem  para os clientes
		SendMessages(NewBuffer, IdChannel[ConnectedChannel[id]].idChannel);
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
		
		string message, rest;
		int channel;
		bool flagAdmin = false;
		
		while(true){
			
			//Limpando as strings.	
			message.clear();
			rest.clear();
				
			//Zera o buffer.
			memset(buffer, 0, sizeof buffer);
			
			//Lendo a mensagem do cliente.
			int ret = read(NewClient, buffer, sizeof buffer);	
			
			//Transformando para std::string.
			message.append(buffer, buffer+strlen(buffer));
		
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
			
			else if(message.size() > 7 and message.substr(0,5) == "/join" and message[6] == '#' and message[5] == ' '){
				char ip[INET_ADDRSTRLEN]; 
				Channel c;
				rest = message.substr(7);
				//Transformando o numero do canal em int.
				
				bool flagError = false;
				
				//Verificando se existe algum caractere diferente de número na string rest. 
				for(int i=0; i<(int)rest.size(); i++){
					if((int)rest[i] < 48 or (int)rest[i] > 57){
						flagError = true;
						break;
					}
				}
			
				if(flagError){
					send(NewClient, message_errorchannel, strlen(message_errorchannel), 0);
					continue;
				}
				
				channel = stoi(rest);
					
				//Pegando o ip do cliente.
				inet_ntop(AF_INET, &(SocketAddress.sin_addr), ip, INET_ADDRSTRLEN); 
				
				if(checkChannel(channel)){
				    c = IdChannel[channel];
					c.UsersChannel.push_back(NewClient);
					c.number++;
					c.UsersIp[NewClient] = ip;
					ConnectedChannel[NewClient] = channel;
					IdChannel[channel] = c;
				}
				else{	
					c.idChannel = channel;
					c.idAdmin = NewClient;
					c.number = 1;
					c.UsersChannel.push_back(NewClient);
					c.UsersIp[NewClient] = ip;		
					ConnectedChannel[NewClient] = channel;
					SetChannels.insert(channel);
					IdChannel[channel] = c;
					flagAdmin = true;
				}
				
				
				string sendMessageWelcome;
				
				sendMessageWelcome.append(message_welcomechannel, message_welcomechannel+strlen(message_welcomechannel));
				
				sendMessageWelcome+=rest + '\n';
				
				if(flagAdmin) sendMessageWelcome += "You are admin this channel !!\n";
				
				//Mandando a mesnagem de boas vindas ao canal.
				send(NewClient, sendMessageWelcome.c_str(), sendMessageWelcome.size(), 0);
				
				break;
			}
			else 
				send(NewClient, message_errorchannel, strlen(message_errorchannel), 0);
			
		}
		
	
		//Relacionando o id do cliente com o seu nickname.
		Clients[NewClient] = nick;
		
		//Relacionando o nick do cliente com o seu id.
		ClientsReverse[nick] = NewClient;
		
		//Guardando o id do cliente no vetor de clientes.
		IdClients.push_back(NewClient);
		
		//Guardando no conjunto de nicks.
		Nicknames.insert(nick);
		
		//Informando que o usuario conectou-se ao server.
		cout << nick << " joined the channel #" << channel << endl;

		//Inserindo o cliente na fila.
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
			SendMessages("/disconnect", -1);
			
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

