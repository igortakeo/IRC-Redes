USP São Carlos - ICMC 2020
Disciplina: Redes de Computadores(SSC-0142) Professora Kalinka R. L. J. Castelo Branco
Trabalho Redes Módulo 2 - Comunicação entre múltiplos clientes e servidor.


Membros Grupo ---> Nome/NUSP
Guilherme Targon Marques Barcellos/10724181
Igor Ambo Takeo de Melo/10830054
Mateus Ferreira Gomes/10734773

Versão Linux:Ubuntu 20.04 LTS
Compilador: gcc version 9.3.0(Ubuntu 9.3.0-10ubuntu2)

Passo a passo para execução:
1->Download/extração do arquivo compactado para uma pasta.
2->Abra terminal na pasta onde se encontram os arquivos extraidos.
3->comando "make all" para compilar os códigos "server.cpp" e "client.cpp".
4->comando "make runs" para iniciar o servidor.
5->comando "make runc" para iniciar o cliente.
OBS->Caso não seja possível utilizar makefile, segue comando compilação terminal:
Servidor-> "g++ -std=c++11 -pthread server.cpp -o server" e "./server".
Cliente->  "g++ -std=c++11 -pthread client.cpp  -o client" e "./client".


Comandos disponíveis:
/connect - Estabelece a conexão com o servidor.
/quit - O cliente fecha a conexão e fecha a aplicação.
/ping - O servidor retorna "pong" assim que receber a mensagem.


Informações importantes:
1->É preciso iniciar o servidor antes de iniciar o programa cliente.
2->É preciso escolher um "nickname" para utilizar o programa cliente. Caso o nick já esteja.
sendo usado, o programa não vai aceitar e pedirá para digitar um nickname válido.
3->Para sair do programa cliente basta digitar "/quit" ou apertar CTRL + D.
4->Após sair do programa cliente seu nickname ficará disponivel para outras pessoas.
