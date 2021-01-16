#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 1024

struct vicini {
    int portaA;
    int portaB;
};
struct vicini* neighbor = NULL;

struct dateZero {
    int data;
    char tipo[BUFFER_SIZE];
    int quanti;
    int final;
    struct dateZero *next;
};
struct dateZero* nuove = NULL;

struct sockaddr_in server_addr, pp_addr, my_addr;
int addrlen = sizeof(my_addr);
int addrlenPP = sizeof(pp_addr);
int ssd;        //discovery server socket descriptor
int porta;      //door; Valore passato all'avvio di ./peer
char portaChar[5];
int len;
int ret;
char bufferMsg[BUFFER_SIZE];
int reply = 0;                          //usata nella REPLY_DATA per verificare se ho ricevuto risposta da entrambi i vicini(== 0 si)
char serverMsg[BUFFER_SIZE];

int start(char* ip, char* door) {
    fd_set masterStart;      //set principale gestito dal programmatore
    FD_ZERO(&masterStart);
    FD_SET(0, &masterStart);
    int fdmaxStart;
    int o;     

    int doorServer = atoi(door);
    //INIZIALIZZAZIONE SOCKET UDP
    ssd = socket(AF_INET, SOCK_DGRAM, 0);
    //INDIRIZZO DEL SERVER
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(doorServer);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    FD_SET(ssd, &masterStart);
    fdmaxStart = ssd;

    //INDIRIZZO VARIABILE DEGLI ALTRI PEER
    memset(&pp_addr, 0, sizeof(pp_addr));
    pp_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &pp_addr.sin_addr);

    //INDIRIZZO DI QUESTO PEER
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(porta);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(ssd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    if(ret < 0) {
        perror("Errore in fase di binding: \n");
        exit(-1);
    }

    //INVIO MESSAGGIO START
    strncpy(bufferMsg, "START", BUFFER_SIZE);
    len = sendto(ssd, bufferMsg, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    while(1) {
        struct timeval tout;
        tout.tv_sec = 2;
        tout.tv_usec = 0;
        FD_ZERO(&masterStart);
        FD_SET(ssd, &masterStart);
        int sel = select(fdmaxStart + 1, &masterStart, NULL, NULL, &tout);
        if(sel <= 0) {
            printf("Il server non ha risposto: provo a riconnettermi\n");
            len = sendto(ssd, bufferMsg, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        }
        else {
            break;
        } 
    }
    ret = recvfrom(ssd, bufferMsg, BUFFER_SIZE, 0, (struct sockaddr*)&my_addr, &addrlen);
    FD_ZERO(&masterStart);
    neighbor = (struct vicini*) malloc(sizeof(struct vicini));
    if (neighbor == NULL) {
        printf("Failed to insert element. Out of memory\n");
        return;
    }
    sscanf(bufferMsg, "%s %d %d", serverMsg, &neighbor->portaA, &neighbor->portaB);
    printf("I miei vicini sono: %d - %d\n", neighbor->portaA, neighbor->portaB);
    if((neighbor->portaA == 0 && neighbor->portaB != 0) || (neighbor->portaA != 0 && neighbor->portaB == 0))
        reply = 1;
    else
        reply = 2;    
}

void ordinaFile(char *entry) {
    char ponte[BUFFER_SIZE];
    int checkData;
    int dataEntry;
    int inserito = 0;       //1: DATA NUOVA GIÀ INSERITA
    char filePrinc[BUFFER_SIZE];
    char fileTmp[BUFFER_SIZE];

    strncpy(fileTmp, portaChar, BUFFER_SIZE);
    strncpy(filePrinc, portaChar, BUFFER_SIZE);
    strncat(filePrinc, ".txt", BUFFER_SIZE);
    strncat(fileTmp, "_tmp.txt", BUFFER_SIZE);

    FILE* fdPrinc;
    FILE* fdTmp;
    fdPrinc = fopen(filePrinc, "r");
    fdTmp = fopen(fileTmp, "w");

    sscanf(entry, "%d", &dataEntry);

    if(fdPrinc != NULL) {
        while(fgets(ponte, sizeof(ponte), fdPrinc)) {
            sscanf(ponte, "%d", &checkData);
            if(checkData > dataEntry && inserito == 0) {
                fprintf(fdTmp, "%s", entry);
                inserito = 1;
            }
            fprintf(fdTmp, "%s", ponte);
        }
    }
    if(inserito == 0) {
        fprintf(fdTmp, "%s", entry);
    }
    fclose(fdPrinc);
    fclose(fdTmp);
    remove(filePrinc);
    rename(fileTmp, filePrinc);
}

//dataDue è la data più recente, dataUno quella più remota
//se nel file incontro il numero 1 allora vuol dire che non ho tutte le entry
//return -1 se non ho tutte le entry, -2 se non ho il file o nessuna entry, 0 altrimenti
int controllaDate(int dataUno, int dataDue, char *type) {
    int valoreTot = 0;
    int valoreTmp = 0;
    int checkData;
    int trovato = 0;    //mi serve per capire se ho almeno una entry nel periodo
    int completo = 0;
    char filePrinc[BUFFER_SIZE];
    char ponte[BUFFER_SIZE];
    char tipo[BUFFER_SIZE];

    strncpy(filePrinc, portaChar, BUFFER_SIZE);
    strncat(filePrinc, ".txt", BUFFER_SIZE);

    FILE* fdPrinc;
    fdPrinc = fopen(filePrinc, "r");

    if(fdPrinc != NULL) {
        while(fgets(ponte, sizeof(ponte), fdPrinc)) {
            sscanf(ponte, "%d %s %d %d", &checkData, tipo, &valoreTmp, &completo);
            if(checkData > dataDue) {
                break;
            }
            else if(checkData < dataUno) {
                continue;
            }
            if(strcmp(tipo, type) != 0) {
                continue;
            }
            if(completo == 1 && trovato > 0) {
                return -1;
            }
            else if(completo == 1 && trovato <= 0){
                return -2;
            }    
            ++trovato;
        }
        fclose(fdPrinc);
    }
    else
        return -2;
    if(dataDue > checkData) 
        return -1;

    if(trovato == 0)
        return -2;

    return 0;
}

void aggiungiLista(int dataRicevuta, char* tipoRicevuto, int quantitaRicevuta, int finale) {
    struct dateZero* elem = NULL;
    elem = (struct dateZero*) malloc(sizeof(struct dateZero));
    if (elem == NULL) {
        printf("Failed to insert element. Out of memory\n");
        return;
    }
    elem->data = dataRicevuta;
    strncpy(elem->tipo, tipoRicevuto, BUFFER_SIZE);
    elem->quanti = quantitaRicevuta;
    elem->final = finale;
    elem->next = NULL;

    //INSERIMENTO IN TESTA
    if(nuove == NULL) {
        nuove = elem;
        return;
    }
    //INSERIMENTO IN ORDINE
    else {
        struct dateZero* checker = nuove;
        while(checker != NULL) {
            if(checker->data == dataRicevuta) {
                if(strcmp(checker->tipo, tipoRicevuto) == 0) {
                    if(checker->final == 0) {
                        return;
                    }
                    else if(finale == 0){
                        checker->quanti = quantitaRicevuta;
                        return;
                    }
                    else{
                        checker->quanti += quantitaRicevuta;
                        return;
                    }
                    
                }
            }
            else if(checker->next == NULL) {
                checker->next = elem;
                return;
            }
            else if(checker->next->data > dataRicevuta) {
                elem->next = checker->next;
                checker->next = elem;
                return;
            }
            else {
                checker = checker->next;
            }          
        }
    }
    return;
}
void controllaLista() {
    struct dateZero *checker = nuove;
    while(checker != NULL) {
        printf("%d %s %d %d\n", checker->data, checker->tipo, checker->quanti, checker->final);
        checker = checker->next;
    }
}
//TRE CASI: 1) FILE NON ESISTENTE, 2) DATA TROVATA, 3) DATA NON PRESENTE NELLA LISTA     
void aggiornaFile() {
    char ponte[BUFFER_SIZE];
    int checkData;
    int dataEntry;
    int valoreTmp;
    int completo;
    int inserito = 0;
    char filePrinc[BUFFER_SIZE];
    char fileTmp[BUFFER_SIZE];
    char tipo[BUFFER_SIZE];

    strncpy(fileTmp, portaChar, BUFFER_SIZE);
    strncpy(filePrinc, portaChar, BUFFER_SIZE);
    strncat(filePrinc, ".txt", BUFFER_SIZE);
    strncat(fileTmp, "_tmp.txt", BUFFER_SIZE);

    FILE* fdPrinc;
    FILE* fdTmp;
    fdPrinc = fopen(filePrinc, "r");
    fdTmp = fopen(fileTmp, "w");

    struct dateZero* checker = nuove;

    //FILE PRESENTE
    if(fdPrinc != NULL) {
        while(fgets(ponte, sizeof(ponte), fdPrinc)) {
            sscanf(ponte, "%d %s %d %d", &checkData, tipo, &valoreTmp, &completo);
            if(checker != NULL) {
                //DATA TROVATA
                if(checkData == checker->data) {
                    if(strcmp(tipo, checker->tipo) == 0) {
                        if(checker->final == 0) {
                            fprintf(fdTmp, "%d %s %d %d\n", checker->data, checker->tipo, checker->quanti, 0);
                        }
                        else if(completo == 0) {
                            fprintf(fdTmp, "%d %s %d %d\n", checkData, tipo, valoreTmp, completo);
                        }
                        else {
                            valoreTmp += checker->quanti;
                            fprintf(fdTmp, "%d %s %d %d\n", checker->data, checker->tipo, valoreTmp, 0);
                        }
                    }
                    checker = checker->next;
                    continue;
                }
                else if(checkData < checker->data) {
                    fprintf(fdTmp, "%d %s %d %d\n", checkData, tipo, valoreTmp, completo);
                    continue;
                }
                else if(checkData > checker->data) {
                    fprintf(fdTmp, "%d %s %d %d\n", checker->data, checker->tipo, checker->quanti, 0);
                    checker = checker->next;
                }
            }
            else {
                fprintf(fdTmp, "%d %s %d %d\n", checkData, tipo, valoreTmp, completo);
            }
        }
        while(checker != NULL) {
            fprintf(fdTmp, "%d %s %d %d\n", checker->data, checker->tipo, checker->quanti, 0);
            checker = checker->next;
        }
        fclose(fdPrinc);
        remove(filePrinc);
    }
    //FILE NON ESISTENTE
    else{
        while(checker != NULL) {
            fprintf(fdTmp, "%d %s %d %d\n", checker->data, checker->tipo, checker->quanti, 0);
            checker = checker->next;
        }
    }
    fclose(fdTmp);
    rename(fileTmp, filePrinc);
}

int controllaEntry(int dataUno, int dataDue) {
    char ponte[BUFFER_SIZE];
    int checkData;
    int dataEntry;
    int valoreTmp;
    char filePrinc[BUFFER_SIZE];
    char tipo[BUFFER_SIZE];

    strncpy(filePrinc, portaChar, BUFFER_SIZE);
    strncat(filePrinc, ".txt", BUFFER_SIZE);

    FILE* fdPrinc;
    fdPrinc = fopen(filePrinc, "r");

    if(fdPrinc != NULL) {
        while(fgets(ponte, sizeof(ponte), fdPrinc)) {
            sscanf(ponte, "%d %s %d", &checkData, tipo, &valoreTmp);
            if(checkData < dataUno)
                continue;
            else if(checkData > dataDue)
                break;
            else
                return 1;
        }
    }
    return 0;
}

int calcolaTotale(int dataUno, int dataDue, char* tipo) {
    char ponte[BUFFER_SIZE];
    int checkData;
    int dataEntry;
    int valoreTmp;
    int valoreTot = 0;
    char filePrinc[BUFFER_SIZE];
    char tipoTmp[BUFFER_SIZE];

    strncpy(filePrinc, portaChar, BUFFER_SIZE);
    strncat(filePrinc, ".txt", BUFFER_SIZE);

    FILE* fdPrinc;
    fdPrinc = fopen(filePrinc, "r");

    if(fdPrinc != NULL) {
        while(fgets(ponte, sizeof(ponte), fdPrinc)) {
            sscanf(ponte, "%d %s %d", &checkData, tipoTmp, &valoreTmp);
            if(checkData < dataUno)
                continue;
            if(checkData > dataDue)
                break;
            if(strcmp(tipo, tipoTmp) == 0)
                valoreTot += valoreTmp;
        }
    }
    else {
        return -1;
    }
    fclose(fdPrinc);
    FILE* calcolo;
    char nomeFile[BUFFER_SIZE];
    sprintf(nomeFile, "%s_%s_%d:%d_%s.txt", portaChar, "totale", dataUno, dataDue, tipo);
    calcolo = fopen(nomeFile, "w");
    fprintf(calcolo, "%d", valoreTot);
    fclose(calcolo);
    return valoreTot;
} 

int calcolaVariazione(int dataUno, int dataDue, char* tipo) {
    char ponte[BUFFER_SIZE];
    int checkData;
    int dataEntry;
    int valoreTmp;
    int valoreGG = -1;
    int valoreVaria;
    char filePrinc[BUFFER_SIZE];
    char tipoTmp[BUFFER_SIZE];

    strncpy(filePrinc, portaChar, BUFFER_SIZE);
    strncat(filePrinc, ".txt", BUFFER_SIZE);

    FILE* fdPrinc;
    fdPrinc = fopen(filePrinc, "r");

    FILE* calcolo;
    char nomeFile[BUFFER_SIZE];
    sprintf(nomeFile, "%s_%s_%d:%d_%s.txt", portaChar, "variazione", dataUno, dataDue, tipo);
    calcolo = fopen(nomeFile, "w");

    if(fdPrinc != NULL) {
        while(fgets(ponte, sizeof(ponte), fdPrinc)) {
            sscanf(ponte, "%d %s %d", &checkData, tipoTmp, &valoreTmp);
            if(checkData < dataUno)
                continue;
            if(checkData > dataDue)
                break;
            if(strcmp(tipo, tipoTmp) == 0) {
                if(valoreGG == -1) {
                    valoreGG = valoreTmp;
                    continue;
                }
                else{
                    valoreVaria = valoreTmp - valoreGG;
                    fprintf(calcolo, "%d\n", valoreVaria);
                    printf("%d\n", valoreVaria);
                    valoreGG = valoreTmp;
                }
            }
        }
    }
    else {
        return -1;
    }
    fclose(calcolo);
    fclose(fdPrinc);
    return 1;
} 

void salvaSuFile(int quanN, int quanT, int anno, int mese, int giorno) {
    char filePrinc[BUFFER_SIZE];
    strncpy(filePrinc, portaChar, BUFFER_SIZE);
    strncat(filePrinc, ".txt", BUFFER_SIZE);
    FILE* fd;
    fd = fopen (filePrinc, "a");

    fprintf(fd, "%d%02d%02d %s %d %d\n", anno, mese, giorno, "N", quanN, 1);
    fprintf(fd, "%d%02d%02d %s %d %d\n", anno, mese, giorno, "T", quanT, 1);

    fclose(fd);    
}

int controllaGiorno() {
    time_t t = time(NULL);
    struct tm *current = localtime(&t);

    int ora = current->tm_hour;
    if(ora >= 18)
        return 1;
    else
        return 0;
    
}

void aggiornaGiorno(int *anno, int* mese, int* giorno) {
    if(*mese == 2 && *giorno == 29 && *anno%4 == 0) {
        *mese += 1;
        *giorno = 1;
    }
    else if(*mese == 2 && *giorno == 2 && *anno%4 != 0) {
        *mese += 1;
        *giorno = 1;
    }
    else if((*mese == 1 || *mese == 3 || *mese == 5 || *mese == 7 || *mese == 8 || *mese == 10) && *giorno == 31) {
        *mese += 1;
        *giorno = 1;
    }
    else if((*mese == 4 || *mese == 6 || *mese == 9 || *mese == 11) && *giorno == 30) {
        *mese += 1;
        *giorno = 1;
    }
    else if(*mese == 12 && *giorno == 31) {
        *anno += 1;
        *mese = 1;
        *giorno = 1;
    }
    else {
        *giorno += 1;
    }
}

int main(int argc, char* argv[]) {
    char bufferCom[BUFFER_SIZE];            //salvo la stringa di comando inserita dall'utente
    char comando[BUFFER_SIZE];              //:start, get, add, exit
    char socketMsg[BUFFER_SIZE];            //:NEIGHBORS, REPLY_DATA, REQ_DATA...
    char indIp[BUFFER_SIZE];                //usata nel comando start, salvo il valore dell'indirizzo ip del server
    char portaServer[BUFFER_SIZE];          //usata nel comando start, salvo il valore della porta del server  
    char type[BUFFER_SIZE];                 //usata nel comando get e nel comando add, salvo il type
    char aggr[BUFFER_SIZE];                 //usata nel comando get, salvo l'aggr
    char period[BUFFER_SIZE];               //usata nel comando get, salvo il period
    char periodCopyOne[BUFFER_SIZE];        //usata come copia per strtok
    char periodCopyTwo[BUFFER_SIZE];        //usata come copia per strtok
    char periodTmp[BUFFER_SIZE];            //usata per cercare il file che vale door_aggr_period_type.txt
    char fileTmp[BUFFER_SIZE];              //usata per salvare il file da chiedere in get nel formato _aggr_period_type.txt
    char messaggioPeer[BUFFER_SIZE];        //usata come buffer per scambiare messaggi con gli altri peer

    char anteFile[BUFFER_SIZE];             //usata da REQ_DATA, salva il nome del file incompleto ( = fileTmp)
    char fileReq[BUFFER_SIZE];              //usata da REQ_DATA, salva il nome del file richiesto (= door_fileTmp)
    char scorri[BUFFER_SIZE];               //usata da REQ_DATA, scorro i valori salvati nel file
    
    char valoriRicevuti[BUFFER_SIZE];       //usata da REPLY_DATA, salvo i valori ricevuti
    char *valoriFile[BUFFER_SIZE];          //usata da REPLY_DATA, scorro i valori ricevuti

    int quantity = 0;                       //usata nel comando add, salvo il valore della quantity 
    int quantityN = 0;                      //salvo in memoria il numero dei casi prima che il register venga chiuso
    int quantityT = 0;                      //salvo in memoria il numero dei tamponi prima che il register venga chiuso
    int value;                              //valore modificato da chi richiede
    int valueReq;                           //valore modificato da chi ha ricevuto la richiesta
    int dataUno;                            //considero la data più remota con il formato giapponese yyyymmdd
    int dataDue;                            //considero la data più recente con il formato giapponese yyyymmdd

    int dataUnoRic;                         //usata nella REQ_ENTRIES
    int dataDueRic;                         //usata nella REQ_ENTRIES

    int portaInoltro;                       //usata nella FLOOD_FOR_ENTRIES per controllare a chi inoltrare la richiesta
    int portaReq;                           //usata nella FLOOD_FOR_ENTRIES per salvare la porta del richiedente

    int portaPrinc;                         //usata per memorizzare a chi ho inviato la prima FLOOD_FOR_ENTRIES e quindi per capire chi ha eseguito la get
    int sonoIo = 0;                         //usata nella FLOOD_FOR_ENTRIES per verificare se sono io ad aver iniziato il flooding

    char confrontoData[BUFFER_SIZE];
    time_t t = time(NULL);
    struct tm *current = localtime(&t);
    int anno = current->tm_year + 1900;
    int mese = current->tm_mon + 1;
    int giorno = current->tm_mday;
    int dopo = 0;                           //usata per non effettuare due volte il cambio data

    if(argc > 1) {
        porta = atoi(argv[1]);
        strncpy(portaChar, argv[1], 4);
    }
    else {
        printf("Error: Could not boot without door. Use './peer <door>' to boot\n");
        exit(-1);
    }

    fd_set master;      //set principale gestito dal programmatore
    fd_set read_fds;    //set gestito dalla select
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(0, &master);
    int fdmax;
    int j;              //indice per scorrere i socket nella select

    printf("\n***************PEER COVID***************\n");
    printf("Digita un comando: \n");
    printf("1) start DS_addr DS_port\n");
    printf("\tPermette la connessione con il Server: DS_addr = indirizzo IP del Server, DS_port = porta del Server\n\n");
    printf("2) add type quantity\n");
    printf("\tAggiunge un nuovo evento: type = N (nuovo caso)/ T(tampone), quantity = numero dei casi o tamponi\n\n");
    printf("3) get aggr type period\n");
    printf("\tEffettua una richiesta di elaborazione: aggr = totale / variazione, type = N / T,\n");
    printf("\tperiod = dd1:mm1:yyyy1-dd2:mm2:yyyy2 (1 è la data più lontana e 2 la più vicina). Una data\n");
    printf("\tpuò essere sostituita con *, in quel caso non si ha limite inferiore o superiore. E' anche\n");
    printf("\tpossibile omettere il periodo. Viene eseguito il calcolo su tutti i dati\n\n");
    printf("4) stop\n");
    printf("\tDisconnessione dal server\n\n");
    
    strncpy(type, "", BUFFER_SIZE);
    strncpy(aggr, "", BUFFER_SIZE);
    strncpy(period, "", BUFFER_SIZE);
    strncpy(periodTmp, "", BUFFER_SIZE);

    while(1) {
        strncpy(bufferCom, "", BUFFER_SIZE);
        strncpy(comando, "", BUFFER_SIZE);
        strncpy(socketMsg, "", BUFFER_SIZE);
        strncpy(indIp, "", BUFFER_SIZE);
        strncpy(portaServer, "", BUFFER_SIZE);
        strncpy(periodCopyOne, "", BUFFER_SIZE);
        strncpy(periodCopyTwo, "", BUFFER_SIZE);
        strncpy(messaggioPeer, "", BUFFER_SIZE);
        strncpy(fileReq, "", BUFFER_SIZE);
        strncpy(anteFile, "", BUFFER_SIZE);
        strncpy(scorri, "", BUFFER_SIZE);
        strncpy(valoriRicevuti, "", BUFFER_SIZE);
        strncpy(messaggioPeer, "", BUFFER_SIZE);
        strncpy(bufferMsg, "", BUFFER_SIZE);
        strncpy(confrontoData, "", BUFFER_SIZE);
        quantity = 0;
        portaPrinc = 0;
        dataUnoRic = 0;
        dataDueRic = 0;
        value = 0;
        valueReq = 0;

        fflush(stdin);
        fflush(stdout);

        read_fds = master;

        select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        for(j = 0; j <= fdmax; ++j) {
            if(FD_ISSET(j, &read_fds)) {
                if(j == 0) {
                    fgets(bufferCom, BUFFER_SIZE, stdin);
                    sscanf(bufferCom, "%s", comando);

                    //COMANDO START
                    if(strcmp(comando, "start") == 0) {
                        if(controllaGiorno() == 1 && dopo == 0) {
                            dopo = 1;
                            aggiornaGiorno(&anno, &mese, &giorno);
                            salvaSuFile(quantityN, quantityT, anno, mese, giorno);
                            quantityN = 0;
                            quantityT = 0;
                        }
                        sscanf(bufferCom, "%s %s %s", comando, indIp, portaServer);
                        if(strcmp(portaServer, "") == 0) {
                            printf("Comando non valido\n");
                            continue;
                        }
                        printf("Connessione con il server in corso...\n");
                        start(indIp, portaServer);
                        FD_SET(ssd, &master);
                        fdmax = ssd;
                        printf("Connessione riuscita!\n");
                    }

                    //COMANDO STOP
                    else if(strcmp(comando, "stop") == 0) {
                        if(controllaGiorno() == 1 && dopo == 0) {
                            dopo = 1;
                            aggiornaGiorno(&anno, &mese, &giorno);
                            salvaSuFile(quantityN, quantityT, anno, mese, giorno);
                            quantityN = 0;
                            quantityT = 0;
                        }
                        strncpy(bufferMsg, "STOP", BUFFER_SIZE);
                        len = sendto(ssd, bufferMsg, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                        salvaSuFile(quantityN, quantityT, anno, mese, giorno);
                        quantityN = 0;
                        quantityT = 0;
                        exit(0);
                    }

                    //COMANDO GET
                    else if(strcmp(comando, "get") == 0) {
                        if(controllaGiorno() == 1 && dopo == 0) {
                            dopo = 1;
                            aggiornaGiorno(&anno, &mese, &giorno);
                            salvaSuFile(quantityN, quantityT, anno, mese, giorno);
                            quantityN = 0;
                            quantityT = 0;
                        }
                        sscanf(bufferCom, "%s %s %s %s", comando, aggr, type, period);
                        if(strcmp(aggr, "totale") == 0 || strcmp(aggr, "variazione") == 0) {
                            if(strcmp(type, "N") == 0 || strcmp(type, "T") == 0) {
                                
                                sprintf(confrontoData, "%d%02d%02d", anno, mese, giorno);
                                int tot;
                                sscanf(confrontoData, "%d", &tot);
                                strncpy(periodTmp, period, BUFFER_SIZE);
                                strncpy(periodCopyOne, period, BUFFER_SIZE);
                                strncpy(periodCopyTwo, period, BUFFER_SIZE);
                                char * dateSeparate[6];
                                int i = 0;
                                char * split = strtok(periodCopyOne, "-");
                                char * split4 = strtok(split, ":");
                                while(split4 != NULL) {
                                    dateSeparate[i++] = split4;
                                    split4 = strtok(NULL, ":");
                                }
                                char * split2 = strtok(periodCopyTwo, "-");
                                split2 = strtok(NULL, "-");
                                char * split3 = strtok(split2, ":");
                                while(split3 != NULL) {
                                    dateSeparate[i++] = split3;
                                    split3 = strtok(NULL, ":");
                                }
                                i = 0;
                                dataUno = 0;
                                dataDue = 0;
                                value = -1;
                                char tmpData[BUFFER_SIZE];
                                if(strcmp(period, "") != 0) {
                                    if(strcmp(dateSeparate[0], "*") != 0 && strcmp(dateSeparate[3], "*") != 0) {
                                        printf("Ho limite inferiore e superiore\n");
                                        sprintf(tmpData, "%s%s%s", dateSeparate[0], dateSeparate[1], dateSeparate[2]);
                                        sscanf(tmpData, "%d", &dataUno);
                                        sprintf(tmpData, "%s%s%s", dateSeparate[3], dateSeparate[4], dateSeparate[5]);
                                        sscanf(tmpData, "%d", &dataDue);

                                        if(dataUno > tot || dataDue > tot) {
                                            printf("ERRORE: periodo non valido\n");
                                            continue;
                                        }
                                    }
                                    else if(strcmp(dateSeparate[0], "*") == 0 && strcmp(dateSeparate[1], "") != 0) {
                                        printf("Non ho limite inferiore\n");
                                        sprintf(tmpData, "%s%s%s", dateSeparate[1], dateSeparate[2], dateSeparate[3]);
                                        sscanf(tmpData, "%d", &dataDue);
                                        dataUno = 0;
                                        if(dataDue > tot) {
                                            printf("ERRORE: periodo non valido\n");
                                            continue;
                                        }
                                    }
                                    else if(strcmp(dateSeparate[3], "*") == 0) {
                                        printf("Non ho limite superiore\n");
                                        sprintf(tmpData, "%s%s%s", dateSeparate[0], dateSeparate[1], dateSeparate[2]);
                                        sscanf(tmpData, "%d", &dataUno);
                                        dataDue = 99999999;
                                        if(dataUno > tot) {
                                            printf("ERRORE: periodo non valido\n");
                                            continue;
                                        }
                                    }
                                }
                                else {
                                    printf("Non ho limite superiore né inferiore\n");
                                    dataUno = 0;
                                    dataDue = 99999999;
                                }
                                sprintf(periodTmp, "%s_%s_%d:%d_%s.txt", portaChar, aggr, dataUno, dataDue, type);
                                sprintf(fileTmp, "_%s_%d:%d_%s.txt", aggr, dataUno, dataDue, type);
                                if(dataUno == 0 && dataDue == 99999999) {
                                    remove(periodTmp);
                                }
                                printf("Cerco il file %s\n", periodTmp);
                                FILE* fd;
                                fd = fopen(periodTmp, "r");
                                //se non ho il file
                                if(fd == NULL) {
                                    printf("Prima volta che calcolo...\n");
                                    value = controllaDate(dataUno, dataDue, type);
                                    //se ho tutte le entry
                                    if(value == 0) {
                                        printf("Ho tutte le entry!\n");
                                        if(strcmp(aggr, "totale") == 0) {
                                            int totRes = 0;
                                            totRes = calcolaTotale(dataUno, dataDue, type);
                                            printf("Risultato: %d\n", totRes);
                                            if(dataUno == 0 && dataDue == 99999999) {
                                                remove(periodTmp);
                                            }
                                            continue;
                                        }
                                        else {
                                            calcolaVariazione(dataUno, dataDue, type);
                                            if(dataUno == 0 && dataDue == 99999999) {
                                                remove(periodTmp);
                                            }
                                            continue;
                                        }
                                    }
                                    //se non ho tutte le entry
                                    else {
                                        printf("Non ho tutte le entry, contatto i miei vicini\n");
                                        sprintf(messaggioPeer, "REQ_DATA %s", fileTmp);
                                        printf("%s\n", messaggioPeer);
                                        reply = 0;
                                        if(neighbor->portaA != 0) {
                                            ++reply;
                                            pp_addr.sin_port = htons(neighbor->portaA);
                                            len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                                        }
                                        if(neighbor->portaB != 0) {
                                            ++reply;
                                            pp_addr.sin_port = htons(neighbor->portaB);
                                            len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                                        }
                                        printf("Attendo un messaggio di risposta\n");
                                        
                                    }
                                }
                                //se ho il file
                                else {
                                    printf("File trovato\n");
                                    if(strcmp(aggr, "totale") == 0) {
                                        fscanf(fd, "%d", &value);
                                        printf("Risultato: %d\n", value);
                                        fclose(fd);
                                        continue;
                                    }
                                    else {
                                       char scorri[BUFFER_SIZE];
                                       printf("Risultato:\n");
                                       while(fgets(scorri, sizeof(scorri), fd)) {
                                            sscanf(scorri, "%d", &value);
                                            printf("%d\n", value);
                                       }
                                    }
                                }        
                            }
                            else {
                                printf("Valore di type non valido\n");
                                continue;
                            }
                        }
                        else {
                            printf("Valore di aggr non valido\n");
                            continue;
                        }
                    }

                    //COMANDO ADD
                    else if(strcmp(comando, "add") == 0) {   
                        if(controllaGiorno() == 1 && dopo == 0) {
                            dopo = 1;
                            aggiornaGiorno(&anno, &mese, &giorno);
                            salvaSuFile(quantityN, quantityT, anno, mese, giorno);
                            quantityN = 0;
                            quantityT = 0;
                        }                    
                        sscanf(bufferCom, "%s %s %d", comando, type, &quantity);
                        if (quantity <= 0) {
                            printf("Inserisci un numero corretto\n");
                            continue;
                        }
                        if(strcmp(type, "N") == 0)
                            quantityN += quantity;
                        else if(strcmp(type, "T") == 0)
                            quantityT += quantity;
                        else {
                            printf("Inserisci un tipo corretto\n");
                            continue;
                        }
                    }
                    else {
                        printf("Comando non valido\n");
                        continue;
                    }
                }
                else if(j == ssd) {
                    if(controllaGiorno() == 1 && dopo == 0) {
                        dopo = 1;
                        aggiornaGiorno(&anno, &mese, &giorno);
                        salvaSuFile(quantityN, quantityT, anno, mese, giorno);
                        quantityN = 0;
                        quantityT = 0;
                    }
                    ret = recvfrom(ssd, bufferMsg, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, &addrlenPP);
                    sscanf(bufferMsg, "%s", socketMsg);

                    if(strcmp(socketMsg, "NEIGHBORS") == 0) {
                        sscanf(bufferMsg, "%s %d %d", socketMsg, &neighbor->portaA, &neighbor->portaB);
                        printf("Messaggio dal server: Nuovi vicini %d - %d\n", neighbor->portaA, neighbor->portaB);
                    }

                    if(strcmp(socketMsg, "REQ_DATA") == 0) {
                        printf("Ho ricevuto un messaggio del tipo REQ_DATA\n");
                        sscanf(bufferMsg, "%s %s", socketMsg, anteFile);
                        strncpy(fileReq, portaChar, BUFFER_SIZE);
                        strncat(fileReq, anteFile, BUFFER_SIZE);
                        FILE* fdReq;
                        fdReq = fopen(fileReq, "r");
                        strncpy(messaggioPeer, "REPLY_DATA ", BUFFER_SIZE);
                        int num = 0;
                        if(fdReq != NULL) {
                            while(fgets(scorri, sizeof(scorri), fdReq)) {
                                sscanf(scorri, "%d", &num);
                                sprintf(scorri, "%d", num);
                                strncat(messaggioPeer, scorri, BUFFER_SIZE);
                                strncat(messaggioPeer, ";", BUFFER_SIZE);
                            }
                            fclose(fdReq);
                        }
                        else {
                            strncat(messaggioPeer, "-1", BUFFER_SIZE);
                        }                          

                        len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                        printf("Ho risposto al messaggio: %s\n", messaggioPeer);
                    }

                    if(strcmp(socketMsg, "REQ_ENTRIES") == 0) {
                        printf("Ho ricevuto una richiesta di entries\n");
                        sscanf(bufferMsg, "%s %d %d", socketMsg, &dataUnoRic, &dataDueRic);
                        char ponte[BUFFER_SIZE];
                        int checkData;
                        int valoreTmp;
                        char filePrinc[BUFFER_SIZE];
                        char tipo[BUFFER_SIZE];
                        int finalTmp;
                        int trovato = 0;
                        strncpy(filePrinc, portaChar, BUFFER_SIZE);
                        strncat(filePrinc, ".txt", BUFFER_SIZE);

                        FILE* fdPrinc;
                        fdPrinc = fopen(filePrinc, "r");

                        if(fdPrinc != NULL) {
                            while(fgets(ponte, sizeof(ponte), fdPrinc)) {
                                sscanf(ponte, "%d %s %d %d", &checkData, tipo, &valoreTmp, &finalTmp);

                                if(checkData < dataUnoRic)
                                    continue;
                                if(checkData > dataDueRic)
                                    break;

                                sprintf(messaggioPeer, "%d %s %d %d", checkData, tipo, valoreTmp, finalTmp);
                                printf("Ecco la mia entry: %s\n", messaggioPeer);
                                len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                            }
                            printf("Ho finito le entry\n");
                            strncpy(messaggioPeer, "END", BUFFER_SIZE);
                            len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                        }
                        fclose(fdPrinc);
                        continue;
                    }

                    if(strcmp(socketMsg, "REPLY_DATA") == 0) {
                        sscanf(bufferMsg, "%s %s", socketMsg, valoriRicevuti);
                        //se non mi aspettavo la risposta (ovvero se ho gia risposto alla get in modo positivo)
                        if(reply == -1) {  
                            reply = 0;
                            if(neighbor->portaA != 0)
                                ++reply;
                            if(neighbor->portaB != 0)
                                ++reply;
                            continue;
                        }
                        --reply;
                        sscanf(valoriRicevuti, "%d", &value);
                        if(value != -1 && reply >= 0) {
                            printf("Ho ricevuto la risposta!\n");
                            printf("Risultato: \n");
                            FILE* fdNuovo;
                            fdNuovo = fopen(periodTmp, "w");
                            int p = 0;
                            char * split = strtok(valoriRicevuti, ";");
                            while(split != NULL) {
                                valoriFile[p] = split;
                                split = strtok(NULL, ";");
                                fprintf(fdNuovo, "%s\n", valoriFile[p]);
                                printf("%s\n", valoriFile[p++]);
                            }
                            p = 0;
                            fclose(fdNuovo);
                            reply = -1;
                            continue;
                        }
                        else if(value == -1 && reply == 0) {
                            sonoIo = 1;
                            printf("I miei vicini non conoscono la risposta, chiedo le entry a tutti\n");
                            sprintf(messaggioPeer, "FLOOD_FOR_ENTRIES %d %d", dataUno, dataDue);
                            //Invio la richiesta ad uno dei vicini
                            if(neighbor->portaB == 0 && neighbor->portaA != 0) {
                                pp_addr.sin_port = htons(neighbor->portaA);
                                portaPrinc = neighbor->portaA;
                                len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                            }
                            else {
                                pp_addr.sin_port = htons(neighbor->portaB);
                                portaPrinc = neighbor->portaB;
                                len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                            }
                        }
                        else {
                            continue;
                        }
                    }
                    
                    if(strcmp(socketMsg, "FLOOD_FOR_ENTRIES") == 0) {
                        if(sonoIo == 1) {
                            sonoIo = 0;
                            strncpy(messaggioPeer, "", BUFFER_SIZE);
                            len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                            ret = recvfrom(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, &addrlenPP);
                            printf("Peer a cui chiedere le entries: %s\n", messaggioPeer);
                            
                            int z = 0;
                            char porteRicevute[BUFFER_SIZE];
                            strncpy(porteRicevute, messaggioPeer, BUFFER_SIZE);
                            char * porteSeparate[BUFFER_SIZE];
                            char * split = strtok(porteRicevute, " ");
                            int portaNecessaria = 0;
                            int dataRicevuta = 0;
                            int quantitaRicevuta = 0;
                            char tipoRicevuto[BUFFER_SIZE];
                            int finale = 0;
                            while (split != NULL){
                                portaNecessaria = 0;
                                porteSeparate[z] = split;
                                split = strtok(NULL, " ");
                                sprintf(messaggioPeer, "REQ_ENTRIES %d %d", dataUno, dataDue);
                                sscanf(porteSeparate[z], "%d", &portaNecessaria);
                                printf("La porta a cui inviare il messaggio è: %d\n", portaNecessaria);
                                z++;
                                pp_addr.sin_port = htons(portaNecessaria);
                                len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                                printf("Ho inviato un messaggio a %d\n", portaNecessaria);
                                while(1) {
                                    ret = recvfrom(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, &addrlenPP);
                                    printf("Ho ricevuto un messaggio di risposta con entry: %s\n", messaggioPeer);
                                    if(strcmp(messaggioPeer, "END") == 0) {
                                        break;
                                    }
                                    dataRicevuta = 0;
                                    quantitaRicevuta = 0;
                                    strncpy(tipoRicevuto, "", BUFFER_SIZE);
                                    finale = 0;
                                    sscanf(messaggioPeer, "%d %s %d %d", &dataRicevuta, tipoRicevuto, &quantitaRicevuta, &finale);
                                    aggiungiLista(dataRicevuta, tipoRicevuto, quantitaRicevuta, finale);
                                }
                            }
                            aggiornaFile();
                    
                            int result = 0;
                            if(strcmp(aggr, "totale") == 0) {
                                result = calcolaTotale(dataUno, dataDue, type);
                                printf("Risultato: %d\n", result);
                            }   
                            else
                                result = calcolaVariazione(dataUno, dataDue, type);
                            z = 0;
                        }
                        else {
                            printf("Ho ricevuto una richiesta di flooding\n");
                            sscanf(bufferMsg, "%s %d %d", socketMsg, &dataUno, &dataDue);

                            if(ntohs(pp_addr.sin_port) == neighbor->portaA){
                                if(neighbor->portaB != 0) {
                                    portaInoltro = neighbor->portaB;
                                    portaReq = neighbor->portaA;
                                }
                                else {
                                    portaInoltro = neighbor->portaA;
                                    portaReq = neighbor->portaA;
                                }
                            }
                            else{
                                if(neighbor->portaA != 0) {
                                    portaInoltro = neighbor->portaA;
                                    portaReq = neighbor->portaB;
                                }
                                else {
                                    portaInoltro = neighbor->portaB;
                                    portaReq = neighbor->portaB;
                                }
                            }
                            
                            pp_addr.sin_port = htons(portaInoltro);
                            len = sendto(ssd, bufferMsg, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                            ret = recvfrom(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, &addrlenPP);
                            //Ho almeno una entry
                            if(controllaEntry(dataUno, dataDue) > 0) {
                                printf("Ho almeno una entry\n");
                                strncat(messaggioPeer, " ", BUFFER_SIZE);
                                strncat(messaggioPeer, portaChar, BUFFER_SIZE);
                            }
                            pp_addr.sin_port = htons(portaReq);
                            len = sendto(ssd, messaggioPeer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));   
                        }
                    }

                    if(strcmp(socketMsg, "STOP") == 0) {
                        printf("Ho ricevuto un messaggio di Stop da parte del server\n");
                        salvaSuFile(quantityN, quantityT, anno, mese, giorno);
                        exit(0);
                    }
                }
            }
        }
    }
}