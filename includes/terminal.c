/*
    Esta função serve para listar os arquivos do root
    Parametros:
        disco: uma imagem de um disco FAT32
*/
void terminal__ls( ){

    // acessar a tabela do diretorio root

    int inicioRoot = ( 32 * 512 ) + 1024 + (diretorioAtual.clusters[0]*512);
    fseek(xdisco, inicioRoot, SEEK_SET);

    printf("\nPATH: %s\n----------------------------------------\n",xpath);

    // pegar todas as entradas do root e printar seus nomes
    for( int i = 0 ; i < 16 ; i++ ){

        // colocar a entrada do diretorio em uma struct para analiza-la
        EntradaDiretorio entrada;
        fread(&entrada, sizeof(EntradaDiretorio), 1, xdisco);
        
        //printar ou não printar ? eis a questão.
        if( entrada.startCluster != 0 || entrada.atributos == 0x40 ){
            printf("%s\n", entrada.filename);
        }
    }  
}