# hardware_id — Inspetor de Hardware & Gerador de Hardware Fingerprint para Linux

O **hardware_id** é uma ferramenta de linha de comando de alto desempenho desenvolvida em **C (C11 / POSIX)** para Linux. O utilitário realiza a inspeção profunda de componentes de hardware, identificação de placas-mãe/DMI, interfaces de conectividade (Rede/Bluetooth) e gera uma **impressão digital criptográfica única (Hardware Fingerprint / Machine UUID)** do computador.

Possui **zero dependências externas** (não necessita de `libpci`, `libudev` ou `openssl`), comunicando-se diretamente com o kernel através do `sysfs` (`/sys`), `procfs` (`/proc`) e chamadas de sistema POSIX.

---

## 🚀 Funcionalidades

- 🔑 **Identificação Única de Hardware (Hardware Fingerprint):**
  - Geração de **Hash SHA-256 único** baseado na combinação de CPU, Placa-Mãe, Machine ID e MACs de interfaces físicas.
  - Geração/Formatação de **Machine UUID** canônico (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).
  - Saída direta instantânea para scripts com `--only-id` e `--uuid`.
  - Leitura do Machine ID do sistema (`/etc/machine-id` e `/var/lib/dbus/machine-id`).
  - Extração de dados de **Placa-Mãe e BIOS (DMI/SMBIOS)** com fallback automático para DeviceTree em arquiteturas ARM (ex.: Raspberry Pi).
-  **Sistema Operacional & Ambiente:**
  - **Detecção de Ambiente e Virtualização:** identifica se o sistema opera em Máquina Física (*Bare Metal*), Contêiner (Docker, Podman, LXC, Kubernetes) ou Máquina Virtual (KVM, VMware, VirtualBox, Hyper-V, WSL2, Xen, AWS EC2, GCP).
  - Nome da Distribuição, Versão e Codename (via `/etc/os-release` com fallback para `/usr/lib/os-release`).
  - Versão do Kernel Linux e Arquitetura do processador (`uname`).
  - Hostname, Usuário logado, Tempo de atividade (*Uptime*) formatado, Shell e versão do Compilador.
-  **Processador (CPU):**
  - Modelo comercial do processador.
  - **Sensor de Temperatura:** monitoramento térmico em tempo real (°C) via `/sys/class/thermal` e `hwmon`.
  - Contagem de núcleos físicos e threads lógicos.
  - Frequências de operação em tempo real (Atual, Mínima e Máxima em MHz, com fallback via `/proc/cpuinfo`).
  - Tamanhos de Cache L1d, L1i, L2 e L3.
  - Utilização percentual total e **monitoramento individual por núcleo** com visualização responsiva.
-  **Memória RAM & Swap Inteligente:**
  - Métricas de RAM: Total, Usada, Livre, Disponível, Buffers e Cached.
  - Formatação adaptativa com layout auto-ajustável para larguras de terminal padrão (80 colunas).
  - **Identificação de Dispositivos de Swap por Tipo:** Detecção automática de dispositivos `zram` (RAM compactada), `partition` (partição de swap) ou `file` (arquivo swap) com tamanho, uso e prioridade.
-  **Placas de Vídeo (GPU):**
  - Detecção via barramento PCI com suporte a classes de vídeo (`0x0300`, `0x0302`, `0x0380`).
  - Suporte a GPUs físicas (NVIDIA, AMD/ATI, Intel, Matrox) e adaptadores virtualizados (VirtIO, VMware SVGA, QEMU, Red Hat QXL, Hyper-V).
-  **Armazenamento:**
  - **Dispositivos de Bloco Físicos:** Listagem de discos NVMe, SSDs e HDDs com modelo, tipo e capacidade.
  - **Sistemas de Arquivos Montados:** Pontos de montagem reais e barra gráfica de uso, com **filtro inteligente de `/dev/loop` e Snaps (`squashfs`)** para manter a visualização limpa.
-  **Rede & Conectividade:**
  - **Priorização de Interfaces Físicas:** interfaces Ethernet e Wi-Fi são coletadas prioritariamente antes de pontes e contêineres Docker, prevenindo perda de adaptadores físicos em ambientes com dezenas de veths.
  - Endereço MAC de cada interface.
  - **Resolução de Endereços IP:** Extração nativa de IPv4 (com máscara CIDR) e IPv6 via `getifaddrs`.
  - Driver do controlador de rede em uso e velocidade do link (Mbps).
-  **Bluetooth:**
  - Descoberta de controladores Bluetooth (`hci0`, etc.), endereço MAC, barramento (USB, UART, SDIO) e fabricante.
- 📄 **Exportação em JSON (`--json` / `-j`):**
  - Saída completa em formato JSON válido e estruturado para automação, auditoria ou licenciamento.
- 📺 **Modo Monitor Interativo (`--watch` / `-w`):**
  - Dashboard em tempo real atualizando dinamicamente métricas de CPU, RAM e sistema a cada intervalo de segundos.

---

## 🛡️ Privacidade e Estabilidade do Fingerprint

Por padrão, o programa oculta identificadores correlacionáveis na saída: fingerprint, UUID DMI, Machine ID, MACs, IPs e endereço Bluetooth. Use `--include-identifiers` somente quando a coleta estiver em um canal confiável.

O fingerprint é derivado de dados de CPU/DMI e das MACs físicas ordenadas, sem incluir o `machine-id` da instalação, garantindo estabilidade mesmo após reinstalações do SO.

---

## 🏗️ Como Compilar

### Requisitos
- Linux (x86_64, aarch64, armv7, etc.)
- GCC (ou Clang) com suporte a C11
- GNU Make

### 1. Compilação Release
```bash
make
```
O executável binário `hardware_id` será gerado na raiz do projeto.

### 2. Compilação para Depuração (AddressSanitizer)
```bash
make debug
```

### 3. Testes Automatizados
```bash
make test
```
Executa a suíte de testes unitários de SHA-256, coletores, serialização JSON e validação de CLI.

### 4. Instalação no Sistema
```bash
sudo make install
```
Instala o binário em `/usr/local/bin/hardware_id` e a página de manual em `/usr/local/share/man/man1/hardware_id.1`.

---

## 🎮 Como Usar

### 1. Modo Visual no Terminal (TUI)
```bash
./hardware_id
```

### 2. Modo Monitor em Tempo Real (Dashboard)
```bash
# Atualiza a cada 2 segundos (padrão)
./hardware_id --watch

# Ou com intervalo customizado de 1 segundo:
./hardware_id -w 1
```

### 3. Extração Rápida para Scripts (Automação Shell)
```bash
# Retorna apenas o hash SHA-256 do hardware fingerprint:
HWID=$(./hardware_id --only-id)

# Retorna apenas o Machine UUID canônico:
UUID=$(./hardware_id --uuid)
```

### 4. Exportação de Variáveis de Ambiente Shell (`--env` / `-e`)
Ideal para scripts de provisionamento, Ansible, Docker ou carregamento dinâmico:
```bash
# Exporta variáveis no formato KEY="VALUE":
./hardware_id --env

# Carrega diretamente na sessão atual do shell:
eval $(./hardware_id -e --include-identifiers)
echo "Máquina: $HW_HOSTNAME | ID: $HW_ID | Gateway: $HW_DEFAULT_GATEWAY"
```

### 5. Modo Rápido (Skip de Amostragem de CPU)
```bash
# Executa em menos de 5ms pulando o sleep de 200ms da amostragem de CPU:
./hardware_id --fast --json
```

### 6. Modo ASCII Clássico (Servidores e Terminais Legados)
```bash
./hardware_id --ascii
```

### 7. Exportação Estruturada em JSON
```bash
# JSON padrão com identificadores mascarados:
./hardware_id --json

# JSON completo com identificadores sensíveis:
./hardware_id -j --include-identifiers
```

Exemplo com `jq`:
```bash
./hardware_id -j | jq '{ambiente: .system.virtualization, cpu: .cpu.model, temp: .cpu.temp_celsius, gateway: .network.default_gateway}'
```

### 8. Ajuda Completa
```bash
./hardware_id --help
```

---

## 🗂️ Estrutura do Projeto

```text
hardware_id/
├── Makefile             # Automação de compilação, testes e instalação
├── README.md            # Documentação técnica e guia do usuário
├── AGENTS.md            # Regras e limites de escopo do projeto
├── completions/
│   └── hardware_id.bash # Autocompletion para bash
├── docs/
│   └── hardware_id.1    # Página de manual (man page)
├── src/
│   ├── main.c           # Ponto de entrada, flags de linha de comando e modo watch
│   ├── collectors.h     # Definição das estruturas de dados e protótipos de coleta
│   ├── collectors.c     # Extração de /sys, /proc, DMI, rede, thermal e POSIX
│   ├── display.h        # Protótipos de renderização de terminal
│   ├── display.c        # Caixas Unicode/ASCII, cores ANSI e layout responsivo
│   ├── sha256.h         # Header do algoritmo de hash SHA-256
│   ├── sha256.c         # Implementação C11 pura do SHA-256
│   ├── json_export.h    # Header para serialização JSON
│   ├── json_export.c    # Serializador completo em JSON válido
│   ├── colors.h         # Definições de cores ANSI
│   └── version.h        # Definição da versão do projeto
└── tests/
    ├── test_cli.sh      # Testes de integração de CLI e flags
    ├── test_collectors.c# Testes dos utilitários de coleta
    ├── test_json.c      # Testes de serialização JSON
    └── test_sha256.c    # Testes com vetores oficiais SHA-256
```

---

## 📄 Licença

Este projeto está licenciado sob os termos da licença MIT.
