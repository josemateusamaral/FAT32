#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "includes/estruturas.c"
#include "includes/terminal.c"
#include "includes/gerador.c"
#include "includes/chamadasSistema.c"

int main() {

    printf("\n\n\n");

    // criar imagem de disco FAT32
    criarDisco("FAT.img",512,16,1);

    // gravar o arquivo teste.txt no arquivo da imagem do FAT32
    gravarArquivo("arquivosTeste/letra-a.png","FAT.img","letra   png");
    gravarArquivo("arquivosTeste/teste.txt","FAT.img","teste   txt");
    gravarArquivo("arquivosTeste/letra-a.png","FAT.img","letra2  png");

    //listarClustersOcupados("FAT.img");

    // ler arquivo da imagem do disco FAT32
    //printf("\n\n");
    lerArquivo("letra   png","FAT.img","arquivosTeste/imagem.png");

    //listarClustersOcupados("FAT.img");
    // apagar arquivo no meio dos clusters usados
    //apagarArquivo("teste   txt","FAT.img");

    // checar os conteudos do diretorio root
    //terminarl__ls("FAT.img");
    //apagarArquivo("teste   txt","FAT.img");
    //terminarl__ls("FAT.img");

    // printar entrada FAT ocupdas
    

    return 0;
}