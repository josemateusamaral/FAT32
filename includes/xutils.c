/*
    printar xFILE
*/
void printXFILE( XFILE file ){
    printf("\nnome: %s\n",file.filename);
    printf("tamanho: %d\n",file.tamanhoArquivo);
    printf("clusters: ");
    for( int i = 0 ; i < file.qnt_cluster ; i++ ){
        printf(" %d",file.clusters[i]);
    }
    printf("\nposicao: %d\n",file.posicaoNoArquivo);
}


/*
    Gravar fat
*/
void gravarFat( int numeroFat, int ponteiro){
    fseek(xdisco,(32*512)+(4 * numeroFat),SEEK_SET);
    fwrite(&ponteiro,1,sizeof(int),xdisco);
    fflush(xdisco);
}


/*
    formatar o nome de um arquivo para o formato do FAT32
*/

char * aplicarFormatacaoNome(char * nomeArquivo) {
    char nome[9]; // Espaço para 8 caracteres + caractere nulo
    char extensao[4]; // Tamanho máximo da extensão do arquivo (defina como necessário)

    int tamanho = strlen(nomeArquivo);
    int i = 0;
    int j = 0;

    // Copiar o nome do arquivo para 'nome'
    while (nomeArquivo[i] != '.' && i < 8) {
        nome[i] = nomeArquivo[i];
        i++;
    }
    if( i < 8 ){
        int caracteresFaltando = 8 - i;
        int posicaoNome = i;
        for( int caractereFaltando = 0 ; caractereFaltando < caracteresFaltando ; caractereFaltando++ ){
            nome[posicaoNome] = ' ';
            posicaoNome++;
        }
    }
    i++;

    for( ; j < 3 ; j++,i++ ){
        extensao[j] = nomeArquivo[i];
    }
    j++;

    // Extrair a extensão do arquivo
    char nomeFormatado[12]; 
    for( int caractere = 0; caractere < 8 ; caractere++ ){
        nomeFormatado[caractere] = nome[caractere];
    }
    for( int caractere = 0, posicaoFinal = 8; caractere < 3 ; caractere++, posicaoFinal++  ){
        nomeFormatado[posicaoFinal] = extensao[caractere];
        if(caractere == 2){
            posicaoFinal++;
            nomeFormatado[posicaoFinal] = '\0';
        }
    }
    
    char * retorno = (char *)(nomeFormatado);

    return retorno;
}

/* 
    remover a formatacao de nome arquivo de uma entradaDeDiretorio
*/
char * removerFormatacaoNome( char * nomeFormatado ){

    // array para conter todos os caracteres do nome apos a retirada da formatacao
    char nomeSemFormatacao[20];

    // pegar o nome do arquivo sem os espacoes e a extensao
    int i = 0;
    int j = 0;
    for( ; i < 8 ; i++ ){
        if( nomeFormatado[i] != ' '){
            nomeSemFormatacao[j] = nomeFormatado[i];
            j++;
        }
    }
    nomeSemFormatacao[j] = '.';
    j++;

    //pegar a extensao do arquivo
    for( ; i < 11 ; i++ ){
        if( nomeFormatado[i] != ' '){
            nomeSemFormatacao[j] = nomeFormatado[i];
            j++;
        }
    }
    nomeSemFormatacao[j] = '\0';
    
    //converter array de chars em uma string
    char * retorno = (char *)(nomeSemFormatacao);    
    return retorno;

}


/*
    Verificar se o arquivo já existe ou deve ser criado
*/
int arquivoExiste( char * nomeArquivo){
    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024;
    EntradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(EntradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int proximoCluster;
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
int acharClusterVazio( int descartar){

    int posicao = 1;
    int inicioFat = ( 512 * 32 ) + 4;

    while(1){

        if( posicao == descartar ){
            posicao++;
            continue;
        }

        fseek(xdisco,inicioFat, SEEK_SET);
        EntradaFAT entrada;
        fread(&entrada, sizeof(EntradaFAT), 1, xdisco);

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
int acharEntradaVazia( ){

    int inicioRoot = ( 512 * 32 ) + 1024 + (diretorioAtual.clusters[0]*512);
    int posicao = 0;

    while(1){

        fseek(xdisco,inicioRoot, SEEK_SET);
        EntradaDiretorio entrada;
        fread(&entrada, sizeof(EntradaDiretorio), 1, xdisco);

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
int pegarPosicaoEntradaDeDiretorio( char * nomeArquivo){
    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024 + (diretorioAtual.clusters[0]*512);
    EntradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(EntradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int proximoCluster;
    int quantidadeEntradas = 0;
    int posicao = 0;

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
EntradaDiretorio pegarEntradaDeDiretorio( char * nomeArquivo ){
    // acessar a tabela do diretorio root
    //int teste = diretorioAtual.clusters[0];
    //printf("pegando entrada...%d\nqtd clusters: %d",,diretorioAtual.qnt_cluster);
    int inicioEntradas = ( 32 * 512 ) + 1024;
    EntradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(EntradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int quantidadeEntradas = 0;

    while( quantidadeEntradas <= 16 ){

        fseek(xdisco, inicioEntradas, SEEK_SET);
        EntradaDiretorio entradaTeste;
        memset(&entradaTeste, 0, sizeof(EntradaDiretorio));
        fread(&entradaTeste, sizeof(EntradaDiretorio), 1, xdisco);

        // Diminuir o tamanho do nome do arquivo para 11
        char nome[11];
        memset(&nome, 0,11);
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

    return entradaLimpa;
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
int* fatToArray() {
    int* fat = malloc(128 * sizeof(unsigned int));
    int inicioFat = 512 * 32;
    fseek(xdisco, inicioFat, SEEK_SET);
    fread(&fat[0], sizeof(unsigned int), 128, xdisco);
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
void printarFat() {
    int* fat = malloc(128 * sizeof(unsigned int));
    int inicioFat = 512 * 32;
    fseek(xdisco, inicioFat, SEEK_SET);
    fread(&fat[0], sizeof(unsigned int), 128, xdisco);
    printf("\nSTART FAT\n");
    for( int i = 0 ; i < 128 ; i++ ){
        printf(" %d ",fat[i]);
    }
    printf("\nEND FAT\n");
}


/*
printar a struct de de chamada de sistemas open
*/
void printarOpen(XFILE file){
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
void printEntradaDiretorio(EntradaDiretorio file){

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
unsigned char* clusterToArray( int numeroCluster, int tamanhoCluster ) {
    unsigned char *cluster = (unsigned char *)malloc( tamanhoCluster * sizeof(unsigned char) );
    int posicaoCluster = ( 512 * 32 ) + 1024 + ( tamanhoCluster * numeroCluster);
    fseek(xdisco, posicaoCluster, SEEK_SET);
    fread(&cluster[0], sizeof(unsigned char), tamanhoCluster, xdisco);
    return cluster;
}




