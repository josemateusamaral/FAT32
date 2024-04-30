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

    int bytes_lidos;
    unsigned char *bufferLeituraEscrita = (unsigned char *)malloc( 512 * sizeof(unsigned char) );


    //testes de criacao de arquivo usando a chamada de sistema
    FILE * arquivoEntrada = fopen(nomeDoArquivo,"rb");
    struct XFILE fileNoFat = xopen(nomeGravacao,"FAT.img");
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

}


/*
    Esta função serve para pegar um arquivo de dentro do disco FAT32 e retorna-lo como um arquivo fora do disco FAT32
    Parametros:
        nomeArquivo: Uma string com o nome do arquivo que esta dentro do disco FAT32
        disco: Uma string com o nome de um disco FAT32
        nomeFinalArquivo: uma string com o nome que será usado para gravar o arquivo fora do disco FAT32.
*/
void copiarArquivo(char* nomeArquivo, char* nomeDiscoFAT, char* nomeFinalArquivo){

    //buffer usado para leitura e escrita e numero de bytes da leitura
    int bytes_lidos;
    unsigned char *bufferLeituraEscrita = (unsigned char *)malloc( 512 * sizeof(unsigned char) );

    //testar a copia de um arquivo do disco FAT32 para fora
    FILE * arquivoSaida = fopen(nomeFinalArquivo,"wb");
    struct XFILE fileDentro = xopen(nomeArquivo,nomeDiscoFAT);
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