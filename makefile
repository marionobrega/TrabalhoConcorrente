# Nome do executável
EXEC = jogo

# Compilador
CC = gcc

# Flags do compilador
CFLAGS = -Wall -pthread -lncurses

# Arquivos fonte
SRCS = main.c estrutura.c

# Arquivos objeto
OBJS = $(SRCS:.c=.o)

# Regra padrão
all: $(EXEC)

# Regra para criar o executável
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(CFLAGS)

# Regra para compilar os arquivos .c em .o
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Regra para limpar os arquivos gerados
clean:
	rm -f $(OBJS) $(EXEC)

# Regra para executar o programa
run: $(EXEC)
	./$(EXEC)

# Regra para depuração
debug: CFLAGS += -g
debug: clean $(EXEC)
