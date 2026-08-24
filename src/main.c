#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "collectors.h"
#include "display.h"
#include "json_export.h"

int main(int argc, char *argv[]) {
    int json_mode = 0;

    // Tratamento de argumentos de linha de comando
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Uso: hardware_id [OPÇÃO]\n\n");
            printf("Coleta e exibe informações completas de hardware, DMI e sistema operacional.\n\n");
            printf("Opções:\n");
            printf("  -j, --json     Exporta os dados coletados em formato JSON estruturado\n");
            printf("  -h, --help     Exibe esta ajuda e sai\n");
            printf("  -v, --version  Exibe informações sobre a versão e sai\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("hardware_id v1.1.0\n");
            return 0;
        } else if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
            json_mode = 1;
        }
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

    // Coleta das informações
    collect_system_info(&sys_info);
    collect_cpu_info(&cpu_info);
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
                    &gpu_info, &disk_info, &mount_info, &net_info, &bt_info);
        return 0;
    }

    // Obter largura do terminal para modo TUI
    int width = get_terminal_width();

    // Exibição dos painéis visuais formatados
    display_header(width);
    display_hardware_id_panel(&hw_id_info, width);
    display_system_panel(&sys_info, width);
    display_cpu_panel(&cpu_info, width);
    display_memory_panel(&mem_info, width);
    display_gpu_panel(&gpu_info, width);
    display_storage_panel(&disk_info, &mount_info, width);
    display_network_panel(&net_info, &bt_info, width);
    display_footer(width);

    return 0;
}
