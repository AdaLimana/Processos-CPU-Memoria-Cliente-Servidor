#include<stdio.h>
#include<stdlib.h>
#include<string.h>

/*bibliotecas para comunicacao via SOCKET*/
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>

/*biblioteca para uso de THREAD*/
#include <pthread.h>

#define QNT_CHAMADAS 2

typedef struct{
	int porta;
	char ip[30];
}endereco;

void *nucleo(void *x);

void zeraString(char *s);

int main(int argc, char *argv[]){
	char mensagem[100];
	endereco end;
	pthread_t thread[QNT_CHAMADAS];/*identificador da thread*/
	int chamada = 0;
	if(argc < 2 ){
		printf("\nargumentos invalidos\n");
		return 0;
	}

	end.porta = atoi(argv[1]);
	strcpy(end.ip, argv[2]);
	while(chamada < QNT_CHAMADAS){

		if(pthread_create(&thread[chamada], NULL, nucleo, (void *) &end)){
			printf("\nErro ao criar a thread\n");
			exit(1);
		}
		chamada ++;
	}
	chamada = 0;

	while(chamada < QNT_CHAMADAS){
		pthread_join(thread[chamada], NULL);
		chamada++;
	}
        return 0;
}

void *nucleo(void *x){

	char mensagem[100];
	endereco *e = (endereco *)x;

	 int sockfd; /*Identifica o socket cliente criado*/
        struct sockaddr_in cliente;

        sockfd = socket(AF_INET, SOCK_STREAM, 0); /*Criando o socket*/

        /*Setando o socket*/
        cliente.sin_family = AF_INET;
        cliente.sin_port = htons(e->porta);
        cliente.sin_addr.s_addr = inet_addr(e->ip);//htonl(INADDR_ANY);

        /*Inicia a conexão com o servidor*/
         if(connect(sockfd, &cliente, sizeof(cliente)) < 0){
                perror("Erro na conexão");
                exit(1);
        }
        memset(mensagem, '\0', 100);/*preenche com '\0' a string*/
        recv(sockfd, mensagem, 100, 0);
        puts(mensagem);

        /*while(1){
                printf("\n%%%%%%%%%%%%%%%%%%%\n");
                memset(mensagem, '\0', 100);
                recv(sockfd, mensagem, 100, 0);
                if(mensagem == "Conexao Encerrada"){
                        break;
                }
                send(sockfd, mensagem, 100,0);
        }*/

        memset(mensagem, '\0',100);
        recv(sockfd, mensagem,100,0);
        puts(mensagem);
	pthread_exit(NULL);

}
