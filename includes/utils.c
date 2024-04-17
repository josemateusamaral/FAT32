
/*
    Verificar se o arquivo já existe ou deve ser criado
*/
int arquivoExiste( FILE* disco, char * nomeArquivo){
    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024;
    struct entradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(struct entradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int proximoCluster;
    int quantidadeEntradas = 0;

    while( quantidadeEntradas <= 16 ){

        fseek(disco, inicioEntradas, SEEK_SET);
        struct entradaDiretorio entradaTeste;
        fread(&entradaTeste, sizeof(struct entradaDiretorio), 1, disco);

        // Diminuir o tamanho do nome do arquivo para 11
        char nome[11];
        strncpy(nome, entradaTeste.filename, 11);
        int comparacao = strcmp(nome,nomeArquivo);

        // comparando o nome do arquivo com o nome da entrada para ver se são iguais
        // OBS: strcmp retorna zero quando as strings são iguais. Diferente doque poderiamos pensar.
        if(comparacao == 0){
            return 1;
        }
        else{
            inicioEntradas += 32;
        }
        quantidadeEntradas++;
    }
    return 0;
}

/*
    Esta função serve para retornar o numero de um cluster livre para podermos iniciar a gravação do arquivo
    OBS: É importante se notar que, os cluster validos começam a partir do terceiro cluster. Os dois primeiros não podem ser usados.
    Parametros:
        file: É o buffer da imagem do disco FAT32 onde os clusters estão.
*/
int acharClusterVazio( FILE* disco, int descartar){

    int posicao = 1;
    int inicioFat = ( 512 * 32 ) + 4;

    while(1){

        if( posicao == descartar ){
            posicao++;
            continue;
        }

        fseek(disco,inicioFat, SEEK_SET);
        struct entradaFAT entrada;
        fread(&entrada, sizeof(struct entradaFAT), 1, disco);

        if(!entrada.ponteiro){
            break;
        }
        else{
            inicioFat += 4;
            posicao++;
        }
    }
    return posicao;
}


/*
    Esta função serve para procurar uma entrada disponivel no diretorio root.
    No futuro ela recebe tambem um nome de diretório para que possamos procurar entradas dentro deste diretório.
    Parametros:
        disco: buffer de uma imagem de disco FAT32
*/
int acharEntradaVazia( FILE* disco ){
    int inicioRoot = ( 512 * 32 ) + 1024;
    int posicao = 0;

    while(1){

        fseek(disco,inicioRoot, SEEK_SET);
        struct entradaDiretorio entrada;
        fread(&entrada, sizeof(struct entradaDiretorio), 1, disco);

        if(entrada.atributos == 0){
            break;
        }
        else{
            inicioRoot += 32;
            posicao++;
        }
    }
    return posicao;
}


/*
    Pegar a posicao de uma entrada de diretorio
*/
int pegarPosicaoEntradaDeDiretorio( FILE* disco, char * nomeArquivo){
    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024;
    struct entradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(struct entradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int proximoCluster;
    int quantidadeEntradas = 0;
    int posicao = 0;

    while( quantidadeEntradas <= 16 ){

        fseek(disco, inicioEntradas, SEEK_SET);
        struct entradaDiretorio entradaTeste;
        fread(&entradaTeste, sizeof(struct entradaDiretorio), 1, disco);

        // Diminuir o tamanho do nome do arquivo para 11
        char nome[11];
        strncpy(nome, entradaTeste.filename, 11);
        int comparacao = strcmp(nome,nomeArquivo);

        // comparando o nome do arquivo com o nome da entrada para ver se são iguais
        // OBS: strcmp retorna zero quando as strings são iguais. Diferente doque poderiamos pensar.
        if(comparacao == 0){
            return posicao;
        }
        else{
            inicioEntradas += 32;
        }
        quantidadeEntradas++;
        posicao++;
    }
}


/*
    Pegar entrada de um arquivo
*/
struct entradaDiretorio pegarEntradaDeDiretorio( FILE* disco, char * nomeArquivo){
    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024;
    struct entradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(struct entradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int proximoCluster;
    int quantidadeEntradas = 0;

    while( quantidadeEntradas <= 16 ){

        fseek(disco, inicioEntradas, SEEK_SET);
        struct entradaDiretorio entradaTeste;
        fread(&entradaTeste, sizeof(struct entradaDiretorio), 1, disco);

        // Diminuir o tamanho do nome do arquivo para 11
        char nome[11];
        strncpy(nome, entradaTeste.filename, 11);
        int comparacao = strcmp(nome,nomeArquivo);

        // comparando o nome do arquivo com o nome da entrada para ver se são iguais
        // OBS: strcmp retorna zero quando as strings são iguais. Diferente doque poderiamos pensar.
        if(comparacao == 0){
            return entradaTeste;
        }
        else{
            inicioEntradas += 32;
        }
        quantidadeEntradas++;
    }
}


/*
    A Funcao a baixo retorna uma lista com 512bytes zerados.
    Isso eh usado sempre quano é necessário escrever um setor no disco.
    Em algumas ocasioes a lista retornada é alterada antes de ser gravada no disco.
    Parametros:
        tamanhoSetores: indica a quantidade de bytes contidos dentro do setor.
*/
unsigned char* genSetorVazio( int tamanhoSetores ) {
    unsigned char *setor = (unsigned char *)malloc( tamanhoSetores * sizeof(unsigned char) );

    for (int i = 0; i < tamanhoSetores ; i++) {
        setor[i] = 0x0;
    }

    return setor;
}


/*
    Colocar a tabela FAT dentro de um array para ser mais facil de analizar
*/
int* fatToArray(FILE * disco) {
    int* fat = malloc(128 * sizeof(unsigned int));
    int inicioFat = 512 * 32;
    fseek(disco, inicioFat, SEEK_SET);
    fread(&fat[0], sizeof(unsigned int), 128, disco);
    //printf("\nSTART FAT\n");
    //for( int i = 0 ; i < 128 ; i++ ){
    //    printf(" %d ",fat[i]);
    //}
    //printf("\nEND FAT\n");
    return fat;
}


/*
    Colocar a tabela FAT dentro de um array para ser mais facil de analizar
*/
void printarFat(char * nomeDisco) {
    FILE * disco = fopen(nomeDisco,"r");
    int* fat = malloc(128 * sizeof(unsigned int));
    int inicioFat = 512 * 32;
    fseek(disco, inicioFat, SEEK_SET);
    fread(&fat[0], sizeof(unsigned int), 128, disco);
    printf("\nSTART FAT\n");
    for( int i = 0 ; i < 128 ; i++ ){
        printf(" %d ",fat[i]);
    }
    printf("\nEND FAT\n");
    fclose(disco);
}


/*
printar a struct de de chamada de sistemas open
*/
void printarOpen(struct FAT32__fopen file){
    printf("nome arquivo: %s\n",file.filename);
    printf("tamanho arquivo: %d\n",file.tamanhoArquivo);
    printf("quantidade clusters: %d\n",file.qnt_cluster);
    printf("posicao no arquivo: %d\n",file.posicaoNoArquivo);
    printf("Clusters do arquivo: ");
    for( int i = 0 ; i < file.qnt_cluster ; i++ ){
        printf("%d > ",file.clusters[i]);
    }
    printf("\n");
}


/*
printar a struct de entradaDeDiretorio
*/
void printEntradaDiretorio(struct entradaDiretorio file){

    printf("nome arquivo: %s\n",file.filename);
    printf("atributos: %02X\n",file.atributos);
    printf("tempo: %d\n",file.time);
    printf("data: %d\n",file.date);
    printf("primeiro cluster: %d\n",file.startCluster);
    printf("tamanho: %d\n",file.fileSize);
    printf("\n");
}


/*
    Pegar um cluster e colocar em um array
*/
/*
    A Funcao a baixo retorna uma lista com 512bytes zerados.
    Isso eh usado sempre quano é necessário escrever um setor no disco.
    Em algumas ocasioes a lista retornada é alterada antes de ser gravada no disco.
    Parametros:
        tamanhoSetores: indica a quantidade de bytes contidos dentro do setor.
*/
unsigned char* clusterToArray( int numeroCluster, FILE * disco, int tamanhoCluster ) {
    unsigned char *cluster = (unsigned char *)malloc( tamanhoCluster * sizeof(unsigned char) );
    int posicaoCluster = ( 512 * 32 ) + 1024 + ( tamanhoCluster * numeroCluster);
    fseek(disco, posicaoCluster, SEEK_SET);
    fread(&cluster[0], sizeof(unsigned char), tamanhoCluster, disco);
    return cluster;
}


/*
    formatar nome de arquivo para entrada de diretorio
*/
char * formatarNomeDeArquivo( char * nomeArquivo ){

    /*
    int tamanhoNome = strlen(nomeArquivo);
    printf("tamanho da string: %ld\n",tamanhoNome);

    //pegar a extensao do arquivo
    char extensao[3];
    strncpy(extensao, nomeArquivo + (tamanhoNome-3), 3);
    printf("extensao: %s\n",extensao);

    //pegar nome do arquivo sem a extensao
    char nomeSemExtensao[8];
    strncpy(nomeSemExtensao, nomeArquivo, (tamanhoNome-4));
    printf("nome sem extensao: %s\n",nomeSemExtensao);

    //criar os espaços para colocar entre o nome e a extensao
    int tamanhoEspaco = 8 - strlen(nomeSemExtensao);
    printf("tamanho espaco: %d\n",tamanhoEspaco);
    char espaco[tamanhoEspaco];
    for( int i = 0 ; i < tamanhoEspaco ; i++ ){
        espaco[i] = " ";
    }


    //montar nome formatado
    
    sprintf(nomeFormatado, "%s%s%s", nomeSemExtensao, espaco,extensao);


    printf("nome formatado final: %s\n",nomeFormatado);
    */
    char *nomeFormatado = (char *)malloc(12 * sizeof(char));
    return nomeFormatado;
}

