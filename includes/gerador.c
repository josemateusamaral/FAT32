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
    Remover arquivo
    Parametros:
        nomeArquivo: String contendo o nome do arquivo para ser apagado
*/
void apagarArquivo( char* nomeArquivo, char* nomeDisco ){

    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024;
    FILE *disco = fopen(nomeDisco,"r+b");
    struct entradaDiretorio entradaLimpa;
    memset(&entradaLimpa, 0, sizeof(struct entradaDiretorio));

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int proximoCluster;
    int tamanhoArquivo;
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
            proximoCluster = entradaTeste.startCluster;
            tamanhoArquivo = entradaTeste.fileSize;
            fseek(disco, inicioEntradas, SEEK_SET);
            fwrite(&entradaLimpa,sizeof(struct entradaDiretorio),1,disco);
            break;
        }
        else{
            inicioEntradas += 32;
        }
        quantidadeEntradas++;
    }

    // limpar todas as entradas FAT referentes aos clusters desse arquivo
    struct entradaFAT entradaLimpaFAT;
    memset(&entradaLimpaFAT, 0, sizeof(struct entradaFAT));

    int qnt_cluster = tamanhoArquivo / 512;
    if(tamanhoArquivo % 512){
        qnt_cluster++;
    }
    for (int i = 0; i < qnt_cluster; i++){
        int posicaoEntradaCluster = ( 512 * 32 ) + ( 4 * proximoCluster );
        fseek(disco, posicaoEntradaCluster, SEEK_SET);
        struct entradaFAT testeEntrada;
        fread(&testeEntrada, sizeof(struct entradaFAT), 1, disco);
        fseek(disco, posicaoEntradaCluster, SEEK_SET);
        proximoCluster = testeEntrada.ponteiro;
        fwrite(&entradaLimpaFAT,sizeof(struct entradaFAT),1,disco);
    }

    fclose(disco);

}


/*
    Esta função recebe o caminho de um arquivo e grava ele em uma imagem de disco.
    Ela realiza primeiro a inserção do arquivo na pasta root, depois coloca os bytes do arquivo
    no cluster e depois configura as informações do cluster.
    Parametros:
        nomeDoArquivo: contem uma string que indica o arquivo a ser gravado no disco
        nomeDisco: contem uma string indicando o nome do disco para o qual a imagem sera gravada
*/
void gravarArquivo( char* nomeDoArquivo, char* nomeDisco, char* nomeGravacao ) {

    // abrir o disco e procurar um cluster vazio para iniciar a gravação do arquivo
    FILE *disco = fopen(nomeDisco,"r+b");
    int clusterVazio = acharClusterVazio(disco,-1);

    // pegar arquivo e colocar em uma lista de bytes
    FILE *file = fopen(nomeDoArquivo,"rb");
    fseek(file, 0, SEEK_END);
    int tamanhoArquivo = ftell(file);
    unsigned char *arquivo = (unsigned char *)malloc( tamanhoArquivo * sizeof(unsigned char) );
    fseek(file, 0, SEEK_SET);
    fread(&arquivo[0], sizeof(char), tamanhoArquivo, file);
    fclose(file);

    // criar entradaDiretorio para o arquivo
    struct entradaDiretorio entrada;
    strcpy(entrada.filename,nomeGravacao);
    entrada.atributos = 0x20;
    entrada.startCluster = clusterVazio;
    entrada.fileSize = tamanhoArquivo;

    // colocar a entradaDiretorio no diretorio root
    int entradaLivreNoRoot = acharEntradaVazia(disco);
    int inicioRoot = ( 32 * 512 ) + 1024 + ( 32 * entradaLivreNoRoot);
    fseek(disco, inicioRoot, SEEK_SET);
    fwrite(&entrada,sizeof(struct entradaDiretorio),1,disco);

    // colocar os bytes do arquivo no cluster
    int qnt_cluster = tamanhoArquivo / 512;
    if(tamanhoArquivo % 512){
        qnt_cluster++;
    }
    int j = 0;

    for (int i = 0; i < qnt_cluster; i++){
        
        // gravando no cluster
        int inicioCluster = ( 32 * 512 ) + 1024 + ( 512 * clusterVazio );
        fseek(disco, inicioCluster, SEEK_SET);
        fwrite(&arquivo[j],sizeof(unsigned char),512,disco);

        // quantidade de bytes sendo gravados por vez
        j += 512;

        // configurar a FAT do cluster
        int inicioFatCluster = ( 512 * 32 ) + ( 4 * clusterVazio );
        fseek(disco, inicioFatCluster, SEEK_SET);

        struct entradaFAT entradaDaFat;
        fread(&entradaDaFat, sizeof(struct entradaFAT), 1, disco);

        if (i == qnt_cluster-1){
            // ultima parte do arquivo
            entradaDaFat.ponteiro = 0xffffffff;
            fseek(disco, inicioFatCluster, SEEK_SET);
            fwrite(&entradaDaFat,sizeof(struct entradaFAT),1,disco);
        }
        else{     
            // parte normal. OBS: É  necessário passar o numero desse cluster para ser descartado da busca por cluster vazios.
            // Isso é necessário pois o cluster ainda não foi gravado, então aparecera como vazio na busca
            clusterVazio = acharClusterVazio(disco,clusterVazio);
            entradaDaFat.ponteiro = clusterVazio; 
            fseek(disco, inicioFatCluster, SEEK_SET);    
            fwrite(&entradaDaFat,sizeof(struct entradaFAT),1,disco);
        }
    }
    fclose(disco);

}


/*
    Esta função serve para pegar um arquivo de dentro do disco FAT32 e retorna-lo como um arquivo fora do disco FAT32
    Parametros:
        nomeArquivo: Uma string com o nome do arquivo que esta dentro do disco FAT32
        disco: Uma string com o nome de um disco FAT32
        nomeFinalArquivo: uma string com o nome que será usado para gravar o arquivo fora do disco FAT32.
*/
void lerArquivo(char* nomeArquivo, char* nomeDiscoFAT, char* nomeFinalArquivo){
    
    // acessar a tabela do diretorio root
    int inicioEntradas = ( 32 * 512 ) + 1024;
    FILE *disco = fopen(nomeDiscoFAT,"r+b");
    

    /* 
      criar struct de entrada apartir das entradas da root
      Para isso é necessário procurar o arquivo pelo nome entre as entrada da root 
    */
    int quantidadeEntradas = 0;
    int tamanhoArquivo;
    int posicaoCluster;
    while( quantidadeEntradas <= 16 ){

        fseek(disco, inicioEntradas, SEEK_SET);
        struct entradaDiretorio entradaTeste;
        fread(&entradaTeste, sizeof(struct entradaDiretorio), 1, disco);

        // Diminuir o tamanho do nome do aqruivo para 11
        char nome[11];
        strncpy(nome, entradaTeste.filename, 11);
        int comparacao = strcmp(nome,nomeArquivo);

        // comparando o nome do arquivo com o nome da entrada para ver se são iguais
        // OBS: strcmp retorna zero quando as strings são iguais. Diferente doque poderiamos pensar.
        if(comparacao == 0){
            tamanhoArquivo = entradaTeste.fileSize;
            posicaoCluster = entradaTeste.startCluster;
            break;
        }
        else{
            inicioEntradas += 32;
        }
        quantidadeEntradas++;
    }
    
    // calcular os parametros para pegar os clusters do arquivo
    int qnt_cluster = tamanhoArquivo / 512;
    if(tamanhoArquivo % 512){
        qnt_cluster++;
    }
    int inicioClusters = ( 512 * 32 ) + 1024;

    // abrir buffer do arquivo e ir jogando as partes dos clusters do arquivo e jogando dentro do arquivo
    FILE *arquivoSaida = fopen(nomeFinalArquivo,"wb");
    for (int i = 0; i < qnt_cluster; i++){
        
        // pegar os bytes do arquivo no cluster
        int inicioCluster =  inicioClusters + ( 512 * posicaoCluster );
        int quantidadeBytes;

        if (i == qnt_cluster -1){          
            quantidadeBytes = tamanhoArquivo % 512;
            unsigned char bufferBytesCluster[quantidadeBytes];
            fseek(disco, inicioCluster, SEEK_SET);
            fread(bufferBytesCluster, 1,quantidadeBytes,disco);

            //fseek(arquivoSaida,0, SEEK_END);
            fwrite(bufferBytesCluster, 1,quantidadeBytes, arquivoSaida);
        }
        else{
            quantidadeBytes = 512;
            unsigned char bufferBytesCluster[quantidadeBytes];
            fseek(disco, inicioCluster, SEEK_SET);
            fread(bufferBytesCluster,1,quantidadeBytes,disco);

            //fseek(arquivoSaida,0, SEEK_END);
            fwrite(bufferBytesCluster, 1,quantidadeBytes, arquivoSaida);
        }

        //printf("lendo %d bytes do cluster %d\n",quantidadeBytes,posicaoCluster);


        // verificar o proximo cluster na tabela FAT do arquivo
        int inicioFatCluster = ( 512 * 32 ) + ( 4 * posicaoCluster);
        struct entradaFAT proximoCluster;
        fseek(disco, inicioFatCluster, SEEK_SET);
        fread(&proximoCluster, sizeof(struct entradaFAT), 1, disco);
        posicaoCluster = proximoCluster.ponteiro;

    }

    // fechar os arquivos
    fclose(arquivoSaida);
    fclose(disco);

}


/*
    Esta função serve para gravar a imagem de um disco FAT32.
    Parametros:
        nomeDoArquivo: Nome do arquivo que sera gravado dentro do disco FAT32.
        tamanhoSetores: Representa a quantidade de Bytes que cada setor do disco tem. Normalmente 512bytes.
        quantidadeClusters: Representa a quantidade de cluster no disco FAT32. Cada cluster contem a mesma quantidade de setores.
        quantidadeSetoresPorCluster: Representa a quantidade de setores contidos dentro de um cluster como 1, 2, 4, 8, 16, 32, 64 ou 128 setores
*/
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

    // Boot Sector / Bios Parameter Block
    // OBS: Consultar a página 3 do arquivo docs/FAT Filesystem.pdf para melhores explicações.
    struct blocoParametrosBios BPB;
    memset(&BPB, 0, sizeof(struct blocoParametrosBios));
    BPB.byterPorSetor = tamanhoSetores;          
    BPB.setoresPorCluster = quantidadeSetoresPorCluster;        
    BPB.setoresReservados = 32;   
    BPB.numeroFats = 2;           
    BPB.entradasRoot = 0;         
    BPB.quantidadeSetores16 = 0;    
    BPB.descricaoMidia = 0xf0;
    BPB.setoresPorFat = 0; 
    BPB.setoresPorTrack = 0;        
    BPB.numeroDeCabecas = 0;         
    BPB.setoresEscondidos = 0;         
    BPB.quantidadeSetores32 = ( quantidadeClusters * quantidadeSetoresPorCluster ) + ( 512 * quantidadeSetoresReservados ) + (( quantidadeClusters * 32 ) * 2);
    BPB.setoresFat = ( ( quantidadeClusters * 32 ) * 2 );     
    BPB.bandeirasEspelhamento = 0;
    BPB.versao = 0;            
    BPB.clusterRoot = 1;             
    BPB.setorDoFSInformation = 2;    
    BPB.setorDoBackup = 0;           
    BPB.numeroDriveFisico = 0;       
    BPB.assinaturaExBoot = 0;        
    BPB.numeroSerialVolume = 0;     
    strcpy(BPB.labelDoVolume,"_testeFat32");       
    strcpy(BPB.tipoSistemaDeArquivos,"FAT32   ");
    BPB.assinaturaBoot = 0xaa55;     
    fwrite(&BPB,512,1,file);

    // File System Information Sector
    struct setorInformacoesFS informacoesFS;
    memset(&informacoesFS, 0, sizeof(struct setorInformacoesFS));
    informacoesFS.leadSignature = 0x41615252;         
    informacoesFS.structSignature = 0x61417272;            
    informacoesFS.contadorClusterLivre = 0;
    informacoesFS.contadorProximoClusterLivre = 0;
    informacoesFS.assinaturaDaTrilha = 0xaaff0000; 
    fwrite(&informacoesFS,sizeof(struct setorInformacoesFS),1,file);


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
        OBS: O primeiro cluster é ocupado pelo diretorio root nas duas tabelas
    */
    struct entradaFAT entradaRoot;
    memset(&entradaRoot, 0, sizeof(struct entradaFAT));
    entradaRoot.ponteiro = 0xffffffff;
    for( int j = 0 ; j < 2 ; j++ ){
        for( int i = 0 ; i < ( 512 / 4 ); i++ ){
            if( i == 0 ){
                fwrite(&entradaRoot,sizeof(struct entradaFAT),1,file);
            }else{
                struct entradaFAT entrada;
                memset(&entrada, 0, sizeof(struct entradaFAT));
                fwrite(&entrada,sizeof(struct entradaFAT),1,file);
            }
            
        }
    }    
    
    /*
        Criando tabela Root
    */
    for( int i = 1 ; i < 16 ; i++ ){
        struct entradaDiretorio entrada;
        memset(&entrada, 0, sizeof(struct entradaDiretorio));
        fwrite(&entrada,sizeof(struct entradaDiretorio),1,file);
    }


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