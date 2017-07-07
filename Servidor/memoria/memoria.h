#include"pagina.h"
#include<stdlib.h>
#include<stdio.h>
#include<math.h>

/*MEMORIA*/
typedef struct{

	pagina *pag;
	int qnt_pagina;/*numero de paginas da memoria virtual*/

}memoria;

/*MEMORIA PROCESSO*/
typedef struct{

	int *endPaginaVirtual;
	int qntPaginaProcesso;


}memoriaProcesso;/*tabela de memoria do processo, onde é armazenado o número das páginas lógicas relacionadas com as virtuais do processo */


/*TABELA PAGINA PRINCIPAL*/
typedef struct {

        int *endPaginaVirtual;
        int *endPaginaPrincipal;
        int *pagPrincipalOcupada;
        int qntPagina;

}tabelaPaginaPrincipal;

/*TABELA PAGINA VIRTUAL*/
typedef struct{
	int *endPaginaVirtual;
	int *endPaginaPrincipal;
	int *pagVirtualOcupada;
	int *pagPrincipalOcupada;
	int qntPagina;

}tabelaPaginaVirtual;


/*####################################MEMORIA####################################################*/

void inicializaMemoria(memoria *m, int qnt_pag);
int armazenaNaMemoria(memoria *m, void *p, int tamanho);
void armazenaNaPagina(memoria *m, void *p, int *tamanho );
int armazenaUmaPagina(memoria *m, void *p, int *tamanho);
void excluiDaMemoria(memoria *m, void *p);
void imprimeMemoria(memoria *m);
void liberaMemoria(memoria *m);
void converteBinario(int numero, int *armazena, int tamanho);
void imprimeBinario(int *binario, int tamanho);

int criaProcessoMemoria(memoria *memV, memoria *memP, tabelaPaginaVirtual *tpvv, tabelaPaginaPrincipal *tppp, memoriaProcesso *memProcesso, int *processo, int tamanho);

void excluiProcessoMemoria(memoria *memV, memoria *memP, tabelaPaginaVirtual *tpvv, tabelaPaginaPrincipal *tppp, memoriaProcesso *memProcesso, int *processo);

void inicializaMemoria(memoria *m, int qnt_pag){
	int i;
        i=0;
	m->qnt_pagina = qnt_pag;

	m->pag = (pagina *) malloc(qnt_pag*sizeof(pagina));/*aloca a quantidade de paginas pelo tamanho da memoria*/

        while(i < qnt_pag){
                inicializaPagina(&m->pag[i]);/*seta as celulas de cada pagina como null e a pagina como não ocupada*/
                i++;
        }

}

int armazenaNaMemoria(memoria *m, void *p, int tamanho){

	int comparaTamanho = tamanho;/*tamanho do processo*/
	int i=0;

	armazenaNaPagina(m, p, &tamanho);/*armazena o processo na quantidade necessária de pagina*/

	if(tamanho == comparaTamanho){/*se o tamanho for igual ao tamanho inicial, a memoria etá completamente cheia*/
		printf("\nMEMORIA CHEIA\n");
		return 0;/*se não armazenou retorna 0*/
	}
	if(tamanho){/* caso não for igual ao inicio, mas ainda tamanho é > 0 então não tem espaço suficiente, ai  tira de onde foi armazenado*/
		printf("\nMEMORIA INSUFICIENTE\n");
		i = 0;
		while(i < m->qnt_pagina){
			if(m->pag[i].celula[0] == p){
				inicializaPagina(&m->pag[i]);/*seta as celulas de cada pagina como null e a pagina como não ocupada*/
			}
			i++;
		}
		return 0;/*se não armazenou retorna 0*/
	}
	return 1;/*se armazenou retorna 1*/

}

void armazenaNaPagina(memoria *m, void *p, int *tamanho ){/*Aloca todas as paginas do processo na memória, (usado para memória secundária(memória virtual) )*/

	int i = 0;
	while(i < m->qnt_pagina){
		if(!m->pag[i].ocupada){
			armazenaNaCelula(&m->pag[i], p, tamanho);/*armazena o processo na celula*/
		}
		if(!*tamanho){/*se tem parte do processo para ser armazenado, continua armazenando em outras páginas*/
			break;
		}
		i++;
	}

}

int armazenaUmaPagina(memoria *m, void *p, int *tamanho){/*usado para memoria principal, onde não aloca todas as paginas do processo*/

	int i = 0;
	while(i < m->qnt_pagina){
		if(!m->pag[i].ocupada){
			armazenaNaCelula(&m->pag[i], p, tamanho);/*armazena o processo na celula*/
			return i;
		}
		i++;
	}
	return -1;/*se não armazenar retorna -1*/

}


void excluiDaMemoria(memoria *m, void *p){
	int i = 0;
	while(i < m->qnt_pagina){
		if(m->pag[i].celula[0] == p){
			inicializaPagina(&m->pag[i]);/*seta as celulas de cada pagina como null e a pagina como não ocupada*/
		}
		i++;
	}

}

void imprimeMemoria(memoria *m){
	int i=0;
	int qntBits;/*recebe a quantidade de bits para armazenar o endereco binario*/
	int *binario;

	qntBits=ceil(log2(m->qnt_pagina));/*pega o expoente que elevando o 2 chega na qnt_pagina, após arredonda por excesso */
	/*binario = (int*)malloc(qntBits*sizeof(int));aloca um vetor para armazenar o binário*/

        pagina *pontPagina;
        int indicePaginas = 0;
	printf("\n################################################");
	printf("\n##END #               DESLOCAMENTO            ##");
        while(indicePaginas < m->qnt_pagina){
		printf("\n################################################");
		printf("\n##    #    #    #    #    #    #    #    #    ##");
		i=0;
                pontPagina = &m->pag[indicePaginas];
		printf("\n## %.2d", indicePaginas);
		imprimePagina(pontPagina);
                indicePaginas++;
		printf(" ##");
		printf("\n##    #    #    #    #    #    #    #    #    ##");
        }
	printf("\n################################################");
	/*free(binario);*/
}


void liberaMemoria(memoria *m){

	free(m->pag);/*libera o array de paginas alocado dinâmicamente*/
}


int criaProcessoMemoria(memoria *memV, memoria *memP, tabelaPaginaVirtual *tpvv, tabelaPaginaPrincipal *tppp, memoriaProcesso *memProcesso, int *processo, int tamanho){

	int enderecoPrincipal;
	int i;

	if(armazenaNaMemoria(memV, (void *)processo, tamanho)){

		//inicializaMemoriaProcesso(memProcesso,(int) ceil(tamanho/8.0));/*cell arredonda para cima*/
		criaMemoriaProcesso(memProcesso, memV, (void *)processo);

		alocaVirtualTPV(tpvv, memProcesso);/*marca como ocupadas as paginas virtuais do processo na tabela TPV*/

		i = 0;
		while(i < memProcesso->qntPaginaProcesso){/*coloca as paginas do processo na memória principal de forma intercalada*/
			enderecoPrincipal = armazenaUmaPagina(memP, (void*)processo, &tamanho);
			if(enderecoPrincipal < 0){
				printf("\nmemoria principal insuficiente\n");
				excluiProcessoMemoria(memV, memP, tpvv, tppp, memProcesso, processo);
				return 0;
			}
			alocaPrincipalTPV(tpvv, memProcesso->endPaginaVirtual[i], enderecoPrincipal);/*relaciona as paginas virtual e a pagina principal, onde está armazenado o processo, da tabela TPV*/
			alocaVirtualTPP(tppp, enderecoPrincipal, memProcesso->endPaginaVirtual[i]);/*relaciona as paginas principais e a virtual, onde está armazenado o processo, da tabela TPP  */
			i = i+2;
			tamanho = tamanho-8;
		}
		
		return 1;
	}
	return 0;


}


void excluiProcessoMemoria(memoria *memV, memoria *memP, tabelaPaginaVirtual *tpvv, tabelaPaginaPrincipal *tppp, memoriaProcesso *memProcesso, int *processo){

	excluiDaMemoria(memP, (void*)processo);
	int enderecoPrincipal;
	int enderecoVirtual;
	int i = 0;
	while(i < memProcesso->qntPaginaProcesso){
		enderecoVirtual = memProcesso->endPaginaVirtual[i];
		if(tpvv->pagPrincipalOcupada[enderecoVirtual]){/*se a pagina principal estiver ocupada*/
			enderecoPrincipal = tpvv->endPaginaPrincipal[enderecoVirtual];
			excluiProcessoTPP(tppp, enderecoPrincipal);
		}
		excluiProcessoTPV(tpvv, enderecoVirtual);
		i++;

	}
	excluiDaMemoria(memV, (void*)processo);
}


/*void converteBinario(int numero, int *armazena, int tamanho){

	while (numero){

		armazena[tamanho-1] = numero%2;
		numero = numero/2;
		tamanho--;

	}
ti	while(0 < tamanho){
		armazena[tamanho-1] = 0;
		tamanho--;
	}


}

void imprimeBinario(int *binario, int tamanho){

	int i = 0;
	while(i < tamanho){
		printf("%d",binario[i]);
		i++;
	}

}*/

/*###############################################################################################*/

/*######################################MEMORIA PROCESSO#########################################*/
void inicializaMemoriaProcesso(memoriaProcesso *mp, int qntPagina);
void criaMemoriaProcesso(memoriaProcesso *mp, memoria *m, void *processo);
int memoriaPrincipalProcessoArmazenado(memoriaProcesso *mp, tabelaPaginaVirtual *tpv, int indicePagProcesso);
void imprimeMemoriaProcesso(memoriaProcesso *mp);
void liberaMemoriaProcesso(memoriaProcesso *mp);



void inicializaMemoriaProcesso(memoriaProcesso *mp, int qntPagina){

        mp->endPaginaVirtual = (int*) malloc(qntPagina*sizeof(int));/*cria um vetor com o tamanho de paginas do processo */
        mp->qntPaginaProcesso = qntPagina;

}

void criaMemoriaProcesso(memoriaProcesso *mp, memoria *m, void *processo){/*relaciona as paginas do processo com as paginas virtuais*/

        int i=0;
        int j=0;

        while(i < mp->qntPaginaProcesso){

                while(j < m->qnt_pagina){

                        if(m->pag[j].ocupada){/*se a pagina está ocupada verifica se é o processo*/

                                if(m->pag[j].celula[0] == processo){/*se for o endereco do processo*/

                                        mp->endPaginaVirtual[i] = j;/*pega o endereço da página da página e passa para a tabela da memoria do processo*/
                                        j++;
                                        break;

                                }
                        }
                        j++;
                }
                i++;
        }


}

int memoriaPrincipalProcessoArmazenado(memoriaProcesso *mp, tabelaPaginaVirtual *tpv, int indicePagProcesso){/*verifica se a pagina do processo informada está armazenada na memória principal*/
 
	int indicePagVirtual;/*recebe o endereço da página virtual referente a pagina do processo*/
	indicePagVirtual = mp->endPaginaVirtual[indicePagProcesso];/*pagina processo (indicePagProcesso) que está na (endPaginaVirtual[indicePagProcesso])*/
	if(tpv->pagPrincipalOcupada[indicePagVirtual]){/*se a pagina virtual também está na memória principal, retorna 1, caso contrário retorna 0*/
		return 1;
	}
	return 0;

}

void imprimeMemoriaProcesso(memoriaProcesso *mp){

        int i = 0;
	printf("\nMEMORIA PROCESSO\n");
        while(i < mp->qntPaginaProcesso){
		printf("#######################\n");
                printf("## NPL # %d # NPV # %d ##\n",i, mp->endPaginaVirtual[i]);/*o indice i é o end da pagina logica referente ao endereço da pagina virtual do processo*/
		i++;

        }
	printf("#######################\n");

}

void liberaMemoriaProcesso(memoriaProcesso *mp){

	free(mp->endPaginaVirtual);

}

/*###############################################################################################*/

/*#####################################TABELA PAGINA PRINCIPAL###################################*/
void inicializaTPP(tabelaPaginaPrincipal *t, int qntPagina);
void alocaVirtualTPP(tabelaPaginaPrincipal *t, int endPrincipal, int endVirtual);
void imprimeTPP(tabelaPaginaPrincipal *t);
void excluiProcessoTPP(tabelaPaginaPrincipal *t, int endPaginaPrincipal);
void liberaTPP(tabelaPaginaPrincipal *t);

void inicializaTPP(tabelaPaginaPrincipal *t, int qntPagina){

	int i = 0;

	t->endPaginaVirtual = (int *) malloc(qntPagina *sizeof(int));
	t->endPaginaPrincipal = (int *) malloc(qntPagina *sizeof(int));
	t->pagPrincipalOcupada = (int *)malloc(qntPagina *sizeof(int));
	t->qntPagina = qntPagina;

	while(i < qntPagina){
		t->endPaginaPrincipal[i] = i;
		t->endPaginaVirtual[i] = -1;
		t->pagPrincipalOcupada[i] = 0;
		i++;

	}

};


void alocaVirtualTPP(tabelaPaginaPrincipal *t, int endPrincipal, int endVirtual){

	t->endPaginaVirtual[endPrincipal] = endVirtual;
	t->pagPrincipalOcupada[endPrincipal] = 1;

}

void imprimeTPP(tabelaPaginaPrincipal *t){

	int i = 0;
	printf("\n#################");	
	printf("\n##NPP #NPV # BP##");
	while(i < t->qntPagina){
		printf("\n#################");
		printf("\n##    #    #   ##");
		printf("\n## %.2d #",t->endPaginaPrincipal[i]);
		if(t->endPaginaVirtual[i] < 0){
			printf(" %d #",t->endPaginaVirtual[i]);
		}
		else{
			printf("  %d #",t->endPaginaVirtual[i]);
		}
		printf(" %d ##",t->pagPrincipalOcupada[i]);
		i++;
		printf("\n##    #    #   ##");
	}
	printf("\n#################\n");
		

}

void excluiProcessoTPP(tabelaPaginaPrincipal *t, int endPaginaPrincipal){

       	t->endPaginaVirtual[endPaginaPrincipal] = -1;
       	t->pagPrincipalOcupada[endPaginaPrincipal] = 0;

}


void liberaTPP(tabelaPaginaPrincipal *t){

	free(t->endPaginaPrincipal);
	free(t->endPaginaVirtual);
	free(t->pagPrincipalOcupada);

}

/*###############################################################################################*/


/*#####################################TABELA PAGINA VIRTUAL#####################################*/
void inicializaTPV(tabelaPaginaVirtual *t, int qnt_pagina);
void alocaVirtualTPV(tabelaPaginaVirtual *t, memoriaProcesso *mp);
void alocaPrincipalTPV(tabelaPaginaVirtual *t, int endPaginaVirtual, int endPaginaPrincipal);
void imprimeTPV(tabelaPaginaVirtual *t);
void excluiProcessoTPV(tabelaPaginaVirtual *t, int endPaginaVirtual);
void liberaTPV(tabelaPaginaVirtual *t);


void inicializaTPV(tabelaPaginaVirtual *t, int qnt_pagina){

	int i = 0;

	t->endPaginaVirtual = (int*)malloc(qnt_pagina*sizeof(int));
	t->endPaginaPrincipal = (int*)malloc(qnt_pagina*sizeof(int));
	t->pagVirtualOcupada = (int*)malloc(qnt_pagina*sizeof(int));
	t->pagPrincipalOcupada = (int*)malloc(qnt_pagina*sizeof(int));
	t->qntPagina = qnt_pagina;

	while(i < qnt_pagina){
		t->endPaginaVirtual[i] = i;
		t->pagVirtualOcupada[i] = 0;
		t->endPaginaPrincipal[i] = -1;
		t->pagPrincipalOcupada[i] = 0;
		i++;

	}

}

void alocaVirtualTPV(tabelaPaginaVirtual *t, memoriaProcesso *mp){/*determina que a pagina virtual está ocupada*/

	int i = 0;
	while(i < mp->qntPaginaProcesso){/*coloca em estado ocupado na lista TPV as paginas que estão ocupadas na memoria virtual*/
		t->pagVirtualOcupada[mp->endPaginaVirtual[i]] = 1;
		i++;

	}

}


void alocaPrincipalTPV(tabelaPaginaVirtual *t, int endPaginaVirtual, int endPaginaPrincipal){/*determina que on processo que está nesta pagina virtual também está na outra pagina da memória principal*/


	t->pagPrincipalOcupada[endPaginaVirtual] = 1;
	t->endPaginaPrincipal[endPaginaVirtual] = endPaginaPrincipal;


}


void imprimeTPV(tabelaPaginaVirtual *t){

	int i = 0;
	printf("\n#######################");
	printf("\n##NPV #NPP # BP # BV ##");
	while(i < t->qntPagina){
		printf("\n#######################");
		printf("\n##    #    #    #    ##");
		printf("\n## %.2d #",t->endPaginaVirtual[i]);
		if(t->endPaginaPrincipal[i]<0){
			printf(" %d #",t->endPaginaPrincipal[i]);
		}
		else{		
			printf("  %d #",t->endPaginaPrincipal[i]);
		}
		printf("  %d #",t->pagPrincipalOcupada[i]);
		printf("  %d ##",t->pagVirtualOcupada[i]);
		i++;
		printf("\n##    #    #    #    ##");
	}
	printf("\n#######################\n");

}

void excluiProcessoTPV(tabelaPaginaVirtual *t, int endPaginaVirtual){


	t->endPaginaPrincipal[endPaginaVirtual] = -1;
	t->pagPrincipalOcupada[endPaginaVirtual] = 0;
       	t->pagVirtualOcupada[endPaginaVirtual] = 0;

}

void liberaTPV(tabelaPaginaVirtual *t){

	free(t->endPaginaVirtual);
	free(t->endPaginaPrincipal);
	free(t->pagPrincipalOcupada);
	free(t->pagVirtualOcupada);

}


/*###############################################################################################*/





