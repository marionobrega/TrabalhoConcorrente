#include "funcoes.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Inicializa o jogo
    inicializar_jogo();

    // Aguarda o fim do jogo
    pthread_join(thread_coordenador_id, NULL);
    pthread_join(thread_mural_id, NULL);
    pthread_join(thread_exibicao_id, NULL);

    for (int i = 0; i < NUM_TEDAX; i++) {
        pthread_join(threads_tedax[i], NULL);
    }

    // Finaliza o jogo
    finalizar_jogo();

    printf("Fim do jogo!\n");
    return 0;
}
