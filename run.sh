# apagar arquivos antigos
echo "apagando arquivos antigos..."
rm Teste
rm FAT.img
rm arquivosTeste/novoTeste.txt
rm dump.txt
rm arquivosTeste/imagem.png

# compilar programa
echo "compilando programa..."
gcc -o Teste main.c

#rodar programa
echo "executando programa..."
./Teste
hexdump -v -C FAT.img > dump.txt