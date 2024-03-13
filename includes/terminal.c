/*
    Esta função serve para listar os arquivos do root
    Parametros:
        disco: uma imagem de um disco FAT32
*/
void terminarl__ls( char* nomeDisco ){

    // acessar a tabela do diretorio root
    int inicioRoot = ( 32 * 512 ) + 1024;
    FILE *disco = fopen(nomeDisco,"rb");
    fseek(disco, inicioRoot, SEEK_SET);

    // pegar todas as entradas do root e printar seus nomes
    for( int i = 0 ; i < 16 ; i++ ){

        // colocar a entrada do diretorio em uma struct para analiza-la
        struct entradaFAT entrada;
        fread(&entrada, sizeof(struct entradaFAT), 1, disco);
        
        //printar ou não printar ? eis a questão.
        if( entrada.fileSize != 0 ){
            printf("%s\t\t%d bytes\n", entrada.filename, entrada.fileSize);
        }
    }  
}