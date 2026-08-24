# Variáveis de compilação
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wno-format-truncation -O2 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

# Diretórios
SRC_DIR = src
BUILD_DIR = build

# Nome do executável
TARGET = hardware_id

# Fontes e objetos
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/collectors.c $(SRC_DIR)/display.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Alvo padrão
.PHONY: all
all: $(BUILD_DIR) $(TARGET)

# Cria o diretório de build se não existir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Regra para compilar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regra para compilar os objetos
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpeza
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Regra para compilar com opções de debug
.PHONY: debug
debug: CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -O0 -fsanitize=address -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
debug: clean all

# Regra para instalação
.PHONY: install
install: all
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
