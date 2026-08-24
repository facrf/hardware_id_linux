# hardware_id

O **hardware_id** é uma ferramenta de linha de comando desenvolvida em C que coleta e exibe informações detalhadas sobre o hardware e o sistema operacional (Linux) em um formato amigável no terminal.

## 🚀 Funcionalidades

A ferramenta extrai os seguintes dados:
- **Sistema:** Distribuição, versão do kernel, arquitetura, tempo de atividade (uptime), shell e compilador padrão.
- **CPU:** Modelo, núcleos físicos, threads lógicos, frequências (atual, mínima, máxima), cache L1/L2/L3 e utilização de CPU atual.
- **Memória:** Memória RAM total, em uso, livre, buffers/cache e swap.
- **GPU (Vídeo):** Lista de placas de vídeo instaladas.
- **Armazenamento:** Discos físicos e pontos de montagem.

## 📸 Exemplo de Saída

*(Espaço reservado para screenshot. A saída possui painéis formatados delimitados por bordas indicando as seções: Sistema, CPU, Memória, Vídeo e Armazenamento)*

## 🛠️ Requisitos

- Sistema operacional Linux
- Compilador C (GCC >= 7 recomendado)
- GNU Make

## 🏗️ Como compilar (Build)

1. Clone o repositório ou obtenha os fontes.
2. Na raiz do projeto, execute o comando make para compilar o executável otimizado:
   ```bash
   make
   ```
3. O executável `hardware_id` será criado na pasta atual.

Para compilar em modo de depuração (debug, com AddressSanitizer):
```bash
make debug
```

Para limpar os arquivos de build:
```bash
make clean
```

Para compilação manual com GCC (caso não queira utilizar o Make):
```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE src/main.c src/collectors.c src/display.c -o hardware_id
```

## 🎮 Como Usar

Para executar a ferramenta e visualizar as informações do seu sistema:
```bash
./hardware_id
```

Outras opções disponíveis:
- Exibir a ajuda:
  ```bash
  ./hardware_id --help
  ```
- Exibir a versão:
  ```bash
  ./hardware_id --version
  ```

## 🗂️ Estrutura do Projeto

```text
hardware_id/
├── Makefile       # Script de compilação automatizada
├── README.md      # Documentação do projeto
└── src/           # Código fonte
    ├── collectors.c # Funções que extraem os dados de /proc, /sys e afins
    ├── collectors.h
    ├── display.c    # Funções de formatação e interface de terminal
    ├── display.h
    └── main.c       # Ponto de entrada do programa
```

## 📄 Licença

Este projeto está sob a licença MIT. Consulte o arquivo LICENSE para mais detalhes.

## ✍️ Autor

[Nome do Autor]
