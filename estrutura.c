#include "estrutura.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Variáveis globais (protegidas por mutexes)
Modulo lista_modulos[MAX_MODULOS];
int num_modulos = 0;
Tedax lista_tedax[NUM_TEDAX];
int tempo_restante = 300; // 5 minutos em segundos
int jogo_ativo = 1;
int resultado_desarme = -1;
pthread_mutex_t mutex_lista_modulos;
pthread_mutex_t mutex_tedax;
pthread_cond_t cond_tedax;
pthread_mutex_t mutex_resultado;
pthread_t thread_mural_id, thread_exibicao_id, thread_coordenador_id;
pthread_t threads_tedax[NUM_TEDAX];
int nivel_dificuldade = 1;
char buffer_comandos[TAMANHO_BUFFER];
int posicao_buffer = 0;
pthread_cond_t cond_comando; // Variável de condição para sinalizar novo comando
FilaEspera filas_espera[NUM_TEDAX]; // Array de filas de espera
int num_bancadas = NUM_TEDAX; // Número de bancadas, igual ao número de Tedax por enquanto
int bancadas[NUM_TEDAX]; // Array para representar as bancadas

// Função para gerar um número aleatório dentro de um intervalo
int gerar_numero_aleatorio(int min, int max) {
    return min + rand() % (max - min + 1);
}

void inicializar_jogo() {
    // Inicializa a seed para números aleatórios
    srand(time(NULL));

    // Inicializa ncurses
    initscr();
    raw(); // Ativa o modo raw
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE); // Habilita o teclado

    // Inicializa mutexes e variáveis de condição
    pthread_mutex_init(&mutex_lista_modulos, NULL);
    pthread_mutex_init(&mutex_tedax, NULL);
    pthread_cond_init(&cond_tedax, NULL);
    pthread_mutex_init(&mutex_resultado, NULL);
    pthread_cond_init(&cond_comando, NULL); // Inicializa a nova variável de condição

    // Ajusta o tempo limite de acordo com o nível de dificuldade
    tempo_restante = 300 / nivel_dificuldade;

    // Inicializa os Tedax
    for (int i = 0; i < NUM_TEDAX; i++) {
        sprintf(lista_tedax[i].nome, "Tedax %d", i + 1);
        lista_tedax[i].habilidade = gerar_numero_aleatorio(1, 5);
        lista_tedax[i].ocupado = 0;
    }

    // Inicializa as filas de espera
    for (int i = 0; i < num_bancadas; i++) {
        filas_espera[i].inicio = 0;
        filas_espera[i].fim = 0;
    }

    // Inicializa as bancadas como livres
    for (int i = 0; i < NUM_TEDAX; i++) {
        bancadas[i] = 0; // 0 indica que a bancada está livre
    }

    // Cria threads
    pthread_create(&thread_mural_id, NULL, thread_mural, NULL);
    pthread_create(&thread_exibicao_id, NULL, thread_exibicao, NULL);
    pthread_create(&thread_coordenador_id, NULL, thread_coordenador, NULL);

    // Cria threads para os Tedax
    for (int i = 0; i < NUM_TEDAX; i++) {
        pthread_create(&threads_tedax[i], NULL, thread_tedax, &lista_tedax[i]);
    }
}

void finalizar_jogo() {
    // Encerra threads
    jogo_ativo = 0;
    pthread_cond_broadcast(&cond_tedax); // Acorda todas as threads Tedax
    pthread_cond_broadcast(&cond_comando); // Acorda a thread de exibição

    // Destroi mutexes e variáveis de condição
    pthread_mutex_destroy(&mutex_lista_modulos);
    pthread_mutex_destroy(&mutex_tedax);
    pthread_cond_destroy(&cond_tedax);
    pthread_mutex_destroy(&mutex_resultado);
    pthread_cond_destroy(&cond_comando); // Destroi a nova variável de condição

    // Desativa o modo raw
    noraw();

    // Encerra ncurses
    endwin();
}

void *thread_mural(void *arg) {
    while (jogo_ativo) {
        // Ajusta a frequência de geração de módulos de acordo com o nível de dificuldade
        int tempo_espera = 2 / nivel_dificuldade;

        // Gera um novo módulo
        gerar_modulo();
        sleep(tempo_espera);
    }
    pthread_exit(NULL);
}

void *thread_exibicao(void *arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_lista_modulos);

        // Limpa a tela e inicia o processo de atualização
        clear();

        // Exibe o tempo restante
        mvprintw(1, 1, "Tempo restante: %d segundos", tempo_restante);

        // Exibe os módulos pendentes
        mvprintw(3, 1, "Modulos pendentes (max: 10):");
        int linha_modulo = 4;

        // Exibe no máximo 10 módulos
        int max_modulos = num_modulos > 10 ? 10 : num_modulos;
        for (int i = 0; i < max_modulos; i++) {
            if (!lista_modulos[i].desarmado) {
                mvprintw(linha_modulo++, 1, "%d - %s (dificuldade: %d)", 
                         i + 1, lista_modulos[i].nome, lista_modulos[i].dificuldade);
            }
        }

        // Exibe os Tedax
        int linha_tedax = linha_modulo + 2;
        mvprintw(linha_tedax++, 1, "Tedax:");
        for (int i = 0; i < NUM_TEDAX; i++) {
            mvprintw(linha_tedax++, 1, "- %s (habilidade: %d, %s)", 
                     lista_tedax[i].nome, 
                     lista_tedax[i].habilidade, 
                     lista_tedax[i].ocupado ? "ocupado" : "livre");
        }

        // Exibe o estado das bancadas
        int linha_bancadas = linha_tedax + 1;
        mvprintw(linha_bancadas++, 1, "Bancadas:");
        for (int i = 0; i < num_bancadas; i++) {
            mvprintw(linha_bancadas++, 1, "Bancada %d: %s", 
                     i + 1, bancadas[i] ? "ocupada" : "livre");
        }

        // Exibe mensagens de resultado do desarme
        int linha_mensagem = linha_bancadas + 1;
        if (resultado_desarme == 1) {
            mvprintw(linha_mensagem++, 1, "Modulo desarmado com sucesso!");
            resultado_desarme = -1;
        } else if (resultado_desarme == 0) {
            mvprintw(linha_mensagem++, 1, "Falha ao desarmar o modulo!");
            resultado_desarme = -1;
        }

        // Informa ao jogador que pode pressionar 'q' para sair
        mvprintw(linha_mensagem + 2, 1, "Pressione 'q' para sair.");

        // Atualiza a tela
        refresh();

        pthread_mutex_unlock(&mutex_lista_modulos);

        // Intervalo curto de atualização
        usleep(50000); // Atualiza a interface a cada 50ms
    }
    pthread_exit(NULL);
}

void *thread_tedax(void *arg) {
    Tedax *meu_tedax = (Tedax *)arg;
    int meu_id = meu_tedax - lista_tedax; // Calcula o ID do Tedax

    while (jogo_ativo) {
        // Aguarda sinal para iniciar o desarme
        pthread_mutex_lock(&mutex_tedax);
        pthread_cond_wait(&cond_tedax, &mutex_tedax);
        pthread_mutex_unlock(&mutex_tedax);

        if (!jogo_ativo) {
            break; // Sai do loop se o jogo terminou
        }

        pthread_mutex_lock(&mutex_lista_modulos);
        int modulo_a_desarmar = -1;
        for (int i = 0; i < num_modulos; i++) {
            if (lista_modulos[i].tedax_desarmando == meu_id) {
                modulo_a_desarmar = i;
                break;
            }
        }

        if (modulo_a_desarmar != -1) {
            // Procura por uma bancada livre
            int bancada_livre = -1;
            for (int i = 0; i < num_bancadas; i++) {
                if (bancadas[i] == 0) {
                    bancada_livre = i;
                    break;
                }
            }

            if (bancada_livre != -1) {
                // Ocupa a bancada livre
                bancadas[bancada_livre] = 1;

                meu_tedax->ocupado = 1;
                pthread_mutex_unlock(&mutex_lista_modulos);

                // Simula o tempo de desarme do módulo
                sleep(lista_modulos[modulo_a_desarmar].tempo_desarme);

                // Determina o resultado do desarme (exemplo: 50% de chance de sucesso)
                int resultado = gerar_numero_aleatorio(0, 1);

                pthread_mutex_lock(&mutex_resultado);
                resultado_desarme = resultado;
                pthread_mutex_unlock(&mutex_resultado);

                pthread_mutex_lock(&mutex_lista_modulos);
                if (resultado == 1) {
                    lista_modulos[modulo_a_desarmar].desarmado = 1;
                }
                lista_tedax[meu_id].ocupado = 0;
                lista_modulos[modulo_a_desarmar].tedax_desarmando = -1;

                // Libera a bancada
                bancadas[bancada_livre] = 0;
                pthread_mutex_unlock(&mutex_lista_modulos);

                pthread_cond_signal(&cond_comando); // Sinaliza para a thread de exibição atualizar a tela
            } else {
                // Nenhuma bancada livre disponível
                pthread_mutex_unlock(&mutex_lista_modulos);
            }
        } else {
            pthread_mutex_unlock(&mutex_lista_modulos);
        }
    }

    pthread_exit(NULL);
}

void *thread_coordenador(void *arg) {
    while (jogo_ativo) {
        int tecla;

        // Aguarda por uma tecla válida
        tecla = getch();

        // Verifica se o jogador pressionou a tecla 'q' para sair
        if (tecla == 'q' || tecla == 'Q') {
            jogo_ativo = 0; // Encerra o jogo
            pthread_cond_broadcast(&cond_tedax); // Acorda todas as threads para encerrar
            pthread_cond_broadcast(&cond_comando); // Atualiza a tela antes de sair
            break;
        }

        // Verifica se é um comando válido
        if (tecla >= '1' && tecla <= '9') {
            buffer_comandos[posicao_buffer++] = tecla;
            buffer_comandos[posicao_buffer] = '\0';

            if (posicao_buffer == 2) {
                int tedax_escolhido = buffer_comandos[0] - '0';
                int modulo_a_desarmar = buffer_comandos[1] - '0';

                pthread_mutex_lock(&mutex_lista_modulos);
                if (tedax_escolhido >= 1 && tedax_escolhido <= NUM_TEDAX &&
                    modulo_a_desarmar >= 1 && modulo_a_desarmar <= num_modulos) {

                    if (!lista_tedax[tedax_escolhido - 1].ocupado &&
                        !lista_modulos[modulo_a_desarmar - 1].desarmado &&
                        lista_modulos[modulo_a_desarmar - 1].tedax_desarmando == -1) {

                        lista_tedax[tedax_escolhido - 1].ocupado = 1;
                        lista_modulos[modulo_a_desarmar - 1].tedax_desarmando = tedax_escolhido - 1;

                        adicionar_tedax_na_fila(modulo_a_desarmar % num_bancadas, tedax_escolhido - 1);
                        pthread_cond_signal(&cond_tedax);
                    }
                }
                pthread_mutex_unlock(&mutex_lista_modulos);
                posicao_buffer = 0; // Limpa o buffer após processar
            } else if (posicao_buffer >= TAMANHO_BUFFER) {
                posicao_buffer = 0; // Evita estouro de buffer
            }
        }
    }
    pthread_exit(NULL);
}


void gerar_modulo() {
    pthread_mutex_lock(&mutex_lista_modulos);
    if (num_modulos < MAX_MODULOS) {
        // Gera um novo módulo com nome e dificuldade aleatórios
        sprintf(lista_modulos[num_modulos].nome, "Modulo %d", num_modulos + 1);

        // Ajusta a dificuldade do módulo de acordo com o nível (opcional)
        lista_modulos[num_modulos].dificuldade = gerar_numero_aleatorio(1, 5) + nivel_dificuldade;

        lista_modulos[num_modulos].tempo_desarme = gerar_numero_aleatorio(5, 15);
        lista_modulos[num_modulos].desarmado = 0; // Inicialmente, não desarmado
        lista_modulos[num_modulos].tedax_desarmando = -1; // Inicializa com nenhum Tedax desarmando
        num_modulos++;
    }
    pthread_mutex_unlock(&mutex_lista_modulos);
}

void adicionar_tedax_na_fila(int bancada, int tedax_id) {
    filas_espera[bancada].tedax_ids[filas_espera[bancada].fim] = tedax_id;
    filas_espera[bancada].fim = (filas_espera[bancada].fim + 1) % MAX_FILA;
}

int remover_tedax_da_fila(int bancada) {
    int tedax_id = filas_espera[bancada].tedax_ids[filas_espera[bancada].inicio];
    filas_espera[bancada].inicio = (filas_espera[bancada].inicio + 1) % MAX_FILA;
    return tedax_id;
}