#include <stdlib.h>
#include <time.h>
#include<math.h>
#include <string.h>
#include"memoria/memoria.h"

typedef struct{

	int pid;
	char *nome;
	time_t criacao;
	int tempoExec;
	int estado;/*Se o estado for 1 o processo está em execução, se for 0 está em espera(apto)*/
	int kill;/*setada para um quando o processo a ser matado é o que está "executando" na thread*/
	int tamanho;/*Tem o tamanho em Bytes do processo*/
	memoriaProcesso mProcesso;/*tabela de memória do processo*/
	int qntCiclo;/*armazena a quantidade de vezes que o processo passou pelo processador*/
	int qntPagina;
	
}processo;

processo* criaProcesso(int pidProcesso, char *nomeProcesso, int tamanho, int tempoProcesso );
int armazenaNovaPaginaProcesso(memoria *m, processo *proc, int indicePagina, tabelaPaginaVirtual *tpv, tabelaPaginaPrincipal *tpp);
void liberaProcesso(processo *p);
//char * mostraHora(processo *processo);
struct tm *mostraHora(processo *processo);


processo* criaProcesso(int pid, char *nomeProcesso,  int tamanho, int tempoExecProcesso){
  processo *novoProcesso = (processo*)malloc(sizeof(processo));

  if(!novoProcesso){/*Caso não cria processo retorna erro*/
      return novoProcesso;
  }
  novoProcesso->pid = pid;
  
  novoProcesso->tamanho = tamanho;/*armazena o tamanho do processo*/
  /*inicializa a tabela de memória do processo*/
  novoProcesso->qntPagina = (int) ceil(tamanho/8.0);/*cada pagina armazena 8 bytes então aqui determina a quantidade de pãginas, ceil arredonda para cima*/
	
  inicializaMemoriaProcesso(&novoProcesso->mProcesso, novoProcesso->qntPagina);

  novoProcesso->tempoExec = tempoExecProcesso;/*armazena o tempo do processo*/
  time(&novoProcesso->criacao);/*armazena a hora que foi criado*/

  /*Aloca dinamicamente uma string para o nome processo*/
  novoProcesso->nome = (char*) malloc(strlen(nomeProcesso)*sizeof(char));
  strcpy(novoProcesso->nome, nomeProcesso);
  novoProcesso->estado = 0;/*quando é criado fica como zero, significa que está apto, e não execução*/
  novoProcesso->kill = 0;/*setado para 1 quando o processo que está em execução recebe um kill*/
  novoProcesso->qntCiclo = 0;/*Seta para zero pois não passou nenhuma vez pelo processador*/
  
  return novoProcesso;
}


int armazenaNovaPaginaProcesso(memoria *m, processo *proc, int indicePagina, tabelaPaginaVirtual *tpv, tabelaPaginaPrincipal *tpp){
	
	memoriaProcesso *mp = &proc->mProcesso;
	int enderecoPrincipal;
	int tamanhoProcesso; 
	tamanhoProcesso  = proc->tamanho-(BITS*indicePagina);/*determina o tamanho da pagina subtraindo os byts do processo alococados pelas paginas anteriores*/
	
	enderecoPrincipal = armazenaUmaPagina(m, (void*)&proc->pid,&tamanhoProcesso);
	if(enderecoPrincipal < 0){

		return 0;
	}
	alocaPrincipalTPV(tpv, mp->endPaginaVirtual[indicePagina], enderecoPrincipal);/*relaciona as paginas virtual e a pagina principal, onde está armazenado o processo, da tabela TPV*/
	alocaVirtualTPP(tpp, enderecoPrincipal, mp->endPaginaVirtual[indicePagina]);/*relaciona as paginas principais e a virtual, onde está armazenado o processo, da tabela TPP  */
	return 1;

}

void liberaProcesso(processo *p){
	free(p->nome);
	liberaMemoriaProcesso(&p->mProcesso);	

}


/*char * mostraHora(processo *processo){ctime retorna uma string com dia mes ano h m s
    return ctime(&processo->criacao);
}*/



struct tm* mostraHora(processo *processo){/*retorna uma estrutura com */
	return gmtime(&processo->criacao);/*dia mes ano h m s */
}					  /*ai só mostra o que quiser*/
					  /*no caso h:m:s*/
