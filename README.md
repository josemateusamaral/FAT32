<br>
A versão atual consegue criar uma imagem de disco FAT32 e manipula-la podendo criar e ler arquivos de dentro da imagem.<br>
Ainda não foi implementado um terminal interativo para podermos manipular e visualizar a imagem.<br>
Para testar o programara é só rodar o comando <b>sh run.sh</b> que então o programa será compilado e rodado se tiver conseguido compilar corretamente. 
<br><br><br>

<b> Coisas a fazer </b> <br>

- [x] Escrever e criar arquivos na imagem <br>
- [ ] Verificar sobre little-endians <br>
- [ ] Programar um simulador de terminal para acessar e manipular a imagem <br>
- [x] Criar a parte do File System information ( Setor 2 ) <br>
- [x] Criar a pasta Root ( Cluster 1 ) <br>
- [x] Verificar se é necessário criar a parte do BPB ( Bios Parameters Block )
