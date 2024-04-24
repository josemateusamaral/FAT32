/*
    Esta chamada se sistema retorna os metadados necessarios para acessar o arquivo.
    Se o arquivo já existe, ela retorna uma lista com os clusters do arquivo e os dados da entrada de diretorio
    Se o arquivo não existe, ela cria uma nova entrada e retorna uma lista com o primeiro cluster onde o arquivo começara
*/
struct FAT32__fopen chamadaSistema__fopen( char *filename, char *nomeDisco) {
    
    FILE *disco = fopen(nomeDisco,"r+b");

    //verificar se o arquivo existe ou deve ser criado
    int existe = arquivoExiste(disco,filename);

    //gerar lista de clusters do arquivo
    if(existe){

        //como a arquivo já existe, temos que pegar a entrada dele
        struct entradaDiretorio entradaDoArquivo = pegarEntradaDeDiretorio(disco,filename);
        
        //criar array dos clusters do arquivo
        int* fat = fatToArray(disco);
        int qnt_cluster = entradaDoArquivo.fileSize / 512;
        if(entradaDoArquivo.fileSize % 512){
            qnt_cluster++;
        }
        int clusters[qnt_cluster];
        int clusterAtual = entradaDoArquivo.startCluster;
        for( int i = 0 ; i < qnt_cluster ; i++ ){
            clusters[i] = clusterAtual;
            clusterAtual = fat[clusterAtual];
        }

        // retornar a struct com os metadados do arquivo
        struct FAT32__fopen file;
        memset(&file, 0, sizeof(struct FAT32__fopen));
        strncpy((char *)file.filename, filename, 11);
        file.tamanhoArquivo = entradaDoArquivo.fileSize;
        file.qnt_cluster = qnt_cluster;
        file.clusters = malloc(qnt_cluster * sizeof(int));
        file.posicaoNoArquivo = 0;
        file.disco = disco;
        memcpy(file.clusters, clusters, qnt_cluster * sizeof(int));

        fflush(disco);
        return file;

    }else{

        //como a arquivo não existe ainda, devemos criar uma entrada para ele com um cluster inicial
        int posicaoEntrada = (512 * 32) + 1024 + (acharEntradaVazia(disco) * 32);
        int clusterVazio = acharClusterVazio(disco,-1);
        struct entradaDiretorio entrada;
        memset(&entrada, 0, sizeof(struct entradaDiretorio));
        strcpy(entrada.filename,filename);
        entrada.atributos = 0x20;
        entrada.startCluster = clusterVazio;
        entrada.fileSize = 0;
        fseek(disco, posicaoEntrada, SEEK_SET);
        fwrite(&entrada,sizeof(struct entradaDiretorio),1,disco);

        //marcar o cluster do arquivo
        struct entradaFAT entradaDaFat;
        memset(&entradaDaFat, 0, sizeof(struct entradaFAT));
        entradaDaFat.ponteiro = 0xffffffff;
        int posicaoFatCluster = ( 512 * 32 ) + ( 4 * clusterVazio );
        fseek(disco, posicaoFatCluster, SEEK_SET);
        fwrite(&entradaDaFat,sizeof(struct entradaFAT),1,disco);

        //retorar a struct com os metadados do novo arquivo
        struct FAT32__fopen file;
        memset(&file, 0, sizeof(struct FAT32__fopen));
        int clusters[] = {clusterVazio};
        strncpy((char *)file.filename, filename, 11);
        file.disco = disco;
        file.tamanhoArquivo = 0;
        file.qnt_cluster = 1;
        file.posicaoNoArquivo = 0;
        file.clusters = malloc(1 * sizeof(int));
        memcpy(file.clusters, clusters, 1 * sizeof(int));
        
        fflush(disco);
        return file;
    }
}


/*
    Esta chamada de sistema serve para definir a posicao onde o ponteiro de leitura e escrita esta no arquivo
    Ela recebe SEEK_SET, SEEK_END e SEEK_CUR como parametros tambem igual as outras chamadas
*/
void chamadaSistema__fseek(struct FAT32__fopen * file, int offset, int posicionador){
    
    //salvar a posicao atual para o caso dos parametros estourarem
    //OBS: neste contexto, estourar é quando o file.posicaoNoArquivo for menor que 0 ou maior que o tamanho do arquivo
    int posicaoAtualBackup = file->posicaoNoArquivo;
    
    //definir o posicionador usando o SEEK_SET e o SEEK_END igual as chamadas originais
    switch(posicionador){
        case SEEK_SET:
            file->posicaoNoArquivo = 0;
            break;
        case SEEK_END:
            file->posicaoNoArquivo = file->tamanhoArquivo;
            break;
    }

    //aplicar o offset
    file->posicaoNoArquivo += offset;

    //verificar se estourou
    if( file->posicaoNoArquivo < 0 || file->posicaoNoArquivo > file->tamanhoArquivo ){
        file->posicaoNoArquivo = posicaoAtualBackup;
        printf("\n!! ATENÇÂO: fseek inválido. Você saiu de onde o arquivo esta !!\n");
    }

}


/*
    Esta chamada de sistema realiza a leitura dos bytes de um arquivo para um array
*/
void chamadaSistema__fread( char * recebedor, int tamanhoTipo, int tamanhoLeitura, struct FAT32__fopen * file ){

    int tamanhoCluster = 512;
    int tamanhoTotalDaLeitura = tamanhoTipo * tamanhoLeitura;
    if( tamanhoTotalDaLeitura > tamanhoCluster ){
        printf("\n!! ATENÇÂO: Leitura inválida. \nA implementação atual permite que a leitura tenha o tamanho de no máximo 1 cluster.\nO tamanho do cluster neste disco é de %d bytes. Você esta tentando ler %d bytes!! \n",tamanhoCluster,tamanhoTotalDaLeitura);
        return;
    }

    if( (file->posicaoNoArquivo + tamanhoTotalDaLeitura) > file->tamanhoArquivo ){
        printf("\n!! ATENÇÂO: Leitura inválida. Você esta lendo mais do que o arquivo !!\n");
        return;
    }
    else{
        
        //array que vai abrigar os clusters lidos do arquivo
        unsigned char *cluster = (unsigned char *)malloc( (tamanhoCluster * 2) * sizeof(unsigned char) );

        //pegar primeiro cluster
        int indexDoClusterNoClusterDoArquivo = file->posicaoNoArquivo / tamanhoCluster;
        int numeroClusterAtual = file->clusters[indexDoClusterNoClusterDoArquivo];
        unsigned char *clusterLido = clusterToArray(numeroClusterAtual,file->disco,tamanhoCluster);
        int quantidadeDoCluster = 0;
        for( int i = 0 ; i < tamanhoCluster ; i++ ){
            cluster[quantidadeDoCluster] = clusterLido[i];
            quantidadeDoCluster++;
        }

        //caso tenha mais algum cluster depois desse, pegaremos ele tambem pois como a leitura esta limitada a 
        //um cluster, dessa forma a gente não precisa fazer um algoritmo muito complicado para resolver isso
        if(indexDoClusterNoClusterDoArquivo+1 < file->qnt_cluster){
            numeroClusterAtual = file->clusters[indexDoClusterNoClusterDoArquivo+1]; 
            unsigned char *clusterLido2 = clusterToArray(numeroClusterAtual,file->disco,tamanhoCluster);
            for( int i = 0 ; i < tamanhoCluster ; i++ ){
                cluster[quantidadeDoCluster] = clusterLido2[i];
                quantidadeDoCluster++;
            }            
        }

        //pegando os bytes dos clusters e colocando no array da saida
        int posicaoLeituraNoCluster = file->posicaoNoArquivo % tamanhoCluster;
        for( int i = 0 ; i < tamanhoTotalDaLeitura ; i++ ){
            recebedor[i] = cluster[posicaoLeituraNoCluster];
            posicaoLeituraNoCluster++;
        }

        file->posicaoNoArquivo += tamanhoTotalDaLeitura;

    }
}

void chamadaSistema__fwrite(unsigned char * bytesParaEscrever, int tamanhoTipo, int quantidadeBytes, struct FAT32__fopen * file){

    int tamanhoCluster = 512;
    int tamanhoTotalDaEscrita = quantidadeBytes * tamanhoTipo;

    if( tamanhoTotalDaEscrita > tamanhoCluster ){
        printf("\n!! ATENÇÂO: Escrita inválida. A escrita é limitada ao tamanho de um cluster.\nO tamanho dos clusters deste disco é %d bytes e você está tentando escrever %d bytes !!\n",tamanhoCluster,tamanhoTotalDaEscrita);
        return;
    }
    else{
        
        //calcular os parametros
        int indexDoClusterNoClusterDoArquivo = file->posicaoNoArquivo / tamanhoCluster;
        int numeroClusterAtual = file->clusters[indexDoClusterNoClusterDoArquivo];
        int espacoNoCluster = tamanhoCluster - (file->posicaoNoArquivo % tamanhoCluster);
        int posicaoClusterInicio = ( 512 * 32 ) + 1024 + ( tamanhoCluster * numeroClusterAtual ) + (tamanhoCluster - espacoNoCluster);

        //aumentar o tamanho do arquivo na entrada de diretorio
        struct entradaDiretorio entradaDoArquivo = pegarEntradaDeDiretorio(file->disco,file->filename);
        int tamanhoAtualDoArquivo = entradaDoArquivo.fileSize;
        if(tamanhoAtualDoArquivo < file->posicaoNoArquivo + tamanhoTotalDaEscrita){
            entradaDoArquivo.fileSize = file->posicaoNoArquivo + tamanhoTotalDaEscrita;
        }
        int posicaoEntrada =  pegarPosicaoEntradaDeDiretorio(file->disco,file->filename);
        int posicaoParaNoDisco = (512 * 32) + 1024 + (posicaoEntrada*32);
        fseek(file->disco, posicaoParaNoDisco, SEEK_SET);
        fwrite(&entradaDoArquivo,sizeof(struct entradaDiretorio),1,file->disco);
        fflush(file->disco);

        //verificar se vai tudo em um cluster ou vai ter que dividir em 2
        if(tamanhoTotalDaEscrita > espacoNoCluster){

            //calcular o quanto vai em cada cluster
            int tamanhoParte1 = tamanhoCluster - (file->posicaoNoArquivo % tamanhoCluster);
            int tamanhoParte2 = tamanhoTotalDaEscrita - tamanhoParte1;

            //dividir o array de bytes em 2 arrays separados
            unsigned char *array1 = (unsigned char *)malloc( tamanhoParte1 * sizeof(unsigned char) );
            unsigned char *array2 = (unsigned char *)malloc( tamanhoParte2 * sizeof(unsigned char) );
            int indexDoArrayPrincipal = 0;
            for( int i = 0 ; i < tamanhoParte1 ; i++ ){
                array1[i] = bytesParaEscrever[indexDoArrayPrincipal];
                indexDoArrayPrincipal++;
            }
            for( int i = 0 ; i < tamanhoParte2 ; i++ ){
                array2[i] = bytesParaEscrever[indexDoArrayPrincipal];
                indexDoArrayPrincipal++;
            }

            //escrevendo a primeira parte no primeiro cluster
            fseek(file->disco, posicaoClusterInicio, SEEK_SET);
            fwrite(bytesParaEscrever,1,tamanhoParte1,file->disco);
            file->posicaoNoArquivo += tamanhoParte1;

            //escrevendo a segunda parte no proximo cluster.
            //no caso de não ter proximo cluster ainda é necessário criar
            if( (indexDoClusterNoClusterDoArquivo+1) == file->qnt_cluster){
                //alocar mais um cluster

                //pegar proximo cluster a adicionalo ao file descriptor
                int clusterVazio = acharClusterVazio(file->disco,-1);
                file->clusters[file->qnt_cluster] = clusterVazio;
                file->qnt_cluster++;

                //modificar ultimo cluster para apontar para o novo
                struct entradaFAT entradaDaFat;
                memset(&entradaDaFat, 0, sizeof(struct entradaFAT));
                entradaDaFat.ponteiro = clusterVazio;
                int posicaoFatCluster = ( 512 * 32 ) + ( 4 * file->clusters[indexDoClusterNoClusterDoArquivo] );
                fseek(file->disco, posicaoFatCluster, SEEK_SET);
                fwrite(&entradaDaFat,sizeof(struct entradaFAT),1,file->disco);

                //colocar novo ultimo cluster
                struct entradaFAT entradaDaFatNova;
                memset(&entradaDaFatNova, 0, sizeof(struct entradaFAT));
                entradaDaFatNova.ponteiro = 0xffffffff;
                int posicaoFatClusterNovo = ( 512 * 32 ) + ( 4 * clusterVazio );
                fseek(file->disco, posicaoFatClusterNovo, SEEK_SET);
                fwrite(&entradaDaFatNova,sizeof(struct entradaFAT),1,file->disco);

                //escrever segunda parte no ultimo cluster
                posicaoClusterInicio = ( 512 * 32 ) + 1024 + ( tamanhoCluster * clusterVazio );
                fseek(file->disco, posicaoClusterInicio, SEEK_SET);
                fwrite(array2,1,tamanhoParte2,file->disco);
                file->posicaoNoArquivo += tamanhoParte2;
                
            }else{

                //tem espaço no ultimo cluster ainda, só escrever nele
                numeroClusterAtual = file->clusters[indexDoClusterNoClusterDoArquivo+1];
                posicaoClusterInicio = ( 512 * 32 ) + 1024 + ( tamanhoCluster * numeroClusterAtual );
                fseek(file->disco, posicaoClusterInicio, SEEK_SET);
                fwrite(array2,1,tamanhoParte2,file->disco);
                file->posicaoNoArquivo += tamanhoParte2;
            }

        }else{

            //gravando no cluster. essa gravação ocupa so um cluster
            fseek(file->disco, posicaoClusterInicio, SEEK_SET);
            fwrite(bytesParaEscrever,1,tamanhoTotalDaEscrita,file->disco);
            file->posicaoNoArquivo += tamanhoTotalDaEscrita;
        }

        //mudar ponteiro
        fflush(file->disco);

    }

}


/* 
    acabar com todas as coisas relacionadas ao arquivo
*/
void chamadaSistema__fclose(struct FAT32__fopen * file){
    fflush(file->disco);
    fclose(file->disco);
}