#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "estruturas.c"

/*
    A Funcao a baixo retorna uma lista com 512bytes zerados.
    Isso eh usado sempre quano é necessário escrever um setor no disco.
    Em algumas ocasioes a lista retornada é alterada antes de ser gravada no disco.
*/
unsigned char* genSetorVazio( int tamanhoSetores ) {
    unsigned char *setor = (unsigned char *)malloc( tamanhoSetores * sizeof(unsigned char) );

    for (int i = 0; i < tamanhoSetores ; i++) {
        setor[i] = 0x0;
    }

    return setor;
}


unsigned char* genSetorVazioTeste( int tamanhoSetores ) {
    unsigned char *setor = (unsigned char *)malloc( tamanhoSetores * sizeof(unsigned char) );

    for (int i = 0; i < tamanhoSetores ; i++) {
        setor[i] = 0xAA;
    }

    return setor;
}


/*
    Esta função serva para testar as structs e ver se estão sendo gravados do jeito certo.
    Estruturas testadas: 
        entradaFAT -            tamanho 32bytes  - OK
        blocoParametrosBios -   tamanho 512bytes - OK
        setorInformacoesFS  -   tamanho 512bytes - OK

        OBS: 512 decimal == 200 hexadecimal

*/
void testarStruct(){
    /* 
        Abrir o buffer para criar o arquivo de imagem do FAT32 
    */
    FILE *file = fopen("FAT.img","wb");
    struct entradaFAT entrada;
    strcpy(entrada.filename,"teste   txt");
    entrada.atributos = 0x04;
    entrada.startCluster = 2;
    entrada.fileSize = 11;

    fwrite(&entrada,sizeof(struct entradaFAT),1,file);
    fclose(file);

}

unsigned char* gravarArquivo( char* nomeDoArquivo, char* nomeDisco ) {

    int tamanhoArquivo = 11;

    // pegar arquivo e colocar em uma lista de bytes
    unsigned char *arquivo = (unsigned char *)malloc( tamanhoArquivo * sizeof(unsigned char) );
    FILE *file = fopen(nomeDoArquivo,"rb");
    int result = fread (&arquivo[0], sizeof(char), tamanhoArquivo, file);
    fclose(file);

    // criar a struct do arquivo
    struct entradaFAT entrada;
    strcpy(entrada.filename,"teste   txt");
    entrada.atributos = 0x04;
    entrada.startCluster = 2;
    entrada.fileSize = tamanhoArquivo;

    // colocar arquivo no diretorio root
    int inicioRoot = ( 32 * 512 ) + 1024;
    FILE *disco = fopen(nomeDisco,"r+b");
    fseek(disco, inicioRoot, SEEK_SET);
    fwrite(&entrada,sizeof(struct entradaFAT),1,disco);

    // colocar os bytes do arquivo no cluster
    int inicioCluster = inicioRoot + 512;
    int indexCluster = 2;
    int posicaoFatArquivo = ( 512 * 32 ) + 32
    fseek(disco, inicioRoot, SEEK_SET);
    fwrite(&entrada,sizeof(struct entradaFAT),1,disco);

    return arquivo;
}


void criarDisco(char* nomeDoArquivo, int tamanhoSetores, int quantidadeClusters, int quantidadeSetoresPorCluster){
    
    /*
        Tamanho do sistema de arquivos
        Cada cluster pode ter 1, 2, 4, 8, 16, 32, 64 ou 128 setores
        Cada setor pode ter 128, 256, 512 ou 1024 bytes
        A quantidade de entradas FAT é igual a quantidade de clusters * 2 pois existem duas FATs
    */
    int quantidadeDataSectors = quantidadeClusters * quantidadeSetoresPorCluster;
    int quantidadeSetoresReservados = 32;


    /* 
        Abrir o buffer para criar o arquivo de imagem do FAT32 
    */
    FILE *file = fopen(nomeDoArquivo,"wb");



    /* 
       Colocando os 32 setores reservados no inicio do FAT32
       Estes setores representam o Boot Sector, FS Information Sector e Additional Reserved Sectors (Optional)
    */

    // Boot Sector
    unsigned char *setorDeBoot = genSetorVazio(tamanhoSetores);
    fwrite(setorDeBoot,sizeof(unsigned char),tamanhoSetores,file);
    free(setorDeBoot);

    // File System Information Sector
    unsigned char *setorDeInformacoes = genSetorVazio(tamanhoSetores);
    fwrite(setorDeInformacoes,sizeof(unsigned char),tamanhoSetores,file);
    free(setorDeInformacoes);

    // setores reservados opcionais
    for( int i = 0 ; i < quantidadeSetoresReservados - 2; i++ ){
        unsigned char *setorReservado = genSetorVazio(tamanhoSetores);
        fwrite(setorReservado,sizeof(unsigned char),tamanhoSetores,file);
        free(setorReservado);
    }



    /*
        Colocando as entradas FAT32.
        As entradas são compostas de structs de 32bytes.
        O numero de entradas é igual ao dobro do numero de clusters pois existem duas tabelas FAT32.
    */
    for( int j = 0 ; j < 2 ; j++ ){
        for( int i = 0 ; i < quantidadeClusters ; i++ ){
            struct entradaFAT entrada;
            fwrite(&entrada,sizeof(struct entradaFAT),1,file);
        }
    }
    
    

    /*
        Criando tabela Root
    */

    //for( int i = 0 ; i < 16 ; i++ ){
    //    struct entradaFAT entrada;
    //    //entrada.filename[11];   
    //    entrada.atributos = 0;  
    //    //entrada.reservados = [0,0,0,0,0,0,0,0,0,0]; 
    //    entrada.time = 0;       
    //    entrada.date = 0;       
    //    entrada.startCluster = 0;
    //    entrada.fileSize = 0; 
    //    
    //    fwrite(&entrada,sizeof(struct entradaFAT),1,file);
//
    //}
    unsigned char *setorDoCluster = genSetorVazio(tamanhoSetores);
    fwrite(setorDoCluster,sizeof(unsigned char),tamanhoSetores,file);
    free(setorDoCluster);


    /*
        Adicionando os Clusters ao disco.
        O numero de setores adicionados será igual ao numero de clusters * o numero de setores por cluster.
    */
    for( int i = 0 ; i < quantidadeClusters - 1 ; i++ ){
        for( int j = 0 ; j < quantidadeSetoresPorCluster ; j++ ){
            unsigned char *setorDoCluster = genSetorVazio(tamanhoSetores);
            fwrite(setorDoCluster,sizeof(unsigned char),tamanhoSetores,file);
            free(setorDoCluster);
        }
    }
    


    // fechar o arquivo de imagem
    fclose(file);

}


/* ESTRUTURA DO DISCO
512 * 32 = setores reservados no começodo disco
( 32 * 16 ) * 2 = Tabelas FAT 2 vezes


*/

int main() {

    // criar imagem de disco FAT32
    
    //testarStruct();
    criarDisco("FAT.img",512,16,1);
    gravarArquivo("teste.txt","FAT.img");

    return 0;
}
