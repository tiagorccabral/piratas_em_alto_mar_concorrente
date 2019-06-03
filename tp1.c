/*
    Trabalho Prático de Programação Concorrente - Turma A - 2019/1 - 117935
    Universidade de Brasília
    Problema: Piratas em alto mar

    Versão do compilador:
        GCC version 4.2.1
        Apple LLVM version 10.0.1 (clang-1001.0.46.4)
        Target: x86_64-apple-darwin18.6.0
        Thread model: posix

    Aluno: Tiago Rodrigues da Cunha Cabral
    Matrícula: 15/0150296
    Descrição do problema: 
        Em um mundo existem, muitas tripluações piratas, marinheiros e um rei dos piratas
        O objetivo dos piratas é encontrar uma ilha para começar a utilizar seus recursos
        O rei dos piratas quer também usar os recursos da ilha, e tem sempre vantagem sobre as tripulações.
        Os marinheiros tem objetivo de expulsar os piratas da ilha.
*/

#if defined(_WIN32)
    #define CLEAR system("cls");
#else
    #define CLEAR system("clear");
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define TRUE 1

// Variaveis de controle da quantidade de entidades no programa
#define QTD_TPIRATAS 5 // Quantidade de tripulacao de piratas
#define QTD_REI_PIRATAS 3 // Quantidade de tripulacoes de reis de piratas
#define QTD_TMARINHEIROS 1  // Quantidade de tripulacoes de marinheiros
#define QTD_ILHAS 2 // Quantidade de ilhas no mundo

// Variaveis da quantidade de tempo dos eventos no programa
#define TEMPO_NAVEGACAO 2 // Tempo que uma tripulacao passa navegando ate encontrar uma ilha
#define TEMPO_MIN_USO 3 // Tempo que uma tripulacao passa obrigatoriamente usando a ilha

// Variaveis logicas para determinar estado de acesso a ilha
#define DOMINIO_NEUTRO 0 // Define controle da ilha como neutro
#define REI_PIRATA 1 // Define controle da ilha para rei dos piratas
#define TRIPULACAO_PIRATA 2 // Define controle da ilha para a tripulacao pirata
#define MARINHA 3 // Define controle da ilha para a marinha

// Variaveis de combate
#define FORCA_MIN_TP 1 // forca minima que uma tripulacao de piratas pode ter
#define FORCA_MAX_TP 5 // forca maxima que uma tripulacao de piratas pode ter
#define FORCA_MIN_RP 10 // forca minima que uma tripulacao de rei dos piratas pode ter
#define FORCA_MAX_RP 20 // forca maxima que uma tripulacao de rei dos piratas pode ter

int ilhas_ocupadas = 0; // Quantidade de ilhas ocupadas, o maximo eh definido 
int trip_rei_pirata = 0, trip_pirata = 0, trip_marinha = 0;
int duelo_pirata[QTD_ILHAS][2];
int quer_entrar_ilha[QTD_ILHAS];

pthread_mutex_t mutex_ilhas[QTD_ILHAS] = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tripulacao = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_trip_quer_entrar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t turno = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mutex_ilhas_ocupdas[QTD_ILHAS];
pthread_cond_t ilha_em_batalha[QTD_ILHAS];
pthread_cond_t rp_cond[QTD_ILHAS];
pthread_cond_t tp_cond[QTD_ILHAS];
pthread_cond_t cond_ilhas_ocupadas[QTD_ILHAS];
sem_t *sem_ilhas_livres[QTD_ILHAS];
sem_t *sem_ilhas[QTD_ILHAS];

// Struct que determina a forca de uma tripulacao pirata
typedef struct {
    int id;
    int forca;
    int tipo_trip;
} atributos_tripulacao;

atributos_tripulacao estado_ilhas[QTD_ILHAS]; // Vetor que armazena quem esta sob controle da ilha atualmente

// Randomiza a forca para as tripulacoes de piratas ou rei piratas
int inicializa_forca_tripulacoes(int tripulacoes[], int tipo) {
    int i=0, forca, erro = 0;
    srand(time(NULL)); 
    if (tipo == TRIPULACAO_PIRATA) {
        for (i=0;i<QTD_TPIRATAS;i++) {
            tripulacoes[i] = ((rand() % FORCA_MAX_TP) + FORCA_MIN_TP); 
        }
    } else if (tipo == REI_PIRATA) {
        for (i=0;i<QTD_REI_PIRATAS;i++) {
            tripulacoes[i] = ((rand() % FORCA_MAX_RP) + FORCA_MIN_RP); 
        }
    } else {
        erro = 1;
    }
    return erro;
}

// Randomiza uma ilha em que a tripulacao ira chegar a seguir
int determinar_ilha_para_tripulacao() {
    int ilha=0;
    ilha = (rand() % (QTD_ILHAS)); 
    return ilha;
}

// Retorna 0 se desafiado vence, retorna 1 se desafiante vence
int determinar_vencedor(int forca_desafiado, int forca_desafiante) {
    if (forca_desafiado >= forca_desafiante) {
        return 0;
    }
    return 1;
}

void* tripulacao_pirata(void *arg) {
    atributos_tripulacao atributos_tp = *((atributos_tripulacao *)arg);
    int id = atributos_tp.id;
    int forca = atributos_tp.forca;
    int tipo_trip = atributos_tp.tipo_trip;
    int perdeu_duelo = 0;
    int ilha_destino = 0;
    printf("Tripulação pirata id: %d apareceu de forca %d\n", id, forca);
    while(TRUE) {
        // // pirata comeca a navegar
        // printf("TP %d navegando\n", id);
        // sleep(rand() % TEMPO_NAVEGACAO);

        // chega em uma ilha
        ilha_destino = determinar_ilha_para_tripulacao();

        // verifica se existe alguem esperando para entrar

        // se existe entra em batalha

        // entra na ilha se nao existe mais ninguem esperando
        if (pthread_mutex_trylock(&mutex_ilhas[ilha_destino]) == 0) {
            trip_pirata++;
            while(estado_ilhas[ilha_destino].tipo_trip != DOMINIO_NEUTRO || trip_rei_pirata > 0) {
                printf("Tripulação pirata %d aguardando ilha %d\n", id, ilha_destino);
                pthread_cond_wait(&rp_cond[ilha_destino],&mutex_ilhas[ilha_destino]);
            }
            trip_pirata--;
            estado_ilhas[ilha_destino].tipo_trip = tipo_trip;
            printf("Tripulação pirata %d entrou na ilha %d\n", id, ilha_destino);
            sleep(rand() % TEMPO_MIN_USO);
            estado_ilhas[ilha_destino].tipo_trip = DOMINIO_NEUTRO;
            printf("Tripulação pirata %d saiu da ilha %d, estado atual: %d\n", id, ilha_destino, estado_ilhas[ilha_destino].tipo_trip);
            pthread_cond_signal(&rp_cond[ilha_destino]);
            pthread_cond_signal(&tp_cond[ilha_destino]);
            pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
            sleep(1);
        } else {
            printf("Tripulação pirata %d não entrou na ilha %d\n", id, ilha_destino);
            sleep(1);
        }
    }

    pthread_exit(0);
}

void* rei_pirata(void *arg) {
    atributos_tripulacao atributos_tp = *((atributos_tripulacao *)arg);
    int id = atributos_tp.id;
    int forca = atributos_tp.forca;
    int tipo_trip = atributos_tp.tipo_trip;
    int perdeu_duelo = 0;
    int ilha_destino = 0;
    printf("Rei pirata id: %d apareceu de forca %d\n", id, forca);
    while(TRUE) {
        // rei pirata comeca a navegar
        printf("RP %d navegando\n", id);
        sleep(rand() % TEMPO_NAVEGACAO);

        // chega em uma ilha
        ilha_destino = determinar_ilha_para_tripulacao();

        // verifica se existe alguem esperando para entrar

        // se existe entra em batalha

        // entra na ilha se nao existe mais ninguem esperando
        pthread_mutex_lock(&mutex_ilhas[ilha_destino]);
        trip_rei_pirata++;
        while(estado_ilhas[ilha_destino].tipo_trip != DOMINIO_NEUTRO) {
            printf("Rei pirata %d aguardando ilha %d\n", id, ilha_destino);
            pthread_cond_wait(&rp_cond[ilha_destino],&mutex_ilhas[ilha_destino]);
        }
        trip_rei_pirata--;
        estado_ilhas[ilha_destino].tipo_trip = tipo_trip;
        printf("Tripulação do rei pirata %d entrou na ilha %d\n", id, ilha_destino);
        sleep(rand() % TEMPO_MIN_USO);
        estado_ilhas[ilha_destino].tipo_trip = DOMINIO_NEUTRO;
        printf("Tripulação do rei pirata %d saiu da ilha %d, estado atual: %d\n", id, ilha_destino, estado_ilhas[ilha_destino].tipo_trip);
        pthread_cond_signal(&rp_cond[ilha_destino]);
        pthread_cond_signal(&tp_cond[ilha_destino]);
        pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
        sleep(1);
    }

    pthread_exit(0);
}

void* tripulacao_marinha(void *arg) {
    int id = *((int *)arg);
    printf("Tripulação da marinha id: %d apareceu\n", id);
    while(TRUE) {

    }

    pthread_exit(0);
}

int main () {
    int i, erro;
    int *id;
    int forca_trip_piratas[QTD_TPIRATAS];
    int forca_trip_rei_piratas[QTD_REI_PIRATAS];
    pthread_t tp_threads[QTD_TPIRATAS]; // threads de tripulacoes piratas
    pthread_t rp_threads[QTD_REI_PIRATAS]; // threads de rei dos piratas
    pthread_t tm_threads[QTD_TMARINHEIROS]; // threads de tripulacoes de marinheiros

    atributos_tripulacao atributos;

    // inicializa vetor de dominio das ilhas como neutro
    for (i=0; i<QTD_ILHAS; i++) {
        estado_ilhas[i].id = DOMINIO_NEUTRO;
        estado_ilhas[i].tipo_trip = DOMINIO_NEUTRO;
        estado_ilhas[i].forca = 0;
    }

    
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_mutex_init(&mutex_ilhas[i], NULL);
    }
    
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_cond_init(&cond_ilhas_ocupadas[i], NULL);
    }

    for (i=0; i<QTD_ILHAS; i++) {
        pthread_cond_init(&tp_cond[i], NULL);
    }

    for (i=0; i<QTD_ILHAS; i++) {
        pthread_cond_init(&rp_cond[i], NULL);
    }

    for (i=0; i<QTD_ILHAS; i++) {
        pthread_cond_init(&ilha_em_batalha[i], NULL);
    }

    CLEAR
    printf("Inicializando...\n");
    sleep(2);
    CLEAR

    // inicializa forca para piratas
    erro = inicializa_forca_tripulacoes(forca_trip_piratas, TRIPULACAO_PIRATA);
    if (erro != 0) {
        printf("erro ao inicializar as forcas! Saindo\n");
        exit(0);
    }

    // inicializa forca para rei dos piratas
    erro = inicializa_forca_tripulacoes(forca_trip_rei_piratas, REI_PIRATA);
    if (erro != 0) {
        printf("erro ao inicializar as forcas! Saindo\n");
        exit(0);
    }

    // Inicializa as threads dos piratas
    for (i = 0; i < QTD_TPIRATAS; i++) {
        atributos_tripulacao *atributos = malloc(sizeof(atributos_tripulacao));
        atributos->id = i;
        atributos->forca = forca_trip_piratas[i];
        atributos->tipo_trip = TRIPULACAO_PIRATA;
        erro = pthread_create(&tp_threads[i], NULL, tripulacao_pirata, (void *)(atributos));
        if (erro) {
            printf("erro ao criar a Thread id %d\n", i);
            exit(1);
        }
    }

    // Inicializa as threads dos reis dos piratas
    for (i = 0; i < QTD_REI_PIRATAS; i++) {
        atributos_tripulacao *atributos = malloc(sizeof(atributos_tripulacao));
        atributos->id = i;
        atributos->forca = forca_trip_rei_piratas[i];
        atributos->tipo_trip = REI_PIRATA;
        erro = pthread_create(&rp_threads[i], NULL, rei_pirata, (void *)(atributos));
        if (erro) {
            printf("erro ao criar a Thread id %d\n", i);
            exit(1);
        }
    }

    for (i=0; i<QTD_ILHAS; i++) {
        sem_ilhas[i] = sem_open("/sem_ilhas", O_CREAT, 0644, QTD_ILHAS);
    }
    // sem_ilhas = sem_open("/sem_ilhas", O_CREAT, 0644, 0);
    // if (sem_ilhas == SEM_FAILED) {
    //     perror("Falha ao abrir semaforo");
    //     exit(-1);
    // }

    for (i=0; i<QTD_ILHAS; i++) {
        sem_ilhas_livres[i] = sem_open("/sem_ilhas_livres", O_CREAT, 0644, 0);
    }
    // if (sem_ilhas_livres == SEM_FAILED) {
    //     perror("Falha ao abrir semaforo");
    //     exit(-1);
    // }

    // Inicializa as threads das tripulacoes de marinheiros
    for (i = 0; i < QTD_TMARINHEIROS; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&tm_threads[i], NULL, tripulacao_marinha, (void *)(id));
        if (erro) {
            printf("erro ao criar a Thread id %d\n", i);
            exit(1);
        }    
    }


    // Junta as threas
    for (i = 0; i < QTD_TPIRATAS; i++){ 
        pthread_join(tp_threads[i], NULL);
    }

    for (i = 0; i < QTD_REI_PIRATAS; i++){ 
        pthread_join(rp_threads[i], NULL);
    }

    for (i = 0; i < QTD_TMARINHEIROS; i++){ 
        pthread_join(tm_threads[i], NULL);
    }

    return 0;
}