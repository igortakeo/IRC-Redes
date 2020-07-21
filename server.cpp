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
#include <mutex>
using namespace std;

struct sockaddr_in SocketAddress;
int addrlen;
mutex m;

//Declaracoes das funcoes.
void handler(int sig);
void ClientQuit(int id);
bool checkChannel(int id);
void SendMessages(string buffer, int channel);
void ThreadMessageClients(int id);
void MessageClients();
void AcceptClient(int NewServer);
void MessagesStdin();
bool ChooseChannel(int NewClient, string nick , string ip, int flag);

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

//O map guarda o idClient e seu idWindowChannel.
map<int, int>ChannelSocket;

//Guarda o ip do cliente temporiaramente .
map<int, string>IPAux;

//O cliente tem um convite pendente.
map<int, bool>PendingInvite;

//Guarda a dupla id do cliente e o conjunto de id's que ele recebeu convite.
map<int, queue<int>>WhoInvited;

class Channel{
	
	public:
	
	//Id do canal.
	int idChannel;
	
	//Variavel booleana para verificar se o canal eh privado ou nao.
	bool privateChannel;
	
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
			send(ChannelSocket[id], errorAdmin.c_str(), errorAdmin.size(), 0);		
		}
		else{ 
			string messageDisconnect = "\nYou was kicked !!\n-------------------------------------------------------------------\n";
			send(id, messageDisconnect.c_str(), messageDisconnect.size(), 0);
			send(ChannelSocket[id], messageDisconnect.c_str(), messageDisconnect.size(), 0);
		}	
	}
	
	void mute(int id){
		if(id == idAdmin){
			string errorAdmin = "Error, command not allowed !!";
			send(ChannelSocket[id], errorAdmin.c_str(), errorAdmin.size(), 0);					
		}
		else{
			UsersMute[id] = true;
			string messageMuted = Clients[id]+" muted";
			string messageErrorMuted = "You are muted !!";
			send(ChannelSocket[idAdmin], messageMuted.c_str(), messageMuted.size(), 0);
			send(ChannelSocket[id], messageErrorMuted.c_str(), messageErrorMuted.size(), 0);
		}
	}
	
	void unmute(int id){
		if(id == idAdmin){
			string errorAdmin = "Error, command not allowed !!";
			send(ChannelSocket[id], errorAdmin.c_str(), errorAdmin.size(), 0);		
		}
		else{
			UsersMute[id] = false;
			string messageMute = Clients[id]+" unmuted";
			string messageNoErrorMuted = "You are not muted !!";
			send(ChannelSocket[idAdmin], messageMute.c_str(), messageMute.size(), 0);
			send(ChannelSocket[id], messageNoErrorMuted.c_str(), messageNoErrorMuted.size(), 0);
		}
	}
	
	void invite(int id){
		if(id  == idAdmin or !privateChannel){
			string errorAdmin = "Error, command not allowed !!";
			send(ChannelSocket[id], errorAdmin.c_str(), errorAdmin.size(), 0);		
		}
		else{	
			string inviteUser = Clients[idAdmin] + " invited you participate of the channel #"+to_string(idChannel)+"(yes/no) ?";
			send(ChannelSocket[id], inviteUser.c_str(), inviteUser.size(), 0);
			PendingInvite[id] = true;
			WhoInvited[id].push(idChannel);
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

void ErasefromChannel(int id, Channel channel){
	
	//Apagando do map de ips a dupla id/ip.
	channel.UsersIp.erase(id);
	
	//Apagando o id do vector do canal.
	for(int i=0; i<channel.UsersChannel.size(); i++){
		if(id == channel.UsersChannel[i]){
			channel.UsersChannel.erase(channel.UsersChannel.begin()+i);
			break;
		}
	} 
	
	//Diminuindo o numero de pessoas no canal.
	channel.number--;
	
	//Caso o canal tenha zero clientes, ele e excluido.
	if(channel.number == 0){
		SetChannels.erase(channel.idChannel);
		IdChannel.erase(channel.idChannel);	
	}
	
	//Cliente sem canal.
	ConnectedChannel[id] = -1;
	
	//Atualizando o canal no map.
	IdChannel[channel.idChannel] = channel;
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
	
	//Apagando do map a dupla idClient/idWindowChannel.
	ChannelSocket.erase(id);
	
	//Apagando do map de ips a dupla id/ip.
	IdChannel[ConnectedChannel[id]].UsersIp.erase(id);
	
	//Apagando o ID do vector do canal.
	for(int i=0; i<IdChannel[ConnectedChannel[id]].UsersChannel.size(); i++){
		if(id == IdChannel[ConnectedChannel[id]].UsersChannel[i]){
			IdChannel[ConnectedChannel[id]].UsersChannel.erase(IdChannel[ConnectedChannel[id]].UsersChannel.begin()+i);
			break;
		}
	}
	
	//Diminuindo o numero de pessoas no canal.
	IdChannel[ConnectedChannel[id]].number--;
	
	//Caso o canal tenha zero clientes, ele e excluido.
	if(IdChannel[ConnectedChannel[id]].number == 0){
		SetChannels.erase(ConnectedChannel[id]);
		IdChannel.erase(ConnectedChannel[id]);	
	}
	
	//Apagando do map id/id_channel.
	ConnectedChannel.erase(id);
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
			if(send(ChannelSocket[i], buffer.c_str(), buffer.size(), 0) == -1) tries--;
			else break;
		}
		
		if(buffer == "/disconnect"){
			send(i, buffer.c_str(), buffer.size(), 0);
			ClientQuit(i);		
		}
		if(tries == 0){
			ClientQuit(i);
			close(i);
		}
	}
}

void ThreadMessageClients(int id){
	string messagePrivateChannel = "This channel is only for guests\n";
	string messagePendingInvite = "Answer your pending invite";
	char buffer[2050];	
	
	while(true){
		//Zera o buffer
		memset(buffer, 0, sizeof buffer);

		//Recebe a mensagem enviada pelo cliente
		int ret  = read(id, buffer, sizeof buffer); 
		
		if(ConnectedChannel[id] == -1 and strcmp(buffer, "/choosechannel") == 0){
				thread ChooseCh(ChooseChannel, id, Clients[id], IPAux[id], 0);
				ChooseCh.join();
				IPAux.erase(id);
				continue;
		}
		
		if(PendingInvite[id]){
			
			if(strcmp(buffer, "yes") == 0){
				Channel c = IdChannel[WhoInvited[id].front()];
				c.UsersChannel.push_back(id);
				c.number++;
				c.UsersIp[id] = IdChannel[ConnectedChannel[id]].UsersIp[id];
				ErasefromChannel(id, IdChannel[ConnectedChannel[id]]);
				ConnectedChannel[id] = WhoInvited[id].front();
				IdChannel[WhoInvited[id].front()] = c;
			
				WhoInvited[id].pop();
				if(WhoInvited[id].empty())PendingInvite.erase(id);
			
				string sendMessageWelcome = "-------------------------------------------------------\n";
				sendMessageWelcome +=  "Welcome to Channel #";
				sendMessageWelcome += to_string(c.idChannel);
				
				cout << Clients[id] << " joined the channel #" << c.idChannel << endl;
				
				//Mandando a mensagem de boas vindas ao canal.
				send(ChannelSocket[id], sendMessageWelcome.c_str(), sendMessageWelcome.size(), 0);		
			}
			else if(strcmp(buffer, "no") == 0){
				string messageThankYou = "Thank you";
				WhoInvited[id].pop();
				if(WhoInvited[id].empty()) PendingInvite.erase(id);
				
				send(ChannelSocket[id], messageThankYou.c_str(), messageThankYou.size(), 0);
			}
			else{
				send(ChannelSocket[id], messagePendingInvite.c_str(), messagePendingInvite.size(), 0);
			}
			continue;
		}
		
		if(ret <= 0) break;
		else if(strcmp(buffer, "/ping") == 0){
			string pong = "pong";
			printf("Sending pong to %s\n", Clients[id].c_str());
			send(ChannelSocket[id], pong.c_str(), pong.size(), 0); 	
			continue;
		}
		else if(strcmp(buffer, "/quit") == 0){
			send(ChannelSocket[id], buffer, strlen(buffer), 0);
			ClientQuit(id);
			break;
		}
		
		//Verificando os comandos do admin.
		else if(buffer[0] == '/'){
	
			string message = "", user = "", joinchannel = "", numberchannel = ""; 
			string messageError = "You no are admin !";
			string messageErrorUserExist = "User not exist";
			string messageErrorUserChannel = "User not belongs in this channel";
			string messageErrorUserChannelBelong = "User belongs this channel";
			string messageErrorNickname = "Nickname is same";
			string messageErrorNicknameExist = "Nickname already exist";
			string messageNicknameChanged = "Nickame was changed";
			string messageErrorJoinChannel = "Error !!";
			string messagePrivateChannel = "This is channel is invite only !!";
			string messageNotPrivateChannel = "This is channel not is invite only !!";
			
			int i = 0;
			
			for(i=0; i<strlen(buffer); i++){
				if(buffer[i] == ' ') break;
				else message += buffer[i];
			}
			
			for(i=i+1; i<strlen(buffer); i++) user+=buffer[i];
			
			joinchannel.append(buffer, buffer+strlen(buffer));
			
			if(joinchannel.size() > 7 and joinchannel.substr(0,5) == "/join" and joinchannel[6] == '#' and joinchannel[5] == ' '){
				Channel c;
				numberchannel = joinchannel.substr(7);
				
				bool flagError = false, flagAdmin = false;
				
				//Verificando se existe algum caractere diferente de numero na string numberchannel. 
				for(int i=0; i<(int)numberchannel.size(); i++){
					if((int)numberchannel[i] < 48 or (int)numberchannel[i] > 57){
						flagError = true;
						break;
					}
				}
			
				if(flagError){
					send(ChannelSocket[id], messageErrorJoinChannel.c_str(), messageErrorJoinChannel.size(), 0);
					continue;
				}
				
				int channel_change = stoi(numberchannel);
				
				if(ConnectedChannel[id] == channel_change){
					send(ChannelSocket[id], messageErrorJoinChannel.c_str(), messageErrorJoinChannel.size(), 0);
					continue;
				}
				
				if(checkChannel(channel_change) and !IdChannel[channel_change].privateChannel){
					c = IdChannel[channel_change];
					c.UsersChannel.push_back(id);
					c.number++;
					c.UsersIp[id] = IdChannel[ConnectedChannel[id]].UsersIp[id];
					ErasefromChannel(id, IdChannel[ConnectedChannel[id]]);
					ConnectedChannel[id] = channel_change;
					IdChannel[channel_change] = c;
				}
				else if(SetChannels.count(channel_change) == 0){
					c.idChannel = channel_change;	
					c.idAdmin = id;
					c.number = 1;
					c.UsersChannel.push_back(id);
					c.UsersIp[id] = IdChannel[ConnectedChannel[id]].UsersIp[id];
					ErasefromChannel(id, IdChannel[ConnectedChannel[id]]);
					ConnectedChannel[id] = channel_change;
					SetChannels.insert(channel_change);
					IdChannel[channel_change] = c;
					flagAdmin = true;
				}
				else{
					send(ChannelSocket[id], messagePrivateChannel.c_str(), messagePrivateChannel.size(), 0);
					continue;
				}
				string sendMessageWelcome = "-------------------------------------------------------\n";
				sendMessageWelcome +=  "Welcome to Channel #";
				sendMessageWelcome+= numberchannel;
				if(flagAdmin) sendMessageWelcome += "\nYou are admin this channel !!\n";
				
				//Mandando a mensagem de boas vindas ao canal.
				send(ChannelSocket[id], sendMessageWelcome.c_str(), sendMessageWelcome.size(), 0);
				
				//Informando que o usuario conectou-se ao server.
				cout << Clients[id]  << " joined the channel #" << channel_change << endl;

				continue;
			}	
				
			if(message == "/whois"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(ChannelSocket[id], messageError.c_str(), messageError.size(), 0);
				}	
				else{
					if(Nicknames.count(user) != 0){
						int idUser = ClientsReverse[user];
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							string sendIP = IdChannel[ConnectedChannel[id]].whois(idUser);
							send(ChannelSocket[id], sendIP.c_str(), sendIP.size(), 0);
						}
						else send(ChannelSocket[id], messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);
					}
					else send(ChannelSocket[id], messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
					
				}	
				continue;
			}
			else if(message == "/kick"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(ChannelSocket[id], messageError.c_str(), messageError.size(), 0);
				}
				else{
					if(Nicknames.count(user) != 0){	
						int idUser = ClientsReverse[user];
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							IPAux[idUser] = IdChannel[ConnectedChannel[id]].UsersIp[idUser];
							IdChannel[ConnectedChannel[id]].kick(idUser);
							ErasefromChannel(idUser, IdChannel[ConnectedChannel[idUser]]);
						}
						else send(ChannelSocket[id], messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);
					}
					else send(ChannelSocket[id], messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
				}
				continue;	
			}
			else if(message == "/mute"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(ChannelSocket[id], messageError.c_str(), messageError.size(), 0);
				}
				else{
					if(Nicknames.count(user) != 0){
						int idUser = ClientsReverse[user];
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							IdChannel[ConnectedChannel[id]].mute(idUser); 
						}
						else send(ChannelSocket[id], messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);	
					
					}
					else send(ChannelSocket[id], messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
					
				}
				continue;				
			}
			else if(message == "/unmute"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(ChannelSocket[id], messageError.c_str(), messageError.size(), 0);		
				}
				else{
					if(Nicknames.count(user) != 0){
						int idUser = ClientsReverse[user];	
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 1){
							IdChannel[ConnectedChannel[id]].unmute(idUser);
						}
						else send(ChannelSocket[id], messageErrorUserChannel.c_str(), messageErrorUserChannel.size(), 0);								
					}
					else send(ChannelSocket[id], messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
				} 
				
				continue;				
			}
			else if(message == "/nickname"){
				
				if(Clients[id] == user) send(ChannelSocket[id], messageErrorNickname.c_str(), messageErrorNickname.size(), 0);
				else if(Nicknames.count(user) == 1) send(ChannelSocket[id], messageErrorNicknameExist.c_str(), messageErrorNicknameExist.size(), 0);
				else{
					Nicknames.erase(Clients[id]);
					ClientsReverse.erase(Clients[id]);
					Clients[id] = user;
					ClientsReverse[user] = id;
					Nicknames.insert(user);
					send(ChannelSocket[id], messageNicknameChanged.c_str(), messageNicknameChanged.size(), 0);
				}
				
				continue;
			}
			else if(message == "/invite"){
				if(!IdChannel[ConnectedChannel[id]].privateChannel){
					send(ChannelSocket[id], messageErrorJoinChannel.c_str(), messageErrorJoinChannel.size(), 0);
				}
				else if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(ChannelSocket[id], messageError.c_str(), messageError.size(), 0);
				}
				else{
					if(Nicknames.count(user) != 0){	
						int idUser = ClientsReverse[user];
						if(count(IdChannel[ConnectedChannel[id]].UsersChannel.begin(),IdChannel[ConnectedChannel[id]].UsersChannel.end(),idUser) == 0){
							IdChannel[ConnectedChannel[id]].invite(idUser);
						}
						else send(ChannelSocket[id], messageErrorUserChannelBelong.c_str(), messageErrorUserChannelBelong.size(), 0);
					}
					else send(ChannelSocket[id], messageErrorUserExist.c_str(), messageErrorUserExist.size(), 0); 
					
				}		
				continue;
			}
			else if(message == "/mode"){
				if(id != IdChannel[ConnectedChannel[id]].idAdmin){
					send(ChannelSocket[id], messageError.c_str(), messageError.size(), 0);		
				}
				else{
						if(user == "+i"){
							IdChannel[ConnectedChannel[id]].privateChannel = true;
							send(ChannelSocket[id], messagePrivateChannel.c_str(), messagePrivateChannel.size(), 0);
						}
						else if(user == "-i"){
							IdChannel[ConnectedChannel[id]].privateChannel = false;
							send(ChannelSocket[id], messageNotPrivateChannel.c_str(), messageNotPrivateChannel.size(), 0);
						}
						else{
							send(ChannelSocket[id], messageErrorJoinChannel.c_str(), messageErrorJoinChannel.size(), 0);
						}	
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

bool ChooseChannel(int NewClient, string nick, string  ip, int flag){
		m.lock();
		char message_errorchannel[50] = "Error, type again\n";
		char message_welcomechannel[50] = "Welcome to Channel #";
		char message_welcomeclient[50] = "Welcome\nType the messages below:\n"; 
		char message_emtpychannel[50] = "List empty !\n"; 
		char message_joinchannel[200] = "Choose one channel to join\n(/list: channels list that already exist)\n(/join '#numberchannel': to join one channel)\n";
		char message_privatechannel[200] = "This channel is only for guests\n";
		char buffer[2050];
		
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
			
			//Verificando se o cliente deu sinal de eof.
			if(message == "/quit"){
				//Informando que o usuario desconectou.
				if(ChannelSocket[NewClient] != -1){
					send(ChannelSocket[NewClient], message.c_str(), message.size(), 0);
				}
				cout << nick << " disconnected from server" << endl; 
				return  false;
			}
			
			//Verificando se o cliente pediu para ver a lista de canais.
			else if(message == "/list"){
				
				//Verificando se a lista esta vazia.
				if(SetChannels.empty()) 
					send(NewClient, message_emtpychannel, strlen(message_emtpychannel), 0);	
				else{
					string sendList = "";
					
					//Pegando a lista de canais.
					for(auto c : SetChannels){
						sendList += "Channel";
						if(IdChannel[c].privateChannel) sendList += " (Invite-only)";
						sendList += ": #"+to_string(IdChannel[c].idChannel)+" -- Users Online: "+to_string(IdChannel[c].number)+'\n';
					}
					//Enviando a lista de canais para o cliente.	
					send(NewClient, sendList.c_str(), sendList.size(), 0);
				}	
			}
			
			else if(message.size() > 7 and message.substr(0,5) == "/join" and message[6] == '#' and message[5] == ' '){
				Channel c;
				rest = message.substr(7);
				string stringflagPrivate = "";
				int j = rest.size(), sz = 0;
				bool flagError = false;
		
				//Verificando se existe algum caractere diferente de número na string rest. 
				for(int i=0; i<(int)rest.size(); i++){
					if((int)rest[i] < 48 or (int)rest[i] > 57){
						flagError = true;
						break;
					}
					else sz++;
				}
				
				
				if(flagError){
					send(NewClient, message_errorchannel, strlen(message_errorchannel), 0);
					continue;
				}
				
				//Transformando o numero do canal em int.
				channel = stoi(rest);
				
				if(checkChannel(channel) and IdChannel[channel].privateChannel){
						send(NewClient, message_privatechannel, strlen(message_privatechannel), 0);
						continue;
				}
						
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
				
				if(flag){		
					//Abrindo um terminal para mostrar as mensagens do canal.
					system("gnome-terminal -e 'sh -c \"./windowchannel < saveIPaddress.txt\"' > /dev/null 2>&1");
					
					//Aceita o canal.
					int NewWindowChannel = accept(NewServer, (struct sockaddr*)&SocketAddress, (socklen_t*)&addrlen);
					
					//Relacionando o idClient com o seu idWindowChannel.
					ChannelSocket[NewClient] = NewWindowChannel;
				}
				
				string sendMessageWelcome;
				
				sendMessageWelcome.append(message_welcomechannel, message_welcomechannel+strlen(message_welcomechannel));
				
				sendMessageWelcome+=rest.substr(0, sz) + '\n';
				
				if(flagAdmin) sendMessageWelcome += "You are admin this channel !!\n";
									
				//Informando que o usuario conectou-se ao server.
				cout << nick << " joined the channel #" << channel << endl;		
			
				//Mandando a mensagem de boas vindas ao canal.
				send(ChannelSocket[NewClient], sendMessageWelcome.c_str(), sendMessageWelcome.size(), 0);
				send(NewClient, message_welcomeclient, strlen(message_welcomeclient), 0);

				break;
			}
			else 
				send(NewClient, message_errorchannel, strlen(message_errorchannel), 0);	
		}
		m.unlock();
		
		return true;
		
}

void AcceptClient(int NewServer){
	
	char message_welcome[150] = "Welcome to server\nInsert your Nickname(less or equal 50 characters ASCII): ";
	char message_accept[50] = "Nickname accepted";
	char message_error_nickame[150] = "Nickname already exist\nInsert your Nickname(less or equal 50 characters ASCII): ";
	char message_nicklarge[150] = "Nickname too large, type again\nInsert your Nickname(less or equal 50 characters ASCII): ";
	char buffer[2050];
	string ip;
	
	while(true){
		
		
		struct sockaddr_in SocketAddressClient;
		int addrlenClient = sizeof SocketAddress;
			
		//Zera o buffer.
		memset(buffer, 0, sizeof buffer);
		
		//Aceita o cliente.
		int NewClient = accept(NewServer, (struct sockaddr*)&SocketAddress, (socklen_t*)&addrlen);
		
		//Se falhar, retorna.
		if(NewClient == -1){
			printf("Accept Failed\n");
			continue;
		}
		
		//Envia a mensagem de boas vindas ao cliente.
		send(NewClient, message_welcome, strlen(message_welcome), 0);
			
		string nick;
		bool flagEOF = false;
		while(true){
			
			//Lendo nickname do cliente.
			int ret = read(NewClient, buffer , sizeof buffer);
	
			//Transformando pra std::string.
			nick.append(buffer, buffer+strlen(buffer));
			
			//Verificando se o cliente deu sinal de eof.	
			if(nick == "/quit"){
				flagEOF = true;
				break;
			}
			
			//Verificando se o nickname tem mais de 50 caracteres.
			else if(nick.size() > 50){
				
				//Dizendo que o nick esta muito grande.
				send(NewClient, message_nicklarge, strlen(message_nicklarge), 0);
			}
			
			//Verificando se o nickname ja existe.
			else if(Nicknames.count(nick) == 0){
				
				//Mandando a mensagem que o nickname foi aceito.
				send(NewClient, message_accept, strlen(message_accept), 0);
		
				break;
			}
			else{
				
				//Enviando a mensagem de erro para o cliente.
				send(NewClient, message_error_nickame, strlen(message_error_nickame), 0);
			}
			
			//Zera o buffer.
			memset(buffer, 0, sizeof buffer);
			
			//Limpando o nick.
			nick.clear();
		}
		
		//Verificando o sinal de eof do cliente.	
		if(flagEOF) continue;
		
		//Zera o buffer.
		memset(buffer, 0, sizeof buffer);
		
		//Lendo a mensagem do cliente.
		read(NewClient, buffer, sizeof buffer);
		
		//Printando a mensagem.
		printf("%s", buffer);
		
		//Pegando ip do cliente
		getpeername(NewClient, (struct sockaddr*)&SocketAddressClient, (socklen_t*)&addrlenClient);
		ip = inet_ntoa(SocketAddressClient.sin_addr);
		
		flagEOF = ChooseChannel(NewClient, nick, ip, 1);
		
		//Verificando o sinal de EOF do cliente.
		if(!flagEOF) continue;
		
		//Relacionando o id do cliente com o seu nickname.
		Clients[NewClient] = nick;
		
		//Relacionando o nick do cliente com o seu id.
		ClientsReverse[nick] = NewClient;
		
		//Guardando o id do cliente no vetor de clientes.
		IdClients.push_back(NewClient);
		
		//Guardando no conjunto de nicks.
		Nicknames.insert(nick);
		
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


bool verifyPort(string port){
	for(int i=0; i<port.size(); i++){
		if((int)port[i] < 48 or (int)port[i] > 57)
			return true;
	}
	return false;
}

int main(){
				
	
	signal(SIGINT, handler);
	
	addrlen = sizeof SocketAddress;
	string port;
	
	//Cria um socket
	NewServer = socket(AF_INET, SOCK_STREAM, 0);

	if(NewServer == 0){
		printf("Creating Failed\n");
		return 0;
	}
	
	//Perguntando a porta para estabelecer a conexao.
	while(true){
		printf("Type the port to connect: ");
		getline(cin, port);
		if(verifyPort(port)){
			printf("Invalid Port\n");
		}
		else break;
	}
	
	//Vincula o socket a porta especificada 
	SocketAddress.sin_family = AF_INET;
	SocketAddress.sin_addr.s_addr = INADDR_ANY;
	SocketAddress.sin_port = htons(stoi(port));
	
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
	
	thread ThreadAccept(AcceptClient, NewServer);
	thread ThreadMessages(MessageClients);
	thread ThreadMessagesStdin(MessagesStdin);
	ThreadAccept.join();
	ThreadMessages.join();
	ThreadMessagesStdin.join();
	
	return 0;
}

