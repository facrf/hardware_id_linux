# AGENTS.md — Regras e Diretrizes para Agentes de IA

Este documento estabelece as diretrizes operacionais, restrições de escopo e padrões de desenvolvimento para qualquer agente de inteligência artificial (incluindo Antigravity, Codex, Claude, ChatGPT ou agentes autônomos) que atue neste repositório.

---

## ⚠️ Regra Fundamental de Escopo e Zona de Trabalho

> [!IMPORTANT]
> **RESTRIÇÃO ESTRITA DE DIRETÓRIO**:
> Esta pasta e todas as suas subpastas compõem a **ÚNICA ZONA DE TRABALHO AUTORIZADA**.
> Qualquer agente operando neste projeto **DEVE ATUAR EXCLUSIVAMENTE** dentro do diretório raiz:
> ```
> /storage/www/projetos/utils/hardware_id/
> ```
> e em suas respectivas **subpastas** (ex.: `src/`, `tests/`, `docs/`, `bin/`, `build/`, `scratch/`, etc.).
> 
> - **NUNCA** crie, edite, mova, execute scripts ou exclua arquivos fora desta pasta raiz.
> - **NUNCA** execute comandos de gravação ou modificação de arquivos em `/tmp`, `/home`, `/var`, `/etc` ou qualquer outro diretório externo do sistema operacional.
> - Todos os arquivos temporários, scripts de teste ou artefatos transitórios devem ser alocados dentro de subdiretórios locais do projeto (ex.: `scratch/` ou `build/`).

---

## 📌 Visão Geral do Projeto

**`hardware_id`** é um projeto utilitário destinado à identificação única de máquinas e componentes de hardware (Motherboard UUID, CPU ID, MAC Address, Disk Serial, DMI/SMBIOS, Machine ID, etc.).

O objetivo central é fornecer rotinas confiáveis, portáteis e seguras para geração, validação e gerenciamento de impressões digitais de hardware (*hardware fingerprints* / IDs de máquina).

---

## 🏗️ Estrutura Recomendada do Repositório

Ao criar e expandir a base de código, mantenha uma estrutura clara e modular:

```
/storage/www/projetos/utils/hardware_id/
├── AGENTS.md             # Este arquivo (regras e limites da zona de trabalho)
├── README.md             # Documentação geral do projeto e guia de uso
├── .gitignore            # Regras de exclusão do Git
├── src/                  # Código-fonte principal da aplicação/biblioteca
├── tests/                # Suíte de testes unitários e de integração
├── docs/                 # Documentação técnica adicional
└── scratch/              # Arquivos temporários/testes locais (ignorado no Git)
```

---

## 🛠️ Diretrizes de Engenharia e Boas Práticas

1. **Segurança e Privilégios**:
   - Rotinas de leitura de hardware devem tratar graciosamente cenários com e sem privilégios de administrador/root.
   - Forneça fallbacks seguros e mensagens claras quando informações específicas de hardware não estiverem acessíveis.

2. **Portabilidade e Resiliência**:
   - Escreva código desacoplado que suporte diferentes distribuições Linux e arquiteturas.
   - Evite dependências desnecessárias ou bibliotecas externas pesadas; priorize interfaces nativas e bibliotecas padrão sempre que viável.

3. **Preservação e Limpeza de Código**:
   - Mantenha comentários explicativos, documentação de funções e tratamento rigoroso de exceções e erros.
   - Não versione arquivos de build, binários compilados ou dados temporários no controle de versão.

4. **Testes e Validação**:
   - Antes de concluir qualquer tarefa, valide as alterações executando testes locais na pasta do projeto.
   - Garanta que a execução não cause efeitos colaterais fora do escopo definido.

---

*Documento ativo para definição de zona de trabalho e controle de agentes.*
