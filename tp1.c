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
#include "vec/vec.c"

#define TRUE 1

// Variaveis de controle da quantidade de entidades no programa
#define QTD_TPIRATAS 6 // Quantidade de tripulacao de piratas
#define QTD_REI_PIRATAS 3 // Quantidade de tripulacoes de reis de piratas
#define QTD_TMARINHEIROS 1  // Quantidade de tripulacoes de marinheiros
#define QTD_ILHAS 2 // Quantidade de ilhas no mundo

// Variaveis da quantidade de tempo dos eventos no programa
#define TEMPO_NAVEGACAO 2 // Tempo que uma tripulacao passa navegando ate encontrar uma ilha
#define TEMPO_MIN_USO 6 // Tempo que uma tripulacao passa obrigatoriamente usando a ilha
#define TEMPO_NAVEGACAO_MARINHA 30 // Tempo que uma tripulacao da marinha passa navegando
#define TEMPO_MARINHA_ILHA 10 // Tempo que uma tripulacao da marinha passa navegando
#define TEMPO_DURACAO_BATALHA 2 // Tempo que dura em media uma batalha

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

/*
Cores
*/
#define COLOR_BLACK "\x1b[30m"
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_BRIGHT_RED "\x1b[91m"
#define COLOR_BRIGHT_GREEN "\x1b[92m"
#define COLOR_BRIGHT_YELLOW "\x1b[93m"
#define COLOR_BRIGHT_BLUE "\x1b[94m"
#define COLOR_BRIGHT_MAGENTA "\x1b[95m"
#define COLOR_BRIGHT_CYAN "\x1b[96m"
#define COLOR_RESET "\x1b[0m"

int ilhas_ocupadas = 0; // Quantidade de ilhas ocupadas, o maximo eh definido 
int trip_rei_pirata = 0, trip_pirata = 0, trip_marinha = 0;
// int duelo_pirata[QTD_ILHAS][2];
int quer_entrar_ilha[QTD_ILHAS];
// int duelo_ilha[QTD_ILHAS]
int venceu_batalha[QTD_ILHAS];
int marinha_sob_controle[QTD_ILHAS];
vec_int_t batalha_na_ilha[QTD_ILHAS];

pthread_mutex_t mutex_ilhas[QTD_ILHAS];
pthread_mutex_t mutex_desafiante[QTD_ILHAS];
pthread_mutex_t mutex_vencedores[QTD_ILHAS];
pthread_mutex_t marinha_na_ilha[QTD_ILHAS];
pthread_mutex_t mutex_printando_batalha[QTD_ILHAS];
pthread_cond_t ilha_em_batalha[QTD_ILHAS];

// Struct que determina a forca de uma tripulacao pirata
typedef struct {
    int id;
    int forca;
    int tipo_trip;
} atributos_tripulacao;

// Vetor que armazena o tipo, forca e id de cada tripulacao no mundo
atributos_tripulacao mapa_das_tripulacoes[QTD_TPIRATAS + QTD_REI_PIRATAS + QTD_TMARINHEIROS]; 


// Randomiza a forca para as tripulacoes de piratas ou rei piratas
int inicializa_forca_tripulacoes(int tripulacoes[], int tipo) {
    int i=0, erro = 0;
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
int realiza_batalha(int ilha) {
    int i = 0;
    int maior = -1;
    for (i=0; i<batalha_na_ilha[ilha].length; i++) {
        if (mapa_das_tripulacoes[batalha_na_ilha[ilha].data[i]].forca >= maior ) {
            maior = mapa_das_tripulacoes[batalha_na_ilha[ilha].data[i]].id;
        }
    }
    sleep(TEMPO_DURACAO_BATALHA);
    return maior;
}

void* decide_batalhas(void *arg) {
    int i = *(int *)(arg);
    int j;
    while(TRUE) {
        pthread_mutex_lock(&mutex_desafiante[i]);
        if (batalha_na_ilha[i].length == 2 && marinha_sob_controle[i] == 0) {
            pthread_mutex_lock(&mutex_printando_batalha[i]);
            printf("\n ========================= \n");
            printf(COLOR_BRIGHT_YELLOW "\nTemos aqui um duelo! Uma batalha pela ilha %d\n" COLOR_RESET, i);
            printf(COLOR_BRIGHT_YELLOW "\nA batalha será entre:\n" COLOR_RESET);
            for (j=0;j<batalha_na_ilha[i].length;j++) {
                if (mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].tipo_trip == REI_PIRATA) {
                    printf(COLOR_YELLOW "Tripulação do rei pirata %d\n" COLOR_RESET, mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].id);
                } else if (mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].tipo_trip == TRIPULACAO_PIRATA) {
                    printf(COLOR_YELLOW "Tripulação pirata %d\n" COLOR_RESET, mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].id);
                }
            }
            printf("\n ========================= \n");
            pthread_mutex_lock(&mutex_vencedores[i]);
            venceu_batalha[i] = realiza_batalha(i);
            vec_clear(&batalha_na_ilha[i]);
            pthread_mutex_unlock(&mutex_printando_batalha[i]);
            pthread_mutex_unlock(&mutex_vencedores[i]);
            pthread_cond_broadcast(&ilha_em_batalha[i]);
        } else if (batalha_na_ilha[i].length >= 3 && marinha_sob_controle[i] == 0) {
            pthread_mutex_lock(&mutex_vencedores[i]);
            pthread_mutex_lock(&mutex_printando_batalha[i]);
            printf(COLOR_BRIGHT_YELLOW "\nTemos um  Battle Royale! Uma batalha pela ilha %d\n" COLOR_RESET, i);
            printf(COLOR_BRIGHT_YELLOW "\nA batalha será entre:\n" COLOR_RESET);
            for (j=0;j<batalha_na_ilha[i].length;j++) {
                if (mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].tipo_trip == REI_PIRATA) {
                    printf(COLOR_YELLOW "Tripulação do rei pirata %d\n" COLOR_RESET, mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].id);
                } else if (mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].tipo_trip == TRIPULACAO_PIRATA) {
                    printf(COLOR_YELLOW "Tripulação pirata %d\n" COLOR_RESET, mapa_das_tripulacoes[batalha_na_ilha[i].data[j]].id);
                }
            }
            printf("\n ========================= \n");
            venceu_batalha[i] = realiza_batalha(i);
            vec_clear(&batalha_na_ilha[i]);
            pthread_mutex_unlock(&mutex_printando_batalha[i]);
            pthread_mutex_unlock(&mutex_vencedores[i]);
            pthread_cond_broadcast(&ilha_em_batalha[i]);
        }
        pthread_mutex_unlock(&mutex_desafiante[i]);
    }
    sleep(1);
}

void* tripulacao_pirata(void *arg) {
    atributos_tripulacao atributos_tp = *((atributos_tripulacao *)arg);
    int id = atributos_tp.id;
    int forca = atributos_tp.forca;
    int ilha_destino = 0;
    printf(COLOR_BRIGHT_BLUE "Tripulação pirata id: %d apareceu de forca %d\n" COLOR_RESET, id, forca);
    while(TRUE) {
        // // pirata comeca a navegar
        // printf("TP %d navegando\n", id);
        sleep(rand() % TEMPO_NAVEGACAO);

        // chega em uma ilha
        ilha_destino = determinar_ilha_para_tripulacao();

        // verifica se existe alguem esperando para entrar

        // se existe entra em batalha
        printf("Tripulacao pirata %d se aproximando da ilha %d\n", id, ilha_destino);
        if (marinha_sob_controle[ilha_destino] == 0) {
            if(venceu_batalha[ilha_destino] != -1 && pthread_mutex_trylock(&mutex_ilhas[ilha_destino]) == 0) {
                // entra na ilha pois ela esta vazia
                pthread_mutex_lock(&mutex_vencedores[ilha_destino]);
                printf(COLOR_GREEN "\n\t\t\t\t\t\t\t\t\t\tTripulação pirata %d entrou na ilha %d\n" COLOR_RESET, id, ilha_destino);
                sleep(TEMPO_MIN_USO);
                printf(COLOR_BRIGHT_RED "\t\t\t\t\t\t\t\t\t\tTripulação pirata %d saiu da ilha %d\n" COLOR_RESET, id, ilha_destino);
                pthread_mutex_unlock(&mutex_vencedores[ilha_destino]);
                pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
            } else {
                // lista de espera para acessar a ilha
                pthread_mutex_lock(&mutex_desafiante[ilha_destino]);
                if (venceu_batalha[ilha_destino] != -1) {
                    // existe um vencedor no duelo da batalha pela ilha destino
                    pthread_mutex_unlock(&mutex_desafiante[ilha_destino]);
                    pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
                    sleep(2);
                } else {
                    // entra na lista de espera pelo duelo na ilha
                    vec_push(&batalha_na_ilha[ilha_destino], id);
                    while (batalha_na_ilha[ilha_destino].length != 0) {
                        // aguarda ate a batalha terminar
                        pthread_cond_wait(&ilha_em_batalha[ilha_destino], &mutex_desafiante[ilha_destino]);
                    }
                    pthread_mutex_unlock(&mutex_desafiante[ilha_destino]);
                    if (venceu_batalha[ilha_destino] == id) {
                        // venceu a batalha
                        pthread_mutex_lock(&mutex_ilhas[ilha_destino]);
                        pthread_mutex_lock(&mutex_vencedores[ilha_destino]);
                        printf(COLOR_BRIGHT_CYAN "\n\t\t\t\t\t\t\t\t\t\tTripulacao pirata %d venceu a batalha pela ilha %d\n" COLOR_RESET, id, ilha_destino);
                        venceu_batalha[ilha_destino] = -1;
                        printf(COLOR_GREEN "\n\t\t\t\t\t\t\t\t\t\tTripulação pirata %d entrou na ilha %d após vencer a batalha\n" COLOR_RESET, id, ilha_destino);
                        sleep(TEMPO_MIN_USO);
                        printf(COLOR_BRIGHT_RED "\t\t\t\t\t\t\t\t\t\tTripulação pirata %d saiu depois de vencer, da ilha %d\n" COLOR_RESET, id, ilha_destino);
                        printf("\n\t\t\t\t\t\t\t\t\t\t ========================= \n");
                        pthread_mutex_unlock(&mutex_vencedores[ilha_destino]);
                        pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
                    } else {
                        // perdeu a batalha
                        sleep(2);
                    }
                }
            }
        }
        sleep(2);
    }

    pthread_exit(0);
}

void* rei_pirata(void *arg) {
    atributos_tripulacao atributos_tp = *((atributos_tripulacao *)arg);
    int id = atributos_tp.id;
    int forca = atributos_tp.forca;
    int ilha_destino = 0;
    printf(COLOR_GREEN "Rei pirata id: %d apareceu de forca %d\n" COLOR_RESET, id, forca);
    while(TRUE) {
        sleep(rand() % TEMPO_NAVEGACAO);

        // chega em uma ilha
        ilha_destino = determinar_ilha_para_tripulacao();

        printf("Tripulacao do rei pirata %d se aproximando da ilha %d\n", id, ilha_destino);
        if (marinha_sob_controle[ilha_destino] == 0) {
            if(venceu_batalha[ilha_destino] != -1 && pthread_mutex_trylock(&mutex_ilhas[ilha_destino]) == 0 && marinha_sob_controle[ilha_destino] == 0) {
                pthread_mutex_lock(&mutex_vencedores[ilha_destino]);
                printf(COLOR_GREEN "\n\t\t\t\t\t\t\t\t\t\tTripulação do rei pirata %d entrou na ilha %d\n" COLOR_RESET, id, ilha_destino);
                sleep(TEMPO_MIN_USO);
                printf(COLOR_BRIGHT_RED "\t\t\t\t\t\t\t\t\t\tTripulação do rei pirata %d saiu da ilha %d\n" COLOR_RESET, id, ilha_destino);
                pthread_mutex_unlock(&mutex_vencedores[ilha_destino]);
                pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
            } else {
                // lista de espera para acessar a ilha
                pthread_mutex_lock(&mutex_desafiante[ilha_destino]);
                if (venceu_batalha[ilha_destino] != -1) {
                    // existe um vencedor no duelo da batalha pela ilha destino
                    pthread_mutex_unlock(&mutex_desafiante[ilha_destino]);
                    pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
                    sleep(2);
                } else {
                    // entra na lista de espera pelo duelo na ilha                    
                    vec_push(&batalha_na_ilha[ilha_destino], id);
                    while (batalha_na_ilha[ilha_destino].length != 0) {
                        // aguarda ate a batalha terminar
                        pthread_cond_wait(&ilha_em_batalha[ilha_destino], &mutex_desafiante[ilha_destino]);
                    }
                    pthread_mutex_unlock(&mutex_desafiante[ilha_destino]);
                    if (venceu_batalha[ilha_destino] == id) {
                        // venceu a batalha
                        pthread_mutex_lock(&mutex_ilhas[ilha_destino]);
                        pthread_mutex_lock(&mutex_vencedores[ilha_destino]);
                        printf(COLOR_BRIGHT_CYAN "\n\t\t\t\t\t\t\t\t\t\tTripulacao do rei pirata %d venceu a batalha pela ilha %d\n" COLOR_RESET, id, ilha_destino);
                        venceu_batalha[ilha_destino] = -1;
                        printf(COLOR_GREEN "\n\t\t\t\t\t\t\t\t\t\tTripulação do rei pirata %d entrou na ilha %d após vencer a batalha\n" COLOR_RESET, id, ilha_destino);
                        sleep(TEMPO_MIN_USO);
                        printf(COLOR_BRIGHT_RED "\t\t\t\t\t\t\t\t\t\tTripulação do rei pirata %d saiu depois de vencer, da ilha %d\n" COLOR_RESET, id, ilha_destino);
                        printf("\n\t\t\t\t\t\t\t\t\t\t ========================= \n");
                        pthread_mutex_unlock(&mutex_vencedores[ilha_destino]);
                        pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
                    } else {
                        // perdeu a batalha
                        sleep(2);
                    }
                }
            }
        }
        sleep(2);
    }

    pthread_exit(0);
}

// funcao que controla o comportamento da thread que controla a tripulacao da marinha
void* tripulacao_marinha(void *arg) {
    int id = *((int *)arg);
    int ilha_destino = 0;
    printf("Tripulação da marinha id: %d apareceu\n", id);
    while(TRUE) {
        printf("Tripulação da marinha id %d navegando\n", id);
        sleep(rand() % TEMPO_NAVEGACAO_MARINHA);

        // chega em uma ilha
        ilha_destino = determinar_ilha_para_tripulacao();

        pthread_mutex_lock(&mutex_ilhas[ilha_destino]);
        pthread_mutex_lock(&marinha_na_ilha[ilha_destino]);
        marinha_sob_controle[ilha_destino] = 1;
        printf(COLOR_BRIGHT_MAGENTA "Marinha %d está sob controle da ilha %d\n" COLOR_RESET, id, ilha_destino);
        printf(COLOR_BRIGHT_MAGENTA "Marinha %d evitará novas batalhas na ilha %d\n" COLOR_RESET, id, ilha_destino);
        sleep(TEMPO_MARINHA_ILHA);
        printf(COLOR_BRIGHT_MAGENTA "Marinha %d esta saindo da ilha %d\n" COLOR_RESET, id, ilha_destino);
        marinha_sob_controle[ilha_destino] = 0;
        pthread_mutex_unlock(&marinha_na_ilha[ilha_destino]);
        pthread_mutex_unlock(&mutex_ilhas[ilha_destino]);
    }

    pthread_exit(0);
}

int main () {
    int i, j, k,erro; // contadores de loop e variavel para detectar erro
    int *id;
    int forca_trip_piratas[QTD_TPIRATAS];
    int forca_trip_rei_piratas[QTD_REI_PIRATAS];
    pthread_t tp_threads[QTD_TPIRATAS]; // threads de tripulacoes piratas
    pthread_t rp_threads[QTD_REI_PIRATAS]; // threads de rei dos piratas
    pthread_t tm_threads[QTD_TMARINHEIROS]; // threads de tripulacoes de marinheiros
    pthread_t t_decide_batalhas[QTD_ILHAS]; // threads de tripulacoes de marinheiros

    vec_init(&batalha_na_ilha); // vetor dinamico que armazena tripulacoes que irao entrar em combate

    CLEAR
    printf(COLOR_BRIGHT_RED "\nIniciando vetores de mutex, vetores condicionais e estado das ilhas...\n" COLOR_RESET);

    // inicializa de vetor controlador de vencedores de batalhas
    for (i=0; i<QTD_ILHAS; i++) {
        venceu_batalha[i] = -1;
    }

    for (i=0; i<QTD_ILHAS; i++) {
        marinha_sob_controle[i] = 0;
    }
    
    // inicializa vetor de mutex controlador de acesso a ilha[x]
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_mutex_init(&mutex_ilhas[i], NULL);
    }
    
    // inicializa vetor de mutex que controla o vencedor da batalha na ilha [x]
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_mutex_init(&mutex_vencedores[i], NULL);
    }
    
    // inicializa vetor de mutex que controla se existe um desafiante esperando para batalhar por uma ilha
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_mutex_init(&mutex_desafiante[i], NULL);
    }
    
    // inicializa vetor de mutex que controla se existe uma tripulacao de marinha na ilha[x] 
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_mutex_init(&marinha_na_ilha[i], NULL);
    }
    
    // inicializa vetor de mutex que controla se a thread controladora de batalha esta ou nao printando alguma coisa
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_mutex_init(&mutex_printando_batalha[i], NULL);
    }

    // inicializa vetor de variaveis de condicao que coloca as threads para dormir quando existe uma batalha em andamento
    for (i=0; i<QTD_ILHAS; i++) {
        pthread_cond_init(&ilha_em_batalha[i], NULL);
    }

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


    printf(COLOR_BRIGHT_RED "\nIniciando mapa das tripulacoes\n" COLOR_RESET);
    j = 0;
    for (i=0; i<(QTD_TPIRATAS + QTD_REI_PIRATAS); i++) {
        if (i >= QTD_TPIRATAS) {
            mapa_das_tripulacoes[i].id = i;
            mapa_das_tripulacoes[i].forca = forca_trip_rei_piratas[j];
            mapa_das_tripulacoes[i].tipo_trip = REI_PIRATA;
            j++;
        }
        else{
            mapa_das_tripulacoes[i].id = i;
            mapa_das_tripulacoes[i].forca = forca_trip_piratas[i];
            mapa_das_tripulacoes[i].tipo_trip = TRIPULACAO_PIRATA;
        }
    }

    printf(COLOR_GREEN "\nInicializando...\n" COLOR_RESET);
    sleep(5);
    CLEAR

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
    j = i; // id dos rei piratas comeca a partir do ultimo dos
    k = 0;

    // Inicializa as threads dos reis dos piratas
    for (i = j; i < j + QTD_REI_PIRATAS; i++) {
        atributos_tripulacao *atributos = malloc(sizeof(atributos_tripulacao));
        atributos->id = i;
        atributos->forca = forca_trip_rei_piratas[k];
        atributos->tipo_trip = REI_PIRATA;
        erro = pthread_create(&rp_threads[i], NULL, rei_pirata, (void *)(atributos));
        if (erro) {
            printf("erro ao criar a Thread id %d\n", i);
            exit(1);
        }
        k++;
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

    // inicializa a thread de controladora de batalha
    for (i = 0; i < QTD_ILHAS; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&t_decide_batalhas[i], NULL, decide_batalhas, (void *)(id));
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

    for (i = 0; i < QTD_ILHAS; i++){ 
        pthread_join(t_decide_batalhas[i], NULL);
    }

    return 0;
}