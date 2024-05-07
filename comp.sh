mkdir temps

# apagar arquivos antigos
echo "apagando arquivos antigos..."
rm -rf temps/*

# compilar programa
echo "compilando programa..."
gcc -o Teste main.c