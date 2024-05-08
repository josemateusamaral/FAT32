#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "includes/xtypes.c"
#include "includes/xsyscalls.c"
#include "includes/terminal.c"
#include "includes/xsystools.c"



int main() {

    printf("\n\n\n");



    // criar imagem de disco FAT32
    criarDisco("FAT.img",512,128,1);
    xmount("FAT.img");

    // gravar o arquivo teste.txt no arquivo da imagem do FAT32
    gravarArquivo("data/letra-a.png","letra.png");
    gravarArquivo("data/teste.txt","teste.txt");
    gravarArquivo("data/letra-a.png","letra2.png");
    copiarArquivo("letra.png","temps/imagem.png");

    //chamada de sistema remove
    xremove("letra2.png","FAT.img");

    //chamada de sistema fopen
    XFILE file = xopen("letra.png");

    //chamada de sistema fseek
    xseek(&file,0,SEEK_SET);

    //chamada de sistema fread
    int tamanhoLeitura = 200;
    unsigned char bytesLidos[tamanhoLeitura];
    xread( &bytesLidos[0], sizeof( unsigned char), tamanhoLeitura, &file );
    printf("\nBytes lidos: ");
    for( int i = 0 ; i < tamanhoLeitura ; i++ ){
        printf("%02X ",bytesLidos[i]);
    }
    printf("\n\n");

    //chamada de sistema fwrite
    xseek(&file,600,SEEK_SET);
    int tamanhoEscrita = 500;
    unsigned char *bytesParaEscrever = (unsigned char *)malloc( tamanhoEscrita * sizeof(unsigned char) );
    for (int i = 0; i < tamanhoEscrita ; i++) {
        bytesParaEscrever[i] = 0xaa;
    }
    xwrite(bytesParaEscrever,sizeof(unsigned char),tamanhoEscrita,&file);

    //chamada de sistema fclose
    xclose(&file);





    //buffer usado para leitura e escrita e numero de bytes da leitura
    int bytes_lidos;
    unsigned char *bufferLeituraEscrita = (unsigned char *)malloc( 512 * sizeof(unsigned char) );


    //testes de criacao de arquivo usando a chamada de sistema
    FILE * arquivoEntrada = fopen("data/letra-a.png","rb");
    XFILE fileNoFat = xopen("arquivo.png");
    while(true){
        bytes_lidos = fread(&bufferLeituraEscrita[0], 1, 200, arquivoEntrada);
        if(!bytes_lidos){
            break;
        }else{
            xwrite(bufferLeituraEscrita,1,bytes_lidos,&fileNoFat);
        }
    }
    fclose(arquivoEntrada);
    xclose(&fileNoFat);


    //testar a copia de um arquivo do disco FAT32 para fora
    FILE * arquivoSaida = fopen("temps/arquivoSaida.png","wb");
    XFILE fileDentro = xopen("arquivo.png");
    while(true){
        bytes_lidos  = xread( &bufferLeituraEscrita[0], 1, 512, &fileDentro );
        if(!bytes_lidos){
            break;
        }else{
            fwrite(bufferLeituraEscrita, sizeof(unsigned char), bytes_lidos, arquivoSaida);
        }
    }
    fclose(arquivoSaida);
    xclose(&fileDentro);

    
    
    
    terminal__ls("FAT.img");


    return 0;

}