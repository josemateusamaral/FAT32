

/*
    Esta chamada se sistema retorna os metadados necessarios para acessar o arquivo.
    Se o arquivo já existe, ela retorna uma lista com os clusters do arquivo e os dados da entrada de diretorio
    Se o arquivo não existe, ela cria uma nova entrada e retorna uma lista com o primeiro cluster onde o arquivo começara
*/
XFILE xopen( char *filename ) {

    //disco = fopen(nomeDisco,"r+w");

    //verificar se o arquivo existe ou deve ser criado
    int existe = arquivoExiste(filename);

    //gerar lista de clusters do arquivo
    if(existe){

        //como a arquivo já existe, temos que pegar a entrada dele
        EntradaDiretorio entradaDoArquivo = pegarEntradaDeDiretorio(filename);
        
        //criar array dos clusters do arquivo
        int* fat = fatToArray(xdisco);
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
        XFILE file;
        memset(&file, 0, sizeof(XFILE));
        strncpy((char *)file.filename, filename, 11);
        file.tamanhoArquivo = entradaDoArquivo.fileSize;
        file.qnt_cluster = qnt_cluster;
        file.clusters = malloc(qnt_cluster * sizeof(int));
        file.posicaoNoArquivo = 0;
        file.disco = xdisco;
        memcpy(file.clusters, clusters, qnt_cluster * sizeof(int));

        fflush(xdisco);
        return file;

    }else{

        //colocar o arquivo atual na lista de arquivos do diretorio

        //como a arquivo não existe ainda, devemos criar uma entrada para ele com um cluster inicial
        int posicaoEntrada = (512 * 32) + 1024 + (acharEntradaVazia() * 32);
        int clusterVazio = acharClusterVazio(-1);
        EntradaDiretorio entrada;
        memset(&entrada, 0, sizeof(EntradaDiretorio));
        strcpy(entrada.filename,filename);
        entrada.atributos = 0x20;
        entrada.startCluster = clusterVazio;
        entrada.fileSize = 0;
        fseek(xdisco, posicaoEntrada, SEEK_SET);
        fwrite(&entrada,sizeof(EntradaDiretorio),1,xdisco);

        //marcar o cluster do arquivo
        EntradaFAT entradaDaFat;
        memset(&entradaDaFat, 0, sizeof(EntradaFAT));
        entradaDaFat.ponteiro = 0xffffffff;
        int posicaoFatCluster = ( 512 * 32 ) + ( 4 * clusterVazio );
        fseek(xdisco, posicaoFatCluster, SEEK_SET);
        fwrite(&entradaDaFat,sizeof(EntradaFAT),1,xdisco);

        //retorar a struct com os metadados do novo arquivo
        XFILE file;
        memset(&file, 0, sizeof(XFILE));
        int clusters[] = {clusterVazio};
        strncpy((char *)file.filename, filename, 11);
        file.disco = xdisco;
        file.tamanhoArquivo = 0;
        file.qnt_cluster = 1;
        file.posicaoNoArquivo = 0;
        file.clusters = malloc(1 * sizeof(int));
        memcpy(file.clusters, clusters, 1 * sizeof(int));
        
        fflush(xdisco);
        return file;
    }
}


/*
    Esta chamada de sistema serve para definir a posicao onde o ponteiro de leitura e escrita esta no arquivo
    Ela recebe SEEK_SET, SEEK_END e SEEK_CUR como parametros tambem igual as outras chamadas
*/
void xseek(XFILE * file, int offset, int posicionador){
    
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
int xread( char * recebedor, int tamanhoTipo, int tamanhoLeitura, XFILE * file ){

    int tamanhoCluster = 512;
    int tamanhoTotalDaLeitura = tamanhoTipo * tamanhoLeitura;
    
    if( tamanhoTotalDaLeitura > tamanhoCluster ){
        printf("\n!! ATENÇÂO: Leitura inválida. \nA implementação atual permite que a leitura tenha o tamanho de no máximo 1 cluster.\nO tamanho do cluster neste disco é de %d bytes. Você esta tentando ler %d bytes!! \n",tamanhoCluster,tamanhoTotalDaLeitura);
        return 0;
    }
    
    else{
        
        if( (file->posicaoNoArquivo + tamanhoTotalDaLeitura) > file->tamanhoArquivo ){
            tamanhoTotalDaLeitura = file->tamanhoArquivo - file->posicaoNoArquivo;
        }
        
        //array que vai abrigar os clusters lidos do arquivo
        unsigned char *cluster = (unsigned char *)malloc( (tamanhoCluster * 2) * sizeof(unsigned char) );

        //pegar primeiro cluster
        int indexDoClusterNoClusterDoArquivo = file->posicaoNoArquivo / tamanhoCluster;
        int numeroClusterAtual = file->clusters[indexDoClusterNoClusterDoArquivo];
        unsigned char *clusterLido = clusterToArray(numeroClusterAtual,tamanhoCluster);
        int quantidadeDoCluster = 0;
        for( int i = 0 ; i < tamanhoCluster ; i++ ){
            cluster[quantidadeDoCluster] = clusterLido[i];
            quantidadeDoCluster++;
        }

        //caso tenha mais algum cluster depois desse, pegaremos ele tambem pois como a leitura esta limitada a 
        //um cluster, dessa forma a gente não precisa fazer um algoritmo muito complicado para resolver isso
        if(indexDoClusterNoClusterDoArquivo+1 < file->qnt_cluster){
            numeroClusterAtual = file->clusters[indexDoClusterNoClusterDoArquivo+1]; 
            unsigned char *clusterLido2 = clusterToArray(numeroClusterAtual,tamanhoCluster);
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

    return tamanhoTotalDaLeitura;
}

void xwrite(unsigned char * bytesParaEscrever, int tamanhoTipo, int quantidadeBytes, XFILE * file){

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
        EntradaDiretorio entradaDoArquivo = pegarEntradaDeDiretorio(file->filename);
        int tamanhoAtualDoArquivo = entradaDoArquivo.fileSize;
        if(tamanhoAtualDoArquivo < file->posicaoNoArquivo + tamanhoTotalDaEscrita){
            entradaDoArquivo.fileSize = file->posicaoNoArquivo + tamanhoTotalDaEscrita;
        }
        int posicaoEntrada =  pegarPosicaoEntradaDeDiretorio(file->filename);
        int posicaoParaNoDisco = (512 * 32) + 1024 + (posicaoEntrada*32);
        fseek(xdisco, posicaoParaNoDisco, SEEK_SET);
        fwrite(&entradaDoArquivo,sizeof(EntradaDiretorio),1,xdisco);
        fflush(xdisco);

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
            fseek(xdisco, posicaoClusterInicio, SEEK_SET);
            fwrite(bytesParaEscrever,1,tamanhoParte1,xdisco);
            file->posicaoNoArquivo += tamanhoParte1;

            //escrevendo a segunda parte no proximo cluster.
            //no caso de não ter proximo cluster ainda é necessário criar
            if( (indexDoClusterNoClusterDoArquivo+1) == file->qnt_cluster){
                //alocar mais um cluster

                //pegar proximo cluster a adicionalo ao file descriptor
                int clusterVazio = acharClusterVazio(-1);
                file->clusters[file->qnt_cluster] = clusterVazio;
                file->qnt_cluster++;

                //modificar ultimo cluster para apontar para o novo
                EntradaFAT entradaDaFat;
                memset(&entradaDaFat, 0, sizeof(EntradaFAT));
                entradaDaFat.ponteiro = clusterVazio;
                int posicaoFatCluster = ( 512 * 32 ) + ( 4 * file->clusters[indexDoClusterNoClusterDoArquivo] );
                fseek(xdisco, posicaoFatCluster, SEEK_SET);
                fwrite(&entradaDaFat,sizeof(EntradaFAT),1,xdisco);

                //colocar novo ultimo cluster
                gravarFat(clusterVazio,0xffffffff);

                //escrever segunda parte no ultimo cluster
                posicaoClusterInicio = ( 512 * 32 ) + 1024 + ( tamanhoCluster * clusterVazio );
                fseek(xdisco, posicaoClusterInicio, SEEK_SET);
                fwrite(array2,1,tamanhoParte2,xdisco);
                file->posicaoNoArquivo += tamanhoParte2;
                
            }else{

                //tem espaço no ultimo cluster ainda, só escrever nele
                numeroClusterAtual = file->clusters[indexDoClusterNoClusterDoArquivo+1];
                posicaoClusterInicio = ( 512 * 32 ) + 1024 + ( tamanhoCluster * numeroClusterAtual );
                fseek(xdisco, posicaoClusterInicio, SEEK_SET);
                fwrite(array2,1,tamanhoParte2,xdisco);
                file->posicaoNoArquivo += tamanhoParte2;
            }

        }else{

            //gravando no cluster. essa gravação ocupa so um cluster
            fseek(xdisco, posicaoClusterInicio, SEEK_SET);
            fwrite(bytesParaEscrever,1,tamanhoTotalDaEscrita,xdisco);
            file->posicaoNoArquivo += tamanhoTotalDaEscrita;
        }

        //mudar ponteiro
        fflush(xdisco);

    }

}


/* 
    acabar com todas as coisas relacionadas ao arquivo
*/
void xclose(XFILE * file){
    fflush(xdisco);
}


/*
    Remover arquivo
    Parametros:
        nomeArquivo: String contendo o nome do arquivo para ser apagado
*/
void xremove( char* nomeArquivo, char* nomeDisco ){

    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024;
    EntradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(EntradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int proximoCluster;
    int tamanhoArquivo;
    int quantidadeEntradas = 0;

    while( quantidadeEntradas <= 16 ){

        fseek(xdisco, inicioEntradas, SEEK_SET);
        EntradaDiretorio entradaTeste;
        fread(&entradaTeste, sizeof(EntradaDiretorio), 1, xdisco);

        // Diminuir o tamanho do nome do arquivo para 11
        char nome[11];
        strncpy(nome, entradaTeste.filename, 11);
        int comparacao = strcmp(nome,nomeArquivo);

        // comparando o nome do arquivo com o nome da entrada para ver se são iguais
        // OBS: strcmp retorna zero quando as strings são iguais. Diferente doque poderiamos pensar.
        if(comparacao == 0){
            proximoCluster = entradaTeste.startCluster;
            tamanhoArquivo = entradaTeste.fileSize;
            fseek(xdisco, inicioEntradas, SEEK_SET);
            fwrite(&entradaLimpa,sizeof(EntradaDiretorio),1,xdisco);
            break;
        }
        else{
            inicioEntradas += 32;
        }
        quantidadeEntradas++;
    }

    // limpar todas as entradas FAT referentes aos clusters desse arquivo
    EntradaFAT entradaLimpaFAT;
    memset(&entradaLimpaFAT, 0, sizeof(EntradaFAT));

    int qnt_cluster = tamanhoArquivo / 512;
    if(tamanhoArquivo % 512){
        qnt_cluster++;
    }
    for (int i = 0; i < qnt_cluster; i++){
        int posicaoEntradaCluster = ( 512 * 32 ) + ( 4 * proximoCluster );
        fseek(xdisco, posicaoEntradaCluster, SEEK_SET);
        EntradaFAT testeEntrada;
        fread(&testeEntrada, sizeof(EntradaFAT), 1, xdisco);
        fseek(xdisco, posicaoEntradaCluster, SEEK_SET);
        proximoCluster = testeEntrada.ponteiro;
        fwrite(&entradaLimpaFAT,sizeof(EntradaFAT),1,xdisco);
    }

}




/*
    mudar o diretorio de trabalho
    Esta função permite cinfigurar o diretorio de trabalho para usar caminho relativo
*/
void xchdir( char * path ){
    xpath = path;
    diretorioAtual = xopen(path);
}

/*
    criar diretorio
*/
void xmkdir( char * path ){

    int clusterUsado = acharClusterVazio(-1);
    gravarFat(clusterUsado,0xffffffff);

    //criar entrada para o novo diretorio no diretorio atual
    EntradaDiretorio entradaNovoDiretorio;
    memset(&entradaNovoDiretorio, 0, sizeof(EntradaDiretorio));
    strcpy(entradaNovoDiretorio.filename,path);
    entradaNovoDiretorio.atributos = 0x40;
    entradaNovoDiretorio.startCluster = clusterUsado;
    entradaNovoDiretorio.fileSize = 512;
    XFILE dir = xopen(".");
    xseek(&dir,32*acharEntradaVazia(),SEEK_SET);
    xwrite( (char *)&entradaNovoDiretorio, 1, sizeof(EntradaDiretorio), &dir );
    xclose(&dir);
    

    //criar entrada root do novo diretorio dentro dele mesmo
    XFILE novoDiretorio = xopen(path);
    xseek(&novoDiretorio,0,SEEK_SET);
    strcpy(entradaNovoDiretorio.filename,".");
    xwrite( (char *)&entradaNovoDiretorio, 1, sizeof(EntradaDiretorio), &novoDiretorio );


    //criar entrada para voltar
    EntradaDiretorio entradaVoltar;
    memset(&entradaVoltar, 0, sizeof(EntradaDiretorio));
    strcpy(entradaVoltar.filename,"..        ");
    entradaVoltar.atributos = 0x40;
    entradaVoltar.startCluster = diretorioAtual.clusters[0];
    entradaVoltar.fileSize = 512;
    xseek(&novoDiretorio,32*4,SEEK_SET);
    xwrite( (char *)&entradaVoltar, 1, sizeof(EntradaDiretorio), &novoDiretorio );

    xclose(&novoDiretorio);
}

/*
    remover diretorio
*/
void xrmdir( char * path ){

}


/*
    montar um disco
*/
void xmount( char * path ){
    if(xdisco == NULL){
        printf("Montando disco %s",path);
        xdisco = fopen(path,"r+w");
        diretorioAtual = xopen(".");
        xpath = "/";
    }else{
        printf("Já existe um disco montado\n");
    }
}


/*
    desmontar disco
*/
void xunmount( char * path ){
    if(xdisco != NULL){
        printf("Desmontando disco\n");
        fflush(xdisco);
        fclose(xdisco);
        xdisco = NULL;
    }else{
        printf("Desmontando disco\n");
    }
    
}