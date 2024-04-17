#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "includes/estruturas.c"
#include "includes/utils.c"
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
    lerArquivo("letra   png","FAT.img","arquivosTeste/imagem.png");



    //chamada de sistema fopen
    struct FAT32__fopen file = chamadaSistema__fopen("letra   png","FAT.img");


    //chamada de sistema fseek
    chamadaSistema__fseek(&file,500,SEEK_SET);


    //chamada de sistema fread
    int tamanhoLeitura = 200;
    unsigned char bytesLidos[tamanhoLeitura];
    chamadaSistema__fread( &bytesLidos[0], sizeof( unsigned char), tamanhoLeitura, file );
    //printf("\nBytes lidos: ");
    //for( int i = 0 ; i < tamanhoLeitura ; i++ ){
    //    printf("%02X ",bytesLidos[i]);
    //}
    //printf("\n\n");


    //chamada de sistema fwrite
    chamadaSistema__fseek(&file,700,SEEK_SET);
    int tamanhoEscrita = 500;
    unsigned char *bytesParaEscrever = (unsigned char *)malloc( tamanhoEscrita * sizeof(unsigned char) );
    for (int i = 0; i < tamanhoEscrita ; i++) {
        bytesParaEscrever[i] = 0xaa;
    }
    chamadaSistema__fwrite(bytesParaEscrever,sizeof(unsigned char),tamanhoEscrita,file);


    //chamada de sistema fclose
    chamadaSistema__fclose(&file);


    //printarFat("FAT.img");
    //terminarl__ls("FAT.img");

    return 0;

}