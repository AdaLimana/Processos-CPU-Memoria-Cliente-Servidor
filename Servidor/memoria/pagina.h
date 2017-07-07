#include<stdio.h>
#include<stdlib.h>
#define BITS 8 /*define a quantidade de bits das celudas das paginas*/

typedef struct{

	void *celula[BITS];/*tem a referencia para um processo*/
	int ocupada;/*se seu valor é 1 caso a pag estiver ocupada, caso contrário sera 0*/
}pagina;

void inicializaPagina(pagina *p);
void armazenaNaCelula(pagina *pag, void *p, int *tamanho);
void imprimePagina(pagina *p);

void inicializaPagina(pagina *p){

	int i=0;
	while(i < BITS){

		p->celula[i] = NULL;
		i++;
	}
	p->ocupada = 0;
}


void armazenaNaCelula(pagina *pag, void *p, int *tamanho){

	int i = 0;
	pag->ocupada = 1;
	while(i < BITS){
		pag->celula[i] = p;
		(*tamanho)--;/*vai diminuindo o tamanho do processo a cada armazenamento*/
		if(!(*tamanho)){/*se o processo já foi armazenado por completo termina de armazenar na celula*/
			break;
		}
		i++;
	}
}

void imprimePagina(pagina *p){

        int valorCelula;
        int indiceCelula = 0;
        while(indiceCelula < BITS){

                if(p->celula[indiceCelula]){
                        valorCelula = *(int*)p->celula[indiceCelula];
			printf(" # %.2d", valorCelula);
                }
                else{
                        valorCelula = -1;
			printf(" # %d", valorCelula);                
		}
		
		indiceCelula++;

        }

}

