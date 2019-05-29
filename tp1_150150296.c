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
    Descrição do problema: Em um mundo existem, muitas tripluações piratas, marinheiros e um rei dos piratas
    O objetivo dos piratas é encontrar uma ilha para começar a utilizar seus recursos
    O rei dos piratas quer também usar os recursos da ilha, e tem sempre vantagem sobre as tripulações.
    Os marinheiros tem objetivo de expulsar os piratas da ilha.
*/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>


#define TRUE 1
#define QTD_TPIRATAS 5 // Quantidade de tripulacao de piratas
#define QTD_REI_PIRATAS 1 // Quantidade de tripulacoes de reis de piratas
#define QTD_TMARINHEIROS 1  // Quantidade de tripulacoes de marinheiros
#define QTD_ILHAS 1 // Quantidade de ilhas no mundo
#define TEMPO_NAVEGACAO 2 // Tempo que uma tripulacao passa navegando ate encontrar uma ilha
#define TEMPO_MIN_USO 2 // Tempo que uma tripulacao passa obrigatoriamente usando a ilha

// int ilhas_disponiveis = QTD_ILHAS;
int ilhas_ocupadas = 0; // Quantidade de ilhas ocupadas, o maximo eh definido 
int trip_rei_pirata = 0, trip_pirata = 0, trip_marinha = 0;

pthread_mutex_t mutex_ilhas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tripulacao = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rp_cond = PTHREAD_COND_INITIALIZER;
sem_t *sem_ilhas_livres;

void* tripulacao_pirata(void *arg) {
    int id = *((int *)arg);
    printf("Tripulação pirata id: %d apareceu\n", id);
    while(TRUE) {
        // // pirata comeca a navegar
        // printf("TP %d navegando\n", id);
        // sleep(rand() % TEMPO_NAVEGACAO);
        // // sem_wait(sem_ilhas_livres);
        // pthread_mutex_lock(&mutex_ilhas);
        // ilhas_disponiveis--;
        // printf("Tripulação pirata %d entrou na ilha\n", id);
        // sleep(rand() % TEMPO_MIN_USO);
        // ilhas_disponiveis++;
        // printf("Tripulação pirata %d saiu da ilha\n", id);
        // pthread_mutex_unlock(&mutex_ilhas);
        // // sem_post(sem_ilhas_livres);
        // sleep(1);

    }

    pthread_exit(0);
}

void* rei_pirata(void *arg) {
    int id = *((int *)arg);
    printf("Rei pirata id: %d apareceu!\n", id);
    while(TRUE) {
        // pirata comeca a navegar
        printf("RP %d navegando\n", id);
        sleep(rand() % TEMPO_NAVEGACAO);
        // sem_wait(sem_ilhas_livres);
        pthread_mutex_lock(&mutex_ilhas);
        trip_rei_pirata++;
        while(ilhas_ocupadas == QTD_ILHAS) {
            printf("Rei pirata aguardando ilha %d\n", id);
            pthread_cond_wait(&rp_cond,&mutex_ilhas);
        }
        trip_rei_pirata--;
        ilhas_ocupadas++;
        printf("Tripulação do rei pirata %d entrou na ilha\n", id);
        sleep(rand() % TEMPO_MIN_USO);
        ilhas_ocupadas--;
        printf("Tripulação do rei pirata %d saiu da ilha, ilhas ocupadas: %d\n", id, ilhas_ocupadas);
        pthread_cond_signal(&rp_cond);
        pthread_mutex_unlock(&mutex_ilhas);
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
    pthread_t tp_threads[QTD_TPIRATAS]; // threads de tripulacoes piratas
    pthread_t rp_threads[QTD_REI_PIRATAS]; // threads de rei dos piratas
    pthread_t tm_threads[QTD_TMARINHEIROS]; // threads de tripulacoes de marinheiros

    // Inicializa as threads dos piratas
    for (i = 0; i < QTD_TPIRATAS; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&tp_threads[i], NULL, tripulacao_pirata, (void *)(id));
        if (erro) {
            printf("erro ao criar a Thread id %d\n", i);
            exit(1);
        }
    }

    // Inicializa as threads dos reis dos piratas
    for (i = 0; i < QTD_REI_PIRATAS; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&rp_threads[i], NULL, rei_pirata, (void *)(id));
        if (erro) {
            printf("erro ao criar a Thread id %d\n", i);
            exit(1);
        }
    }

    sem_ilhas_livres = sem_open("/sem_ilhas", O_CREAT, 0644, QTD_ILHAS);
    if (sem_ilhas_livres == SEM_FAILED) {
        perror("Falha ao abrir semaforo");
        exit(-1);
    }

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