# hardware_id — Inspetor de Hardware & Gerador de Hardware Fingerprint para Linux

O **hardware_id** é uma ferramenta de linha de comando de alto desempenho desenvolvida em **C (C11 / POSIX)** para Linux. O utilitário realiza a inspeção profunda de componentes de hardware, identificação de placas-mãe/DMI, interfaces de conectividade (Rede/Bluetooth) e gera uma **impressão digital criptográfica única (Hardware Fingerprint / Machine UUID)** do computador.

Possui **zero dependências externas** (não necessita de `libpci`, `libudev` ou `openssl`), comunicando-se diretamente com o kernel através do `sysfs` (`/sys`), `procfs` (`/proc`) e chamadas de sistema POSIX.

---

## 🚀 Funcionalidades

- 🔑 **Identificação Única de Hardware (Hardware Fingerprint):**
  - Geração de **Hash SHA-256 único** baseado na combinação de CPU, Placa-Mãe, Machine ID e MAC Address.
  - Geração/Formatação de **Machine UUID** canônico (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).
  - Leitura do Machine ID do sistema (`/etc/machine-id` e `/var/lib/dbus/machine-id`).
  - Extração de dados de **Placa-Mãe e BIOS (DMI/SMBIOS)**: Fabricante, Modelo da Placa, Fabricante/Modelo do Sistema e Versão/Data da BIOS/UEFI.
-  **Sistema Operacional & Ambiente:**
  - Nome da Distribuição, Versão e Codename (via `/etc/os-release`).
  - Versão do Kernel Linux e Arquitetura do processador (`uname`).
  - Hostname, Usuário logado, Tempo de atividade (*Uptime*) formatado, Shell e versão do Compilador.
-  **Processador (CPU):**
  - Modelo comercial do processador.
  - Contagem de núcleos físicos e threads lógicos.
  - Frequências de operação em tempo real (Atual, Mínima e Máxima em MHz).
  - Tamanhos de Cache L1d, L1i, L2 e L3.
  - Utilização percentual total e **monitoramento individual por núcleo**.
-  **Memória RAM & Swap Inteligente:**
  - Métricas de RAM: Total, Usada, Livre, Disponível, Buffers e Cached.
  - Status geral de Swap com percentual de uso.
  - **Identificação de Dispositivos de Swap por Tipo:** Detecção automática de dispositivos `zram` (RAM compactada), `partition` (partição de swap) ou `file` (arquivo swap) com tamanho, uso e prioridade.
-  **Placas de Vídeo (GPU):**
  - Detecção via barramento PCI (NVIDIA, AMD/ATI, Intel, etc.).
  - Nome do dispositivo, Driver de vídeo em uso e Slot PCI.
-  **Armazenamento:**
  - **Dispositivos de Bloco Físicos:** Listagem de discos NVMe, SSDs e HDDs com modelo, tipo e capacidade (com filtro para não poluir com zram/loop).
  - **Sistemas de Arquivos Montados:** Pontos de montagem reais, tipo de FS (`ext4`, `vfat`, `btrfs`, `xfs`), espaço ocupado/total e barra gráfica de uso.
-  **Rede & Conectividade:**
  - Interfaces físicas (Ethernet, Wi-Fi), loopback, pontes (Bridges) e virtuais.
  - Endereço MAC de cada interface.
  - **Resolução de Endereços IP:** Extração nativa de IPv4 (com máscara CIDR) e IPv6 via `getifaddrs`.
  - Driver do controlador de rede em uso e velocidade do link (Mbps).
  - Agrupamento inteligente de interfaces virtuais (ex.: múltiplos containers Docker/veth).
-  **Bluetooth:**
  - Descoberta de controladores Bluetooth (`hci0`, etc.).
  - Endereço MAC Bluetooth, tipo de barramento (USB, UART, SDIO), driver e fabricante.
- 📄 **Exportação em JSON (`--json` / `-j`):**
  - Saída completa em formato JSON válido e estruturado para integração em scripts de automação, auditoria de inventário ou sistemas de licenciamento.

## Privacidade e estabilidade do fingerprint

Por padrão, o programa oculta identificadores correlacionáveis na saída: fingerprint, UUID DMI, Machine ID, MACs, IPs e endereço Bluetooth. Use `--include-identifiers` somente quando a coleta estiver em um canal confiável.

O fingerprint atual é `v2`: ele é derivado de dados de CPU/DMI e das MACs físicas ordenadas, sem incluir o `machine-id` da instalação. Assim, uma reinstalação do sistema não deve alterar o identificador. Ainda assim, IDs de hardware podem ser ausentes, alterados pelo firmware ou virtualizados; em contêineres e VMs o resultado pode refletir o host/hipervisor ou ser pouco distintivo.

---

## 🏗️ Como Compilar

### Requisitos
- Linux (x86_64, aarch64, armv7, etc.)
- GCC (ou Clang) com suporte a C11
- GNU Make

### 1. Compilação Otimizada (Release)
```bash
make
```
O executável binário `hardware_id` será gerado na raiz do projeto.

### 2. Compilação para Depuração (Debug com AddressSanitizer)
Para verificar vazamento de memória (*leaks*) ou corrupções de ponteiro:
```bash
make debug
```

### 3. Limpeza
```bash
make clean
```

### 4. Testes
```bash
make test
```
Inclui vetores conhecidos de SHA-256, validação de JSON e comportamento da CLI.

### 5. Instalação no Sistema (Opcional)
```bash
sudo make install
```
Instala o binário em `/usr/local/bin/hardware_id`. Para empacotamento ou instalação em staging, use `make PREFIX=/usr DESTDIR=/caminho/de/staging install`. A remoção usa `make uninstall` com os mesmos valores de `PREFIX` e `DESTDIR`.

---

## 🎮 Como Usar

### 1. Modo Visual no Terminal (TUI)
```bash
./hardware_id
```

### 2. Exportação em JSON
```bash
./hardware_id --json
# ou de forma abreviada:
./hardware_id -j
```

Exemplo de uso com `jq` ou redirecionamento:
```bash
./hardware_id -j | jq .hardware_id
```

Para incluir identificadores sensíveis deliberadamente:
```bash
./hardware_id --json --include-identifiers
```

No modo visual, as cores são ativadas apenas em terminal interativo. Use `--no-color` ou defina `NO_COLOR=1` para desativá-las explicitamente.

### 3. Linha de Comando e Ajuda
```bash
./hardware_id --help
```

---

## 🗂️ Estrutura do Projeto

```text
hardware_id/
├── Makefile             # Automação de compilação (Release, Debug/ASan, Clean, Install)
├── README.md            # Documentação técnica e guia do usuário
├── AGENTS.md            # Regras e limites de escopo do projeto
└── src/
    ├── main.c           # Ponto de entrada, tratamento de flags de linha de comando
    ├── collectors.h     # Definição das estruturas de dados e protótipos de coleta
    ├── collectors.c     # Extração de dados de /sys, /proc, DMI, rede e POSIX
    ├── display.h        # Protótipos de renderização de interface de terminal
    ├── display.c        # Formatação de caixas Unicode, cores ANSI e barras de progresso
    ├── sha256.h         # Header do algoritmo de hash SHA-256
    ├── sha256.c         # Implementação C11 pura do SHA-256 para Hardware Fingerprint
    ├── json_export.h    # Header para serialização JSON
    ├── json_export.c    # Serializador completo de todas as structs em JSON válido
    └── colors.h         # Definições de cores e formatações ANSI
```

---

## 🛡️ Segurança e Privilégios

- **Execução Sem Root (Modo Padrão):**
  - O programa foi projetado para funcionar perfeitamente com usuários não-privilegiados. Caso arquivos protegidos (como o UUID DMI nativo `/sys/class/dmi/id/product_uuid`) não estejam legíveis, o coletor realiza um *fallback* gracioso sintetizando o UUID a partir da combinação criptográfica dos identificadores públicos disponíveis.
- **Execução com Root (`sudo`):**
  - Com privilégios de administrador, o utilitário tem acesso direto aos números de série reais de chassi, placa-mãe e UUID de fábrica.

---

## 📄 Licença

Este projeto está licenciado sob os termos da licença MIT.
