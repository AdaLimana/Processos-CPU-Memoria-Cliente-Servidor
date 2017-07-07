#include "lista.h"
#include "processo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*bibliotecas para comunicação via SOCKET*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*biblioteca para uso de thread*/
#include <pthread.h>

#define QNT_ATENDIMENTO 4
#define TAMANHO_VIRTUAL 32
#define TAMANHO_PRINCIPAL 16

/*estruturas para controle das memorias virtual e princiapl*/
memoria memoriaVirtual;
memoria memoriaPrincipal;
tabelaPaginaPrincipal tpp;
tabelaPaginaVirtual tpv;

pthread_mutex_t mutexApto = PTHREAD_MUTEX_INITIALIZER;/*controla o acesso a lista de APTOS*/
pthread_mutex_t mutexExec = PTHREAD_MUTEX_INITIALIZER;/*controla o acesso a lista de EXECUCAO*/
pthread_mutex_t serv = PTHREAD_MUTEX_INITIALIZER; /*controla o acesso ao ID_CLIENTE*/
/*Estrutura para mandar as duas listas para thread "núcleo" */

typedef struct{

        lista lApto;/*lista que armazena os processos aptos*/
        lista lExec;/**lista que armazena os processos em execução*/
        int id_cliente;/*identificador do cliente*/
	int porta;
	int quantum;/*Determina o tempo de ciclos de processamento do nucleo por processo*/
}listProcesso;



int shutd =0 ;




int setaServidor( struct sockaddr_in *serv, int *id_serv, int port);
void* servico(void* x);
void* nucleo(void* l);

void escolheComando(listProcesso *l, int *pid, pthread_t *nucleo);
int separaComando(char *comando, char *tipoComando, char *nome, int *tamanhoProcesso, int *tempoExec, int *pidKill, int *pidPs);
int separaCreate(char *comando, char *nome, int *tamanhoProcesso, int *tempoExec, int indice);
int separaKill(char *comando, int *pidKill, int indice);
int separaPs(char *comando, int *pidPs, int indice);
int separaMem(char *comando, int *argumento, int indice);
int processoLista(lista *l, int *pids, char *nome, int tamanhoProcesso, int tempoExec);/*coloca um processo na lista*/
int killProcesso(lista *l, int pidKill);
void psProcesso(lista *l);
int psPid(lista *l, int psPid);
void mostraTime(time_t *t);
void shutdownProcessos(lista *l);
int aleatorio(int numInicial, int numFinal);
void legenda();





int main(int argc, char *argv[]){

	
	pthread_t thread;/*identificaor da thread*/

	listProcesso l;/*argumento para a thread "núcleo"*/

	inicializaMemoria(&memoriaVirtual, TAMANHO_VIRTUAL);/*inicializa a memória virtual*/
	inicializaMemoria(&memoriaPrincipal, TAMANHO_PRINCIPAL);/*inicializa a memória principal*/
	
	inicializaTPV(&tpv, TAMANHO_VIRTUAL);/*inicializa a tabela da memória virtual*/
	inicializaTPP(&tpp, TAMANHO_PRINCIPAL);/*inicializa a tabela da memória principal*/

        inicializa(&l.lApto);
        inicializa(&l.lExec);
	l.quantum = aleatorio(5,20); /*Determina o tempo de ciclos de processamento do nucleo por processo, podendo ser um valor entre  5 e 20*/
	
        int pids = 0;/*controle dos PIDs*/


	if(argc == 2 ){
		l.porta  = atoi(argv[1]);
	}
	else{
		l.porta = 7777;
	}

	if(pthread_create(&thread, NULL, servico, (void *) &l)){

		printf("\nErro ao criar a thread\n");
		exit(1);
	}

	legenda();/*mostra exemplos de comando*/
	printf("\nO VALOR DO QUANTUM E %d\n",l.quantum);

	escolheComando(&l, &pids, &thread);

	pthread_join(thread, NULL);/*aguarda a thread terminar para finalizar*/
	
	liberaTPP(&tpp);
	liberaTPV(&tpv);
	liberaMemoria(&memoriaVirtual);
	liberaMemoria(&memoriaPrincipal);
	return 0;
}



void* servico(void* x){/*thread responsável por criar as outras threads para a comunicação*/

	int atendimento = 0;
	listProcesso *l = (listProcesso *) x;
	pthread_t threads[QNT_ATENDIMENTO];/*referência das threads para a comunicação*/

	/*variaveis para a comunicação (SERVIDOR)*/
        struct sockaddr_in servidor;/*estrutura para o servidor*/
        int id_servidor;/*identificador do servidor*/

        /*variaveis para a comunicação (CLIENTE)*/
        struct sockaddr_in cliente[QNT_ATENDIMENTO];/*estrutura para os clientes*/
        int id_cliente[QNT_ATENDIMENTO];/*identificador dos clientes*/
        int tamanho_cliente = sizeof(struct sockaddr_in);/*Armazena o tamanho dos clientes*/

	if(!(setaServidor(&servidor, &id_servidor, l->porta))){
		printf("\n Falha na criacao do socket\n");
		exit(1);
	}

	while(atendimento < QNT_ATENDIMENTO){

		listen(id_servidor, 5);/*Coloca o servidor na escuta*/
		pthread_mutex_lock(&serv);/*bloqueia ate o servidor receber o ID_CLIENETE na funcao (nucleo)*/
		id_cliente[atendimento] = accept(id_servidor, (struct sockaddr *) &cliente[atendimento], &tamanho_cliente);/*Aceita a conexão*/
		if(id_cliente[atendimento]>0){/*Caso tenha recebido uma conexão cria uma thread*/
			l->id_cliente = id_cliente[atendimento];
			if(pthread_create(&threads[atendimento], NULL, nucleo, (void *) l)){
				printf("\nErro ao criar a thread %d\n", atendimento);
				close(id_servidor);
			}
			atendimento++;
		}
	}


	atendimento = 0;/*Espera as thread terminar*/
	while(atendimento < QNT_ATENDIMENTO){

		pthread_join(threads[atendimento], NULL);
		atendimento++;
	}
	close(id_servidor);
	pthread_exit(NULL);

}



void* nucleo(void* l){

	listProcesso *list;
	char mensagem[100];
        list =(listProcesso *)l;
	int id = list->id_cliente;
	pthread_mutex_unlock(&serv);/*desbloquia, apos o armazenamento do ID_CLIENTE para criar novos sockets*/
	memset(mensagem, '\0',100);
	strcpy(mensagem, "Conexao Iniciada");
	send(id, mensagem, 100, 0);

        lista *listA;
        listA = &list->lApto;

        lista *listE;
        listE = &list->lExec;

        no *noExcluiExec; /*armazena o end do processo em execução que será excluido*/
        int pid;/*armazena o pid do processo em execucao que será excluido (caso termine seu tempo de execucao)*/
        int i=1; /*indice para achar a posição do processo que está na lista execução e devera ser excluido após executar */


        no *noAtual;
        processo *proc;
        time_t inicioExec;
        time_t tempoAtual;
	time_t tempoAnterior;

	int indicePagina; /*recebe a pagina que deve estar armazenada na memoria principal no segundo ciclo do processo*/ 
	while(1){
                pthread_mutex_lock(&mutexApto);//bloquei o acesso a lista aptos
                if(listA->tamanho){/*Se tem processo na lista*/
			noAtual = (no*)listA->primeiro;
                        proc = (processo *)noAtual->memoria;
			proc->estado = 1;/*coloca no estado executando*/
			proc->qntCiclo++;/*Adiciona um a quantidade de vezes que passou pelo processador*/
                        pid = proc->pid;/*recebe o pid do processo que ira para execuçao para após a execução o mesmo poder se excluido*/

			if(!inserefim(listE, (void *)proc)){/*Passa o processo da lista de Aptos para a lista de Execucao*/
                                printf("\nProblema para armazenar na lista de EXECUCAO\n");
                                pthread_exit(NULL);/*Finaliza A thread*/
                        }

                        tiraInicio(listA);/*Apos passa para a lista de Execução, tira o mesmo da lista de Aptos, mas não exclui o nó pois o mesmo ira para a lista execução*/
                        pthread_mutex_unlock(&mutexApto);//desboqueia
			indicePagina = aleatorio(0, (proc->qntPagina-1));/*um numero entre 0 e o numero referente a ultima pagina*/
			if(proc->qntCiclo == 2){/*Se for a segunda vez que for passar pelo processador verifica se precisa de um nova página alocada na memória principal*/
				if(memoriaPrincipalProcessoArmazenado(&proc->mProcesso, &tpv, indicePagina)){
					//printf("\nProcesso %d pagina %d mem armazenada\n",proc->pid, indicePagina);
				}
				else{/*se a pagina solicitada não estiver armazenada tenta-se armazená-la*/
					if(armazenaNovaPaginaProcesso(&memoriaPrincipal, proc, indicePagina, &tpv, &tpp)){
						//printf("\nProcesso %d pagina %d mem foi armazenada agora\n",proc->pid, indicePagina);

					}
					else{
						printf("\nMemoria principal cheia, abortando Processo PID = %d \n",proc->pid);
						proc->kill = 1;
					}
				}
			}			

                        time(&inicioExec);
                        time(&tempoAtual);
			time(&tempoAnterior);
			while((tempoAtual-inicioExec < list->quantum)){

                                if(proc->kill){
                                        break;
                                }
                                if(shutd){
					memset(mensagem, '\0', 100);
		                        strcpy(mensagem ,"Conexao Encerrada");
                		        send(id, mensagem, 100, 0);
                                        pthread_exit(NULL);
                                }
                                time(&tempoAtual);

				proc->tempoExec = proc->tempoExec-(tempoAtual-tempoAnterior);/*diminui do tempo do processo, o tempo que já foi executado o ciclo*/
				if(tempoAtual - inicioExec >= 1 ){
					tempoAnterior = tempoAtual;
				}

				if(0 >= proc->tempoExec){ /*enquanto tiver tempo de execução para o processo, o ciclo deve continuar, só se o quantum acabar, ai interrompe-se o ciclo*/
					//printf("\ntempoProcesso acabou\n");
					break;
				}
                        }
                        pthread_mutex_lock(&mutexExec);//bloqueia
                        i=1;

                        while(i <= listE->tamanho){/*Após terminar a Execução, procura o processo na lista de execução e o exclui*/

                                noExcluiExec = retornaNoPosicao(listE, i);
                                proc = (processo *)noExcluiExec->memoria;
                                if(proc->pid == pid){

					if(proc->kill){
						excluiProcessoMemoria(&memoriaVirtual, &memoriaPrincipal, &tpv, &tpp, &proc->mProcesso, &proc->pid);
						liberaProcesso(proc);
						excluiEm(listE, i);
					}
					else if(proc->tempoExec <= 0){
						excluiProcessoMemoria(&memoriaVirtual, &memoriaPrincipal, &tpv, &tpp, &proc->mProcesso, &proc->pid);
						liberaProcesso(proc);
                                        	excluiEm(listE, i);
					}
					else{
						//######printf("\nTempo quantum acabou tempo de execucao do processo%d\n", proc->tempoExec);
						tiraEm(listE, i);
						proc->estado=0;/*indica que o estado está em Apto*/
						pthread_mutex_lock(&mutexApto);//bloquei o acesso a lista aptos
						if(!inserefim(listA, (void *)proc)){/*Passa o processo da lista de Aptos para a lista de Aptos*/
                                			pthread_mutex_unlock(&mutexApto);
							printf("\nProblema para armazenar na lista de Aptos\n");
                                			pthread_exit(NULL);/*Finaliza A thread*/
                        			}
						pthread_mutex_unlock(&mutexApto);
					}
					break;
                                }
                                i++;
                        }
                        pthread_mutex_unlock(&mutexExec);//desbloqueia acesso a fila Execução

                }
		else if(shutd){
			memset(mensagem, '\0', 100);
        		strcpy(mensagem ,"Conexao Encerrada");
        		send(id, mensagem, 100, 0);
			pthread_mutex_unlock(&mutexApto);//desloqueia acesso a fila de Aptos
                        pthread_exit(NULL);
                }
                else{
                        pthread_mutex_unlock(&mutexApto);//desloqueia acesso a fila da Aptos
                }

		/*memset(mensagem, '\0', 100);
		strcpy(mensagem, "continua");
		send(id, mensagem, 100, 0);
		memset(mensagem, '\0', 100);
		if(!recv(id, mensagem, 100, 0)){
			printf("\nCliente %d caiu\n",id);
			pthread_exit(NULL);
		}*/
        }

}




int setaServidor( struct sockaddr_in *serv, int *id_serv, int port){

	*id_serv = socket(AF_INET, SOCK_STREAM, 0);/*Cria o socket*/

	if(*id_serv < 0){/*Verifica se o socket foi criado*/
		return 0;
	}

	serv->sin_family = AF_INET;
	serv->sin_port = htons(port);
	serv->sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(*id_serv, (struct sockaddr *) serv, sizeof(struct sockaddr_in)) < 0){
		perror("\nErro ao \"Bindar\" \n");
		close(*id_serv);
		return 0;
	}
	return 1;
}







void legenda(){
        printf("\e[H\e[2J");/*limpa a tela*/
        printf("\n############################################################################\n");
        printf("## Comando CREATE NOME TAMANHO TEMPO_EXECUCAO,          ex.(create a 5 10)##\n");
        printf("## Comando KILL PID                                     ex.(kill 12)      ##\n");
        printf("## Comando TIME                                         ex.(time )        ##\n");
        printf("## Comando PS                                           ex.(ps )          ##\n");
	printf("## Comando PS                                           ex.(ps )          ##\n");
        printf("## Comando SHUTDOWN                                     ex.(shutdown)     ##\n");
	printf("## Comando MEM						ex.(mem)	  ##\n");
	printf("## Comando MEM						ex.(mem v)	  ##\n");
	printf("## Comando MEM						ex.(mem p)	  ##\n");
	printf("## Comando MEM						ex.(mem t)	  ##\n");
        printf("############################################################################\n");
}


void escolheComando(listProcesso *l, int *pid, pthread_t *nucleo){

        /*Var. p/ controle o comando dado pelo usuário*/
        char comando[50];
        char tipoComando[50];
        char nome[50];
	int tamanhoProcesso = 0;
        int tempoExec = 0;
        int pidKill = 0;
	int pidPs;
        time_t tempo;
        int opcao = 0;

	while(1){

                gets(comando);
                opcao = separaComando(comando, tipoComando, nome, &tamanhoProcesso, &tempoExec, &pidKill, &pidPs);

                if(opcao){
                        if(opcao == 1){/*se o comando for create a funcao retorna (1) */
                                if(processoLista(&l->lApto, pid, nome, tamanhoProcesso, tempoExec)){
                                        legenda();
                                        printf("\nProcesso Criado\n");
                                }
                                else{
                                        legenda();
                                        printf("\nProcesso nao Criado\n");
                                }
                        }
			else if(opcao == 2){/*se o comando for (kill) a função retorna (2)*/
                                if( killProcesso(&l->lApto, pidKill)){/*Procura primeiro na lista de Aptos*/
                                        legenda();
                                        printf("\nkill processo PID=%d\n", pidKill);
                                }
                                else if(killProcesso(&l->lExec, pidKill)){/*Procura na lista de Execução*/
                                        legenda();
                                        printf("\nkill processo PID=%d\n", pidKill);
                                }
                                else{
                                        legenda();
                                        printf("\nprocesso PID=%d nao existe\n", pidKill);
                                }
                        }
                        else if(opcao == 3){/*se o comando for (ps) a função retorna (3)*/
                                legenda();
                                printf("\nPID\tNOME PROCESSO\tTAMANHO\tTEMPO EXEC\tTEMPO CRIACAO\tESTADO\n");
                                psProcesso(&l->lExec);/*mostra os processos da lista de execução*/
                                psProcesso(&l->lApto);/*mostra os processos da lista de aptos*/
                        }
			else if(opcao == 4){/*Se o comando for (ps) + (pid de um processo) a função retorna 4 */
				legenda();
				if(psPid(&l->lApto, pidPs)){

				}
				else if(psPid(&l->lExec, pidPs)){								
				
				}
				else{/*caso não ache o processo*/
					printf("\nPid invalido\n");
				}
			}
                        else if(opcao == 5){/*se o comando for (time) a função retorna (4)*/
				legenda();                                
				time(&tempo);
                                mostraTime(&tempo);
                        }
			else if(opcao == 6){/*se o comando for (mem) a função retorna 6*/
				legenda();				
				printf("\nTabela Memoria Virtual");
				imprimeMemoria(&memoriaVirtual);
				printf("\nTabela Memoria Principal");
				imprimeMemoria(&memoriaPrincipal);
			}
			else if(opcao == 7){/*se o comando for (mem) + (v) a função retorna 7*/
				legenda();				
				printf("\nTabela Memoria Virtual");
				imprimeMemoria(&memoriaVirtual);
			}
			else if(opcao == 8){/*se o comando for (mem) + (p) a função retorna 8*/
				legenda();				
				printf("\nTabela Memoria Principal");
				imprimeMemoria(&memoriaPrincipal);
			}
			else if(opcao == 9){/*se o comando for (mem) + (t) a função retorna 9*/
				legenda();				
				printf("\nTABELA TPV");
				imprimeTPV(&tpv);
				printf("\nTABELA TPP");
				imprimeTPP(&tpp);
			}
			else if(opcao == 10){/*se o comando for (shutdown) a função retorna*/
                                legenda();
                                shutd = 1; /*informa a thread nucleo para finalizar-se*/
                                pthread_join(*nucleo, NULL);/*espera a thread nucleo finalizar*/

                                shutdownProcessos(&l->lExec);/*Finaliza os processos da lista de execução */
                                shutdownProcessos(&l->lApto);/*Finaliza os processos da lista de Aptos*/
                                break;
                        }
                }
		else{
                        legenda();
                        printf("\nComando invalido\n");
                }

        }

}


int separaComando(char *comando, char *tipoComando, char *nome, int *tamanhoProcesso, int *tempoExec, int *pidKill, int *pidPs){

        int indice=0;
        int j=0;

        /*Separa até a primeira parte da string que deve ser o comando*/
        while(comando[indice]!=' ' && comando[indice]!='\0'){
                tipoComando[j] = comando[indice];
                indice++;
                j++;
        }

        tipoComando[j] = '\0';/*Coloca o caracter terminador na string tipoComando*/

        /*Comando CREATE ?*/
        if(!strcmp(tipoComando,"create")){
                if(separaCreate(comando, nome, tamanhoProcesso, tempoExec, indice)){
                        return 1;
                }
                return 0;
        }

        /*Comando KILL ?*/
        if(!strcmp(tipoComando, "kill")){
                if(separaKill(comando, pidKill, indice)){
                        return 2;
                }
                return 0;
        }

	 /*Comando PS ?*/
        if(!strcmp(tipoComando, "ps")){
		if(separaPs(comando, pidPs, indice)){
			return 3;
		}
		return 4;		
        }

        /*Comando TIME ?*/
        if(!strcmp(tipoComando, "time")){
                return 5;
        }
	
	/*Comando MEM ?*/
	if(!strcmp(tipoComando, "mem")){
		int argumento;
		if(separaMem(comando, &argumento, indice)){/*se tiver algum argumento*/
			if(argumento){/*se o valor for zero o argumento foi inválido*/		
				return argumento;
			}
			printf("\nArgumento inválido\n");
		}
		return 6;		
        }		

        /*Comando SHUTDOWN ?*/
        if(!strcmp(tipoComando, "shutdown")){
                return 10;
        }

        return 0;

}

int separaCreate(char *comando, char *nome, int *tamanhoProcesso, int *tempoExec, int indice){
        char tempo[10];/*recebe o tempo do processo da string comando, para depois ser convertido em inteiro*/
	char tamanho[10];/*recebe o tamanho do processo da string comando, para depois ser convertido em inteiro*/

        int j=0;
        if(comando[indice] == '\0'){/*verifica se o comando  'create' tem um nome */
                return 0;           /* do prcesso*/
        }
        indice++;

        while(comando[indice] != ' ' && comando[indice] != '\0'){/*separa o nome do processo*/
                nome[j] = comando[indice];
                j++;
                indice++;
        }
        nome[j] = '\0';/*Coloca o finalizador na string*/
        j=0;

        if(comando[indice] == '\0'){/*verifica se o comano é so 'create' e 'nome' */
                return 0;           /*pois o mesmo precisa de um tamanho e tempo de execução*/
        }
        indice++;
	
	while(comando[indice] != ' ' && comando[indice] != '\0'){/*separa o tamanho do processo*/
		tamanho[j] = comando[indice];
		j++;
		indice++;
	}

	tamanho[j] = '\0';/*Coloca o finalizador na string*/
        j=0;

        if(comando[indice] == '\0'){/*verifica se o comano é so 'create' , 'nome' e 'tamanho' */
                return 0;           /*pois o mesmo precisa de um tempo de execução*/
        }
        indice++;

	while(comando[indice] != '\0'){/*separa o tempo de execução do processo*/
                tempo[j] = comando[indice];
                j++;
                indice++;
        }
        tempo[j] = '\0'; /*Coloca o finalizador na string*/

	*tamanhoProcesso = atoi(tamanho);/*converte os numeros da string em int, caso não possua números retorna zero*/

        *tempoExec = atoi(tempo); /*converte os numeros da string em int, caso não possua números retorna zero*/

	if(!(*tamanhoProcesso)){/*não pode ter tamanho igual a zero*/
                return 0;
        }

        if(!(*tempoExec)){/*não pode ter tempo de exec igual a zero*/
                return 0;
        }

        return 1;
}


int separaKill(char *comando, int *pidKill, int indice){

        char pid[10];/*recebe o pid do comando*/
        int j=0;
        if(comando[indice] == '\0'){/*verifica se o comando kill está certo*/
                return 0;
        }
        indice++;

        while(comando[indice] != '\0'){/*pega da string comando o pid informado*/
                pid[j] = comando[indice];
                j++;
                indice++;
        }
        pid[j] = '\0'; /*Coloca o marcado final na string*/

        if(pid[0] == '0'){/*testa aqui, pois quando converte*/
                *pidKill = 0;/*string para inteiro com atoi()*/
                return 1;    /*qualquer caracter que não for*/
        }                    /*número recebe zero*/

        *pidKill = atoi(pid);/*conver a string para inteiro*/

        if(!(*pidKill)){/*se for zero quer dizer que o caracter não era número*/
                return 0;
        }
        return 1;

}

int separaPs(char *comando, int *pidPs, int indice){
	char pid[10];/*recebe o possivel pid informado*/
	int j = 0;

	if(comando[indice] == '\0'){
		return 1;/*o ps é normal, não foi informado nenhum pid*/
	}
	indice++;

	while(comando[indice] != '\0'){/*pega da string comando o pid informado*/
                pid[j] = comando[indice];
                j++;
                indice++;
        }
	*pidPs = atoi(pid);/*converte a string para inteiro*/
	return 0;/*O ps é com o argumento de pid*/	

}

int separaMem(char *comando, int *argumento, int indice){

	char arg;
	if(comando[indice] == '\0'){
		return 0;/*o mem é normal, não foi informado nenhum argumento*/
	}
	indice++;
	arg = comando[indice];
	indice++;
	if(comando[indice] == '\0'){/*se não for verdade o argumento já é considerado inválido*/
		if(arg == 'v'){/*para mostrar a tabela de memória virtual*/
			*argumento = 7;
			return 1; 
		}
		else if(arg == 'p'){/*para mostrar a tabela de memória principal*/
			*argumento = 8;
			return 1;	
		}
		else if(arg == 't'){/*para mostrar as tabelas TPV e TPP*/
			*argumento = 9;
			return 1;
		}
	}
	/*se não for nenhum é um argumento invãlido*/
	*argumento = 0;/*indica que o  argumento é inválido*/
	return 1;

}
int processoLista(lista *l, int *pids,char *nome, int tamanhoProcesso, int tempoExec){/*coloca um processo na lista*/
        processo *p;
        p = criaProcesso((*pids), nome, tamanhoProcesso, tempoExec);/*cria o processo e retorna a referência do mesmo*/
	
	if(!criaProcessoMemoria(&memoriaVirtual, &memoriaPrincipal, &tpv, &tpp, &p->mProcesso, &p->pid, tamanhoProcesso)){
		liberaProcesso(p);
		free(p);
		p = NULL;

	}

        if(p){/*verifica se o processo foi criado*/
                inserefim(l,(void *)p);/*Coloca o novo processo no fim da lista*/
                (*pids)++;
                return 1;
        }
        return 0;
}

int killProcesso(lista *l, int pidKill){
        no *noPid;/*recebe o NÓ da POSICÃO em questão*/
        processo *proPid;/*recebe o PROCESSO do NÓ em questão*/
        int i = 1;/*variavel para comecar a busca pelo primeiro NÓ*/

        while(i<=l->tamanho){/*busca em todos os NÓs*/
                noPid = retornaNoPosicao(l, i);
                proPid = (processo *) noPid->memoria;
                if(proPid->pid == pidKill){
                        if(proPid->estado){/*Se o processo está em execução precisa ser parado*/
                                proPid->kill = 1;/*ou seja, tirado da thread*/
                                return 1;
                        }
			else{
				excluiProcessoMemoria(&memoriaVirtual, &memoriaPrincipal, &tpv, &tpp, &proPid->mProcesso, &proPid->pid);
				liberaProcesso(proPid);
			}
                        excluiEm(l, i);
                        return 1;
                }
                i++;
        }
        return 0;
}

void psProcesso(lista *l){

        struct tm *x;/*recebe a estrutura da hora*/

        int i = 0;
        no *noAtual;
        processo  *pAtual;

        if(l->tamanho){/*se tem processo criado*/
                noAtual = (no*)l->primeiro;
                pAtual = (processo *)noAtual->memoria;

                while(i < l->tamanho){
                        printf("%d\t",pAtual->pid);
                        printf("%s\t\t",pAtual->nome);
                        printf("%d\t",pAtual->tamanho);
                        printf("%d\t\t",pAtual->tempoExec);

                        x=mostraHora(pAtual);
                        printf("%d:%d:%d\t\t",x->tm_hour, x->tm_min, x->tm_sec);

			if(pAtual->estado){
                                printf("%s\n","executando");
                        }
                        else{
                                printf("%s\n","apto");
                        }
                        i++;
                        if(i < l->tamanho){/*Na ultima iteração não tem proximo, então não existe memória para ele*/
                                noAtual = (no*)noAtual->prox;
                                pAtual= (processo *)noAtual->memoria;
                        }
                }
        }

}

int psPid(lista *l, int psPid){

	struct tm *x;/*recebe a estrutura da hora*/
	int i = 0;
	no *noAtual;
	processo *pAtual;

	if(l->tamanho){/*se tiver processo em lista*/
		
		noAtual = (no*)l->primeiro;
                pAtual = (processo *)noAtual->memoria;
		while(i < l->tamanho){/*busca pelo processo correspondente ao psPid*/
		
			if(pAtual->pid == psPid){/*se for esse o processo*/
				printf("\nPID\tNOME PROCESSO\tTAMANHO\tTEMPO EXEC\tTEMPO CRIACAO\tESTADO\n");
				printf("%d\t",pAtual->pid);
                        	printf("%s\t\t",pAtual->nome);
                        	printf("%d\t",pAtual->tamanho);
                        	printf("%d\t\t",pAtual->tempoExec);

                        	x=mostraHora(pAtual);
                        	printf("%d:%d:%d\t\t",x->tm_hour, x->tm_min, x->tm_sec);

				if(pAtual->estado){
                                	printf("%s\n","executando");
                        	}
                        	else{
                                	printf("%s\n","apto");
                        	}
				imprimeMemoriaProcesso(&pAtual->mProcesso);/*imprime a tabela de memória do processo*/
				return 1;/*retorna um quando achou o processo procurado*/
				

			}
			i++;
			if(i < l->tamanho){/*Na ultima iteração não tem proximo, então não existe memória para ele*/
                                noAtual = (no*)noAtual->prox;
                                pAtual= (processo *)noAtual->memoria;
                        }		
		}
		return 0;/*retorna zero quando não acha o processo procurado*/
	}
}

void mostraTime(time_t *t){
        struct tm *time;
        time = gmtime(t);
        legenda();
        printf("\n%d:%d:%d\n", time->tm_hour, time->tm_min, time->tm_sec);

}

void shutdownProcessos(lista *l){
        int i=0;

        no *noAtual;
        processo *procAtual;
        if(l->tamanho){
                noAtual =(no*) l->primeiro;
                procAtual = (processo*) noAtual->memoria;
                while(i < l->tamanho){
                        printf("\nFinalizando Processo\t%d\n",procAtual->pid);
			excluiProcessoMemoria(&memoriaVirtual, &memoriaPrincipal, &tpv, &tpp, &procAtual->mProcesso, &procAtual->pid);
			liberaProcesso(procAtual);
                        if(noAtual->prox){/*Se for o último, não tem próximo*/
                                noAtual = (no*) noAtual->prox;
                                procAtual = (processo*)noAtual->memoria;
                        }
                        i++;
                }
                liberalista(l);/*Exclui todos os processos da lista que fi passada*/
        }
}

int aleatorio(int numInicial, int numFinal){
	srand(time(NULL));
        return (numInicial+(rand()%(numFinal+1)));

}


