#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "includes/estruturas.c"
#include "includes/terminal.c"
#include "includes/gerador.c"

int main() {

    printf("\n\n\n");

    // criar imagem de disco FAT32
    criarDisco("FAT.img",512,16,1);

    // gravar o arquivo teste.txt no arquivo da imagem do FAT32
    // gravarArquivo("arquivosTeste/teste.txt","FAT.img","teste   txt");
    gravarArquivo("arquivosTeste/letra-a.png","FAT.img","letra   png");
    gravarArquivo("arquivosTeste/teste.txt","FAT.img","teste   txt");
    gravarArquivo("arquivosTeste/letra-a.png","FAT.img","letra2  png");
    // gravarArquivo("arquivosTeste/teste2.txt","FAT.img","teste2  txt");

    // ler arquivo da imagem do disco FAT32
    // lerArquivo("teste2  txt","FAT.img","arquivosTeste/novoTeste.txt");
    lerArquivo("letra   png","FAT.img","arquivosTeste/imagem.png");

    // checar os conteudos do diretorio root
    // terminarl__ls("FAT.img");

    return 0;
}