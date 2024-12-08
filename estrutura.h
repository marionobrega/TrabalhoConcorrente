#ifndef FUNCOES_H
#define FUNCOES_H

#include <pthread.h>
#include <ncurses.h>

// Definições e constantes
#define MAX_MODULOS 100
#define NUM_TEDAX 5
#define TAMANHO_BUFFER 10
#define MAX_FILA 10

// Estruturas de dados
typedef struct {
    char nome[50];
    int dificuldade;
    int tempo_desarme;
    int desarmado;
    int tedax_desarmando;
} Modulo;

typedef struct {
    char nome[50];
    int habilidade;
    int ocupado;
} Tedax;

typedef struct {
    int tedax_ids[MAX_FILA];
    int inicio;
    int fim;
} FilaEspera;

// Variáveis globais
extern Modulo lista_modulos[MAX_MODULOS];
extern int num_modulos;
extern Tedax lista_tedax[NUM_TEDAX];
extern int tempo_restante;
extern int jogo_ativo;
extern int resultado_desarme;
extern pthread_mutex_t mutex_lista_modulos;
extern pthread_mutex_t mutex_tedax;
extern pthread_cond_t cond_tedax;
extern pthread_mutex_t mutex_resultado;
extern pthread_cond_t cond_comando;
extern pthread_t thread_mural_id, thread_exibicao_id, thread_coordenador_id;
extern pthread_t threads_tedax[NUM_TEDAX];
extern int nivel_dificuldade;
extern char buffer_comandos[TAMANHO_BUFFER];
extern int posicao_buffer;
extern FilaEspera filas_espera[NUM_TEDAX];
extern int num_bancadas;
extern int bancadas[NUM_TEDAX];

// Funções

void inicializar_jogo();
void finalizar_jogo();
void *thread_mural(void *arg);
void *thread_exibicao(void *arg);
void *thread_tedax(void *arg);
void *thread_coordenador(void *arg);
void gerar_modulo();
void adicionar_tedax_na_fila(int bancada, int tedax_id);
int remover_tedax_da_fila(int bancada);
int gerar_numero_aleatorio(int min, int max);

#endif // FUNCOES_H
