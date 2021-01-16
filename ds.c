#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int sd, ret, len, new_sd;
char buffer[BUFFER_SIZE];
struct sockaddr_in myaddr, pp_addr;
fd_set master;      //set principale gestito dal programmatore
fd_set read_fds;    //set gestito dalla select
int fdmax;
int i;
int addrlen = sizeof(pp_addr);
int portaServer;
int numeroPeer = 0;

struct peer {
    char ip[BUFFER_SIZE];
    char porta[BUFFER_SIZE];
    struct peer* next;
};
struct peer* testa = NULL;

struct vicini {  
    int portaA;
    int portaB;
};
char bufferVicini[BUFFER_SIZE];
char comando[BUFFER_SIZE];

//Controlla se un peer è nella lista o meno. Ritorna 1 se c'è, 0 altrimenti
int controllaPeer(char* ip, char* door) {
    struct peer* checker = testa;
    while(checker != NULL) {
        if(strcmp(ip, checker->ip) == 0) {
            if(strcmp(door, checker->porta) == 0)
                return 1;
        }
        checker = checker->next;
    }
    return 0;
}

void aggiungiPeer(char* ip, char* door) {
    struct peer* elem = NULL;
    elem = (struct peer*) malloc(sizeof(struct peer));
    if (elem == NULL) {
        printf("Failed to insert element. Out of memory\n");
        return;
    }

    strcpy(elem->ip, ip);
    strncpy(elem->porta, door, BUFFER_SIZE);
    elem->next = NULL;

    //INSERIMENTO IN TESTA
    if(testa == NULL) {
        testa = elem;
        return;
    }
    else {
        struct peer* checker = testa;
        struct peer* succ = checker->next;
        while(checker != NULL) {
             //INSERIMENTO IN FONDO
            if(succ == NULL) {
                checker->next = elem;
                return;
            }
            //INSERIMENTO IN MEZZO
            if(atoi(succ->porta) > atoi(elem->porta)) {
                elem->next = succ;
                checker->next = elem;
                return;
            }
            checker = succ;
            succ = succ->next;
        }
    }
    return;
}

int ultimaPorta() {
    struct peer *checker = testa;
    while(checker->next != NULL)
        checker = checker->next;
    return atoi(checker->porta);
}

struct vicini* trovaVicini(char* porta) {
    struct vicini* porte = NULL;
    porte = (struct vicini*) malloc(sizeof(struct vicini));
    if (porte == NULL) {
        printf("Failed to insert element. Out of memory\n");
        return;
    }
    struct peer* checker = testa;
    struct peer* succ = checker->next;
    struct peer* prec = NULL;

    int portaPrec = 0;
    int portaSucc = 0;
    int primaPort = atoi(testa->porta);
    int ultimaPort = ultimaPorta();
    while(checker != NULL) {
        if(strcmp(checker->porta, porta) == 0) {
            if(prec != NULL)
                portaPrec = atoi(prec->porta);
            if(succ != NULL)
                portaSucc = atoi(succ->porta);
            break;
        }
        prec = checker;
        checker = checker->next;
        succ = succ->next;
    }
    if(portaPrec == 0 && numeroPeer > 2)
        porte->portaA = ultimaPort;
    else
        porte->portaA = portaPrec;
    if(portaSucc == 0 && numeroPeer > 2)
        porte->portaB = primaPort;
    else
        porte->portaB = portaSucc;

    return porte;
}

void allineaVicini(int porta) {
    struct vicini* nuovi = NULL;
    nuovi = (struct vicini*) malloc(sizeof(struct vicini));
    if (nuovi == NULL) {
        printf("Failed to insert element. Out of memory\n");
        return;
    }
    char portaString[BUFFER_SIZE];
    sprintf(portaString, "%d", porta);
    nuovi = trovaVicini(portaString);
    sprintf(bufferVicini, "%s %d %d","NEIGHBORS", nuovi->portaA, nuovi->portaB);
    pp_addr.sin_port = htons(porta);
    len = sendto(sd, bufferVicini, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));

}

void rimuoviPeer(char *peerDaRimuovere){
    struct peer* checker = testa;
    struct peer* prec = NULL;
    while(checker != NULL) {
        if(strcmp(checker->porta, peerDaRimuovere) == 0) {
            if(prec == NULL) {
                testa = checker->next;
                return;
            }
            else {
                prec->next = checker->next;
                return;
            }
        }
        prec = checker;
        checker = checker->next;
    }
}

int main(int argc, char* argv[]) {
    char portaRichiesta[BUFFER_SIZE];
    char portaString[BUFFER_SIZE];

    if(argc > 1)
        portaServer = atoi(argv[1]);
    else {
        printf("Error: Could not boot without door. Use './ds <door>' to boot\n");
        exit(-1);
    }

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(0, &master);

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_port = htons(portaServer);
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sd, (struct sockaddr*)&myaddr, sizeof(myaddr));
    if(ret < 0) {
        perror("Errore in fase di binding: \n");
        exit(-1);
    }
    FD_SET(sd, &master);    //aggiungo il socket di ascolto al set
    fdmax = sd;             //il maggiore ora è sd

    printf("\n***************DS COVID STARTED***************\n");
    printf("Digita un comando: \n");
    printf("1) status --> mostra un elenco dei peer connessi\n");
    printf("2) showneighbor <peer> --> mostra i neighbor di un peer\n");
    printf("3) esc --> chiude il DS\n");

    while(1) {
        strncpy(portaRichiesta, "", BUFFER_SIZE);
        strncpy(portaString, "", BUFFER_SIZE);
        strncpy(comando, "", BUFFER_SIZE);
        strncpy(buffer, "", BUFFER_SIZE);
        fflush(stdin);
        fflush(stdout);

        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        for(i = 0; i <= fdmax; ++i) {
            if(FD_ISSET(i, &read_fds)) {
                if(i == sd) {
                    ret = recvfrom(sd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, &addrlen);
                    char *ipString = inet_ntoa(pp_addr.sin_addr);
                    int portaInt = ntohs(pp_addr.sin_port);
                    sprintf(portaString, "%d", portaInt);
                    if(strcmp(buffer,"START") == 0) {
                        if(controllaPeer(ipString, portaString) == 0) {
                            aggiungiPeer(ipString, portaString);
                            ++numeroPeer;
                            struct vicini* nuovi = NULL;
                            nuovi = (struct vicini*) malloc(sizeof(struct vicini));
                            if (nuovi == NULL) {
                                printf("Failed to insert element. Out of memory\n");
                                return;
                            }
                            nuovi = trovaVicini(portaString);
                            int portaPrec = nuovi->portaA;
                            int portaSucc = nuovi->portaB;
                            sprintf(bufferVicini, "%s %d %d", "NEIGHBORS", nuovi->portaA, nuovi->portaB);
                            len = sendto(sd, bufferVicini, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                            if(portaPrec != 0)
                                allineaVicini(portaPrec);
                            if(portaSucc != 0)
                                allineaVicini(portaSucc);
                        }
                    }
                    else if(strcmp(buffer,"STOP") == 0) {
                        int portaReq = ntohs(pp_addr.sin_port);
                        char portaDisc[BUFFER_SIZE];
                        sprintf(portaDisc, "%d", portaReq);
                        printf("%s\n", portaDisc);
                        struct vicini* peerDisc = NULL;
                        peerDisc = (struct vicini*) malloc(sizeof(struct vicini));
                        if (peerDisc == NULL) {
                            printf("Failed to insert element. Out of memory\n");
                            return;
                        }
                        peerDisc = trovaVicini(portaDisc);
                        rimuoviPeer(portaDisc);
                        int portaPrec = peerDisc->portaA;
                        int portaSucc = peerDisc->portaB;
                        if(portaPrec != 0)
                            allineaVicini(portaPrec);
                        if(portaSucc != 0)
                            allineaVicini(portaSucc);
                    }
                }
                else if(i == 0) {
                    fgets(buffer, BUFFER_SIZE, stdin);
                    sscanf(buffer, "%s", comando);
                    if(strcmp(comando, "status") == 0) {
                        struct peer* checker = testa;
                        printf("Peer Connessi:\n");
                        while(checker != NULL) {
                            printf("%s:%s\n", checker->ip, checker->porta);
                            checker = checker->next;
                        }
                    }

                    else if(strcmp(comando, "showneighbor") == 0) {
                        sscanf(buffer, "%s %s", comando, portaRichiesta);
                        if(strcmp(portaRichiesta, "") != 0) {
                            if(controllaPeer("127.0.0.1", portaRichiesta) == 0) {
                                printf("Porta non presente\n");
                                continue;
                            }
                            printf("Vicini di %s: ", portaRichiesta);
                            struct vicini* relativi = NULL;
                            relativi = (struct vicini*) malloc(sizeof(struct vicini));
                            if (relativi == NULL) {
                                printf("Failed to insert element. Out of memory\n");
                                return;
                            }
                            relativi = trovaVicini(portaRichiesta);
                            printf("%d, %d\n", relativi->portaA, relativi->portaB);
                        }
                        else {
                            struct peer* checker = testa;
                            struct vicini* relativi = NULL;
                            relativi = (struct vicini*) malloc(sizeof(struct vicini));
                            if (relativi == NULL) {
                                printf("Failed to insert element. Out of memory\n");
                                return;
                            }
                            printf("Peer connessi e relativi vicini:\n");
                            while(checker != NULL) {
                                printf("%s:%s", checker->ip, checker->porta);
                                relativi = trovaVicini(checker->porta);
                                printf(" vicini: %d, %d\n", relativi->portaA, relativi->portaB);
                                checker = checker->next;
                            }
                        }
                    }
                    else if(strcmp(comando, "esc") == 0) {
                        struct peer* checker = testa;
                        int doorPeer;
                        while(checker != NULL) {
                            sscanf(checker->porta, "%d", &doorPeer);
                            pp_addr.sin_port = htons(doorPeer);
                            strncpy(buffer, "STOP", BUFFER_SIZE);
                            len = sendto(sd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&pp_addr, sizeof(pp_addr));
                            checker = checker->next;
                        }
                        exit(0);
                    }
                } 
            }
        }
    }
}