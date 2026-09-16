#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "collectors.h"
#include "display.h"
#include "json_export.h"
#include "version.h"

int main(int argc, char *argv[]) {
    int json_mode = 0;
    int include_identifiers = 0;
    int color_mode = -1;

    // Tratamento de argumentos de linha de comando
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Uso: hardware_id [OPÇÃO]\n\n");
            printf("Coleta e exibe informações completas de hardware, DMI e sistema operacional.\n\n");
            printf("Opções:\n");
            printf("  -j, --json     Exporta os dados coletados em formato JSON estruturado\n");
            printf("      --include-identifiers  Inclui IDs, MACs e endereços IP sensíveis\n");
            printf("      --redact   Oculta identificadores (padrão)\n");
            printf("      --no-color Desativa cores ANSI no modo visual\n");
            printf("  -h, --help     Exibe esta ajuda e sai\n");
            printf("  -v, --version  Exibe informações sobre a versão e sai\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("hardware_id v%s\n", HARDWARE_ID_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
            json_mode = 1;
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
                    &gpu_info, &disk_info, &mount_info, &net_info, &bt_info,
                    include_identifiers);
        return 0;
    }

    // Obter largura do terminal para modo TUI
    if (color_mode < 0) color_mode = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
    set_color_enabled(color_mode);
    int width = get_terminal_width();

    // Exibição dos painéis visuais formatados
    display_header(width);
    display_hardware_id_panel(&hw_id_info, width, include_identifiers);
    display_system_panel(&sys_info, width);
    display_cpu_panel(&cpu_info, width);
    display_memory_panel(&mem_info, width);
    display_gpu_panel(&gpu_info, width);
    display_storage_panel(&disk_info, &mount_info, width);
    display_network_panel(&net_info, &bt_info, width, include_identifiers);
    display_footer(width);

    return 0;
}
