#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "collectors.h"
#include "display.h"
#include "json_export.h"
#include "env_export.h"
#include "version.h"

static volatile sig_atomic_t g_keep_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    g_keep_running = 0;
}

int main(int argc, char *argv[]) {
    int json_mode = 0;
    int env_mode = 0;
    int include_identifiers = 0;
    int color_mode = -1;
    int only_id_mode = 0;
    int uuid_mode = 0;
    int fast_mode = 0;
    int watch_mode = 0;
    int watch_interval = 2;

    // Tratamento de argumentos de linha de comando
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Uso: hardware_id [OPÇÃO]\n\n");
            printf("Coleta e exibe informações completas de hardware, DMI e sistema operacional.\n\n");
            printf("Opções de Saída:\n");
            printf("  -j, --json              Exporta os dados coletados em formato JSON estruturado\n");
            printf("  -e, --env               Exporta os dados como variáveis de ambiente (KEY=\"VALUE\")\n");
            printf("  -s, --only-id           Exibe apenas o hash SHA-256 do Hardware ID e sai\n");
            printf("      --uuid              Exibe apenas o Machine UUID canônico e sai\n");
            printf("  -f, --fast              Modo rápido: pula amostragem de uso de CPU (200ms)\n");
            printf("  -w, --watch [SEGUNDOS]  Modo monitor interativo com atualização periódica (padrão: 2s)\n\n");
            printf("Opções Visuais e de Privacidade:\n");
            printf("      --include-identifiers  Inclui IDs, MACs e endereços IP sensíveis\n");
            printf("      --redact               Oculta identificadores correlacionáveis (padrão)\n");
            printf("      --ascii                Usa caracteres ASCII padrão nas caixas (sem Unicode)\n");
            printf("      --no-color             Desativa cores ANSI no modo visual\n\n");
            printf("Ajuda e Versão:\n");
            printf("  -h, --help                 Exibe esta ajuda e sai\n");
            printf("  -v, --version              Exibe informações sobre a versão e sai\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("hardware_id v%s\n", HARDWARE_ID_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
            json_mode = 1;
        } else if (strcmp(argv[i], "--env") == 0 || strcmp(argv[i], "-e") == 0) {
            env_mode = 1;
            fast_mode = 1;
        } else if (strcmp(argv[i], "--only-id") == 0 || strcmp(argv[i], "-s") == 0) {
            only_id_mode = 1;
            fast_mode = 1;
        } else if (strcmp(argv[i], "--uuid") == 0) {
            uuid_mode = 1;
            fast_mode = 1;
        } else if (strcmp(argv[i], "--fast") == 0 || strcmp(argv[i], "-f") == 0) {
            fast_mode = 1;
        } else if (strcmp(argv[i], "--ascii") == 0) {
            set_ascii_mode(1);
        } else if (strcmp(argv[i], "--watch") == 0 || strcmp(argv[i], "-w") == 0) {
            watch_mode = 1;
            if (i + 1 < argc && argv[i + 1][0] >= '0' && argv[i + 1][0] <= '9') {
                watch_interval = atoi(argv[++i]);
                if (watch_interval < 1) watch_interval = 1;
            }
        } else if (strncmp(argv[i], "--watch=", 8) == 0) {
            watch_mode = 1;
            watch_interval = atoi(argv[i] + 8);
            if (watch_interval < 1) watch_interval = 1;
        } else if (strcmp(argv[i], "--include-identifiers") == 0) {
            include_identifiers = 1;
        } else if (strcmp(argv[i], "--redact") == 0) {
            include_identifiers = 0;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            color_mode = 0;
        } else {
            fprintf(stderr, "Opção desconhecida: %s\nUse --help para ver as opções disponíveis.\n", argv[i]);
            return 2;
        }
    }

    // Modo direto: imprimir apenas o Hardware ID e sair
    if (only_id_mode || uuid_mode) {
        CpuInfo cpu_info;
        NetInfo net_info;
        HardwareIdInfo hw_id_info;
        collect_cpu_info_ex(&cpu_info, 1);
        collect_net_info(&net_info);
        collect_hardware_id_info(&hw_id_info, &cpu_info, &net_info);
        if (only_id_mode) {
            printf("%s\n", hw_id_info.hardware_id);
        } else {
            printf("%s\n", hw_id_info.formatted_uuid);
        }
        return 0;
    }

    // Declaração das estruturas para armazenar as informações
    SysInfo sys_info;
    HardwareIdInfo hw_id_info;
    CpuInfo cpu_info;
    MemInfo mem_info;
    GpuInfo gpu_info;
    DiskInfo disk_info;
    MountInfo mount_info;
    NetInfo net_info;
    BtInfo bt_info;

    // Coleta inicial das informações
    collect_system_info(&sys_info);
    collect_cpu_info_ex(&cpu_info, fast_mode);
    collect_net_info(&net_info);
    collect_hardware_id_info(&hw_id_info, &cpu_info, &net_info);
    collect_memory_info(&mem_info);
    collect_gpu_info(&gpu_info);
    collect_disk_info(&disk_info);
    collect_mount_info(&mount_info);
    collect_bt_info(&bt_info);

    // Modo JSON
    if (json_mode) {
        export_json(stdout, &sys_info, &hw_id_info, &cpu_info, &mem_info,
                    &gpu_info, &disk_info, &mount_info, &net_info, &bt_info,
                    include_identifiers);
        return 0;
    }

    // Modo Variáveis de Ambiente (--env / -e)
    if (env_mode) {
        export_env(stdout, &sys_info, &hw_id_info, &cpu_info, &mem_info,
                   &net_info, include_identifiers);
        return 0;
    }

    // Configuração de cores e largura do terminal para modo TUI
    if (color_mode < 0) color_mode = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
    set_color_enabled(color_mode);

    // Modo Monitor Interativo (--watch)
    if (watch_mode) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = handle_sigint;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        // Oculta cursor no terminal interativo
        if (isatty(STDOUT_FILENO)) {
            printf("\033[?25l");
            fflush(stdout);
        }

        while (g_keep_running) {
            int width = get_terminal_width();
            printf("\033[H\033[2J"); // Limpa tela e posiciona cursor no início

            display_header(width);
            display_hardware_id_panel(&hw_id_info, width, include_identifiers);
            display_system_panel(&sys_info, width);
            display_cpu_panel(&cpu_info, width);
            display_memory_panel(&mem_info, width);
            display_gpu_panel(&gpu_info, width);
            display_storage_panel(&disk_info, &mount_info, width, include_identifiers);
            display_network_panel(&net_info, &bt_info, width, include_identifiers);
            display_footer(width);
            fflush(stdout);

            // Intervalo de atualização sensível a interrupções (verifica a cada 100ms)
            for (int step = 0; step < watch_interval * 10 && g_keep_running; step++) {
                usleep(100000);
            }

            if (!g_keep_running) break;

            // Atualiza métricas dinâmicas para o próximo ciclo
            collect_system_info(&sys_info);
            collect_cpu_info_ex(&cpu_info, 0);
            collect_memory_info(&mem_info);
            collect_mount_info(&mount_info);
            collect_net_info(&net_info);
        }

        // Restaura cursor
        if (isatty(STDOUT_FILENO)) {
            printf("\033[?25h\n");
            fflush(stdout);
        }
        return 0;
    }

    // Exibição única dos painéis visuais formatados
    int width = get_terminal_width();
    display_header(width);
    display_hardware_id_panel(&hw_id_info, width, include_identifiers);
    display_system_panel(&sys_info, width);
    display_cpu_panel(&cpu_info, width);
    display_memory_panel(&mem_info, width);
    display_gpu_panel(&gpu_info, width);
    display_storage_panel(&disk_info, &mount_info, width, include_identifiers);
    display_network_panel(&net_info, &bt_info, width, include_identifiers);
    display_footer(width);

    return 0;
}
