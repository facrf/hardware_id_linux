/*
 * env_export.c — Implementação da exportação de variáveis de ambiente para hardware_id
 */

#include "env_export.h"
#include <string.h>

static void env_print_str(FILE *out, const char *key, const char *val) {
    fprintf(out, "%s=\"", key);
    if (val) {
        while (*val) {
            if (*val == '"' || *val == '\\' || *val == '$' || *val == '`') {
                fputc('\\', out);
            }
            fputc(*val, out);
            val++;
        }
    }
    fprintf(out, "\"\n");
}

static void env_print_identifier(FILE *out, const char *key, const char *val, int include_identifiers) {
    env_print_str(out, key, include_identifiers ? val : "<redacted>");
}

void export_env(FILE *out,
                const SysInfo *sys,
                const HardwareIdInfo *hw_id,
                const CpuInfo *cpu,
                const MemInfo *mem,
                const NetInfo *net,
                int include_identifiers) {
    if (!out) return;

    /* 1. Identificadores */
    env_print_identifier(out, "HW_ID", hw_id->hardware_id, include_identifiers);
    env_print_identifier(out, "HW_UUID", hw_id->formatted_uuid, include_identifiers);
    env_print_identifier(out, "HW_MACHINE_ID", hw_id->machine_id, include_identifiers);
    env_print_str(out, "HW_FINGERPRINT_VERSION", hw_id->fingerprint_version);

    /* 2. Sistema Operacional */
    env_print_str(out, "HW_DISTRO_NAME", sys->distro_name);
    env_print_str(out, "HW_DISTRO_VERSION", sys->distro_version);
    env_print_str(out, "HW_DISTRO_CODENAME", sys->distro_codename);
    env_print_str(out, "HW_KERNEL", sys->kernel_version);
    env_print_str(out, "HW_ARCH", sys->architecture);
    env_print_str(out, "HW_HOSTNAME", sys->hostname);
    env_print_str(out, "HW_USERNAME", sys->username);
    fprintf(out, "HW_UPTIME_SECONDS=%ld\n", sys->uptime_seconds);
    env_print_str(out, "HW_VIRTUALIZATION", sys->virtualization);

    /* 3. Processador (CPU) */
    env_print_str(out, "HW_CPU_MODEL", cpu->model);
    fprintf(out, "HW_CPU_PHYSICAL_CORES=%d\n", cpu->physical_cores);
    fprintf(out, "HW_CPU_LOGICAL_THREADS=%d\n", cpu->logical_threads);
    fprintf(out, "HW_CPU_FREQ_CURRENT_MHZ=%.2f\n", cpu->freq_current_mhz);
    if (cpu->temp_celsius > 0.0) {
        fprintf(out, "HW_CPU_TEMP_CELSIUS=%.1f\n", cpu->temp_celsius);
    }

    /* 4. Memória RAM e Swap */
    fprintf(out, "HW_RAM_TOTAL_KB=%llu\n", mem->total_kb);
    fprintf(out, "HW_RAM_USED_KB=%llu\n", mem->used_kb);
    fprintf(out, "HW_RAM_FREE_KB=%llu\n", mem->free_kb);
    fprintf(out, "HW_RAM_AVAILABLE_KB=%llu\n", mem->available_kb);
    fprintf(out, "HW_RAM_USAGE_PERCENT=%.2f\n", mem->ram_usage_percent);
    fprintf(out, "HW_SWAP_TOTAL_KB=%llu\n", mem->swap_total_kb);
    fprintf(out, "HW_SWAP_USED_KB=%llu\n", mem->swap_used_kb);

    /* 5. Rota e Conectividade */
    if (net && net->default_iface[0]) {
        env_print_str(out, "HW_DEFAULT_INTERFACE", net->default_iface);
    }
    if (net && net->default_gateway[0]) {
        env_print_identifier(out, "HW_DEFAULT_GATEWAY", net->default_gateway, include_identifiers);
    }
}
