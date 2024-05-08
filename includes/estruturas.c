/*
    Esta estrutura é retornada pela função open e representa os metadados necessários para acessar o arquivo no disco
*/
struct XFILE {
    unsigned char filename[11];
    unsigned int tamanhoArquivo;
    unsigned int qnt_cluster;
    unsigned int posicaoNoArquivo;
    FILE* disco;
    int *clusters; // Um ponteiro para um array de inteiros
};
typedef struct XFILE XFILE;



/*
    A struct entradaDiretorio representa uma entrada de diretirio FAT.
    Cada entrada representa um arquivo e aponta para o cluster inicial do arquivo. 
*/
struct entradaDiretorio{
    unsigned char filename[11];     // 0-10    File name (8 bytes) with extension (3 bytes)
    unsigned char atributos;        /* 11      Attribute - a bitvector. 
                                               Bit 0: read only. 
                                               Bit 1: hidden.
                                               Bit 2: system file. 
                                               Bit 3: volume label. 
                                               Bit 4: subdirectory.
                                               Bit 5: archive.
                                               Bit 6-7:unused.*/
    unsigned char reservados[10];   // 12-21   Reserved (see below)
    unsigned short time;            // 22-23   Time (5/6/5 bits, for hour/minutes/doubleseconds)
    unsigned short date;            // 24-25   Date (7/4/5 bits, for year-since-1980/month/day)
    unsigned short startCluster;    // 26-27   Starting cluster (0 for an empty file)
    unsigned int fileSize;          // 28-31   Filesize in bytes
};
typedef struct entradaDiretorio EntradaDiretorio;

/*
    A struct entradaFAT serve para apontar os clusters na FAT32
*/
struct entradaFAT{
    unsigned int ponteiro;
};
typedef struct entradaFAT EntradaFAT;


/*
    A struct blocoParametrosBios representa a região Bios Paraters Block do disco.
    Ela contem todas as informações relacionadas a tamanho e espaço do disco.
    Ela é formada por três tabelas seguidas: DOS 2.0 BPB, DOS 3.31 BPB e DOS 7.1 EBPB 
*/
struct blocoParametrosBios{

                                            // [ DOS 2.0 BPB ]
                                            // Sector offset	BPB offset	Field length	Description
    unsigned short byterPorSetor;           // 0x00B	        0x00	    WORD	        Bytes per logical sector
    unsigned char setoresPorCluster;        // 0x00D	        0x02	    BYTE	        Logical sectors per cluster
    unsigned short setoresReservados;       // 0x00E	        0x03	    WORD	        Reserved logical sectors
    unsigned char numeroFats;               // 0x010	        0x05	    BYTE	        Number of FATs
    unsigned short entradasRoot;            // 0x011	        0x06	    WORD	        Root directory entries
    unsigned short quantidadeSetores16;       // 0x013	        0x08	    WORD	        Total logical sectors
    unsigned char descricaoMidia;           // 0x015	        0x0A	    BYTE	        Media descriptor
    unsigned short setoresPorFat;           // 0x016	        0x0B	    WORD	        Logical sectors per FAT


                                            // [ DOS 3.31 BPB ]
                                            // Sector offset	BPB offset	Field length	Description
    unsigned short setoresPorTrack;         // 0x018	        0x0D	    WORD	        Physical sectors per track (identical to DOS 3.0 BPB)
    unsigned short numeroDeCabecas;         // 0x01A	        0x0F	    WORD	        Number of heads (identical to DOS 3.0 BPB)
    unsigned int setoresEscondidos;         // 0x01C	        0x11	    DWORD	        Hidden sectors (incompatible with DOS 3.0 BPB)
    unsigned int quantidadeSetores32;       // 0x020	        0x15	    DWORD	        Large total logical sectors


                                            // [ DOS 7.1 EBPB ]
                                            // Sector offset	BPB offset	Field length	Description
    unsigned int setoresFat;            // 0x024	        0x19	    DWORD	        Logical sectors per FAT
    unsigned short bandeirasEspelhamento;   // 0x028	        0x1D	    WORD	        Mirroring flags etc.
    unsigned short versao;                  // 0x02A	        0x1F	    WORD	        Version
    unsigned int clusterRoot;               // 0x02C	        0x21	    DWORD	        Root directory cluster
    unsigned short setorDoFSInformation;    // 0x030	        0x25	    WORD	        Location of FS Information Sector
    unsigned short setorDoBackup;           // 0x032	        0x27	    WORD	        Location of backup sector(s)
    unsigned char reservado[12];            // 0x034	        0x29	    12 BYTEs	    Reserved (Boot file name)
    unsigned char numeroDriveFisico;        // 0x040	        0x35	    BYTE	        Physical drive number
    unsigned char reservados2;              // 0x041	        0x36	    BYTE	        Flags etc.
    unsigned char assinaturaExBoot;         // 0x042	        0x37	    BYTE	        Extended boot signature (0x29)
    unsigned int numeroSerialVolume;        // 0x043	        0x38	    DWORD	        Volume serial number
    unsigned char labelDoVolume[11];        // 0x047	        0x3C	    11 BYTEs	    Volume label
    unsigned char tipoSistemaDeArquivos[8]; // 0x052	        0x47	    8 BYTEs	        File-system type
    unsigned char codigoBoot[420];
    unsigned short assinaturaBoot;          // 0xAA55
};


/*
    A struct setorInformacoesFS representa o Bloco de informações referentes ao sistema de arquivos.
    Este bloco ocupa todo o segundo setor do disco logo depois do Boot sector.
*/
struct setorInformacoesFS{
    int leadSignature;                  // 0x41615252 Esta assinatura valida que isso é realmente um FSinformation Sector
    char reservados[480];               // Estes bytes devem estar sempre zerados
    int structSignature;                // 0x61417272 Esta assinatura valida que isso é realmente um FSinformation Sector
    int contadorClusterLivre;           // Indica o ultimo custer livre usado no disco. Se for 0xFFFFFFFF então não existe. Nem sempre esta corretor.
    int contadorProximoClusterLivre;    // Indica o proximo cluster livre.
    char reservados2[12];               // Este bytes devem estar sempre zerados.
    int assinaturaDaTrilha;             // 0xAA550000 Esta assinatura valida que isso é realmente um FSinformation Sector.
};