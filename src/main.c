/**
 * Arquivo: main.c
 * Descrição: Ponto de entrada da aplicação hardware_id.
 * Este programa coleta e exibe informações detalhadas sobre o hardware e o sistema operacional.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "collectors.h"
#include "display.h"

int main(int argc, char *argv[]) {
    // Tratamento de argumentos de linha de comando
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("Uso: hardware_id [OPÇÃO]\n");
            printf("Coleta e exibe informações sobre o hardware e sistema.\n\n");
            printf("Opções:\n");
            printf("  --help     Exibe esta ajuda e sai\n");
            printf("  --version  Exibe informações sobre a versão e sai\n");
            return 0;
        } else if (strcmp(argv[1], "--version") == 0) {
            printf("hardware_id v1.0.0\n");
            return 0;
        }
    }

    // Declaração das estruturas para armazenar as informações
    SysInfo sys_info;
    CpuInfo cpu_info;
    MemInfo mem_info;
    GpuInfo gpu_info;
    DiskInfo disk_info;
    MountInfo mount_info;

    // Obter largura do terminal
    int width = get_terminal_width();

    // Coleta das informações
    collect_system_info(&sys_info);
    collect_cpu_info(&cpu_info);
    collect_memory_info(&mem_info);
    collect_gpu_info(&gpu_info);
    collect_disk_info(&disk_info);
    collect_mount_info(&mount_info);

    // Exibição dos painéis
    display_header(width);
    display_system_panel(&sys_info, width);
    display_cpu_panel(&cpu_info, width);
    display_memory_panel(&mem_info, width);
    display_gpu_panel(&gpu_info, width);
    display_storage_panel(&disk_info, &mount_info, width);
    display_footer(width);

    return 0;
}
