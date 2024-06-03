@echo off

:: Criar diretório
mkdir temps

:: Apagar arquivos antigos
echo apagando arquivos antigos...
del /q temps\*

:: Compilar programa
echo compilando programa...
gcc -o Teste main.c

:: Rodar programa
echo executando programa...
Teste.exe