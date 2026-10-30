/*
 * json_export.c — Implementação da serialização JSON completa para hardware_id
 */

#include "json_export.h"
#include <string.h>

// Função auxiliar para imprimir strings escapando caracteres especiais para JSON
static void json_print_str(FILE *out, const char *key, const char *val, int comma) {
    fprintf(out, "    \"%s\": \"", key);
    if (val) {
        while (*val) {
            switch (*val) {
                case '"':  fputs("\\\"", out); break;
                case '\\': fputs("\\\\", out); break;
                case '\b': fputs("\\b", out); break;
                case '\f': fputs("\\f", out); break;
                case '\n': fputs("\\n", out); break;
                case '\r': fputs("\\r", out); break;
                case '\t': fputs("\\t", out); break;
                default:
                    if ((unsigned char)*val < 32) {
                        fprintf(out, "\\u%04x", (unsigned char)*val);
                    } else {
                        fputc(*val, out);
                    }
                    break;
            }
            val++;
        }
    }
    fprintf(out, "\"%s\n", comma ? "," : "");
}

static void json_print_identifier(FILE *out, const char *key, const char *val,
                                  int comma, int include_identifiers) {
    json_print_str(out, key, include_identifiers ? val : "<redacted>", comma);
}

void export_json(FILE *out,
                 const SysInfo *sys,
                 const HardwareIdInfo *hw_id,
                 const CpuInfo *cpu,
                 const MemInfo *mem,
                 const GpuInfo *gpu,
                 const DiskInfo *disks,
                 const MountInfo *mounts,
                 const NetInfo *net,
                 const BtInfo *bt,
                 int include_identifiers) {
    if (!out) return;

    fprintf(out, "{\n");

    /* 1. Sistema Operacional */
    fprintf(out, "  \"system\": {\n");
    json_print_str(out, "distro_name", sys->distro_name, 1);
    json_print_str(out, "distro_version", sys->distro_version, 1);
    json_print_str(out, "distro_codename", sys->distro_codename, 1);
    json_print_str(out, "kernel_version", sys->kernel_version, 1);
    json_print_str(out, "architecture", sys->architecture, 1);
    json_print_str(out, "hostname", sys->hostname, 1);
    json_print_str(out, "username", sys->username, 1);
    fprintf(out, "    \"uptime_seconds\": %ld,\n", sys->uptime_seconds);
    json_print_str(out, "uptime_formatted", sys->uptime_formatted, 1);
    json_print_str(out, "shell", sys->shell, 1);
    json_print_str(out, "compiler", sys->compiler, 1);
    json_print_str(out, "virtualization", sys->virtualization, 0);
    fprintf(out, "  },\n");

    /* 2. Identificação Única & Placa-Mãe */
    fprintf(out, "  \"hardware_id\": {\n");
    json_print_identifier(out, "fingerprint_sha256", hw_id->hardware_id, 1, include_identifiers);
    json_print_identifier(out, "machine_uuid", hw_id->formatted_uuid, 1, include_identifiers);
    json_print_identifier(out, "machine_id", hw_id->machine_id, 1, include_identifiers);
    json_print_str(out, "fingerprint_version", hw_id->fingerprint_version, 1);
    json_print_str(out, "system_vendor", hw_id->sys_vendor, 1);
    json_print_str(out, "product_name", hw_id->product_name, 1);
    json_print_str(out, "product_version", hw_id->product_version, 1);
    json_print_str(out, "board_vendor", hw_id->board_vendor, 1);
    json_print_str(out, "board_name", hw_id->board_name, 1);
    json_print_str(out, "bios_vendor", hw_id->bios_vendor, 1);
    json_print_str(out, "bios_version", hw_id->bios_version, 1);
    json_print_str(out, "bios_date", hw_id->bios_date, 0);
    fprintf(out, "  },\n");

    /* 3. Processador (CPU) */
    fprintf(out, "  \"cpu\": {\n");
    json_print_str(out, "model", cpu->model, 1);
    fprintf(out, "    \"physical_cores\": %d,\n", cpu->physical_cores);
    fprintf(out, "    \"logical_threads\": %d,\n", cpu->logical_threads);
    fprintf(out, "    \"freq_current_mhz\": %.2f,\n", cpu->freq_current_mhz);
    fprintf(out, "    \"freq_min_mhz\": %.2f,\n", cpu->freq_min_mhz);
    fprintf(out, "    \"freq_max_mhz\": %.2f,\n", cpu->freq_max_mhz);
    json_print_str(out, "cache_l1d", cpu->cache_l1d, 1);
    json_print_str(out, "cache_l1i", cpu->cache_l1i, 1);
    json_print_str(out, "cache_l2", cpu->cache_l2, 1);
    json_print_str(out, "cache_l3", cpu->cache_l3, 1);
    if (cpu->temp_celsius > 0.0) {
        fprintf(out, "    \"temp_celsius\": %.1f,\n", cpu->temp_celsius);
    } else {
        fprintf(out, "    \"temp_celsius\": null,\n");
    }
    fprintf(out, "    \"usage_total_percent\": %.2f,\n", cpu->usage_total);
    fprintf(out, "    \"per_core_usage_percent\": [");
    for (int i = 0; i < cpu->num_cores_sampled; i++) {
        fprintf(out, "%s%.2f", i ? ", " : "", cpu->usage_per_core[i]);
    }
    fprintf(out, "],\n");
    fprintf(out, "    \"per_core_usage_truncated\": %s\n", cpu->cores_truncated ? "true" : "false");
    fprintf(out, "  },\n");

    /* 4. Memória RAM & Swap */
    fprintf(out, "  \"memory\": {\n");
    fprintf(out, "    \"ram_total_kb\": %llu,\n", mem->total_kb);
    fprintf(out, "    \"ram_used_kb\": %llu,\n", mem->used_kb);
    fprintf(out, "    \"ram_free_kb\": %llu,\n", mem->free_kb);
    fprintf(out, "    \"ram_available_kb\": %llu,\n", mem->available_kb);
    fprintf(out, "    \"ram_buffers_kb\": %llu,\n", mem->buffers_kb);
    fprintf(out, "    \"ram_cached_kb\": %llu,\n", mem->cached_kb);
    fprintf(out, "    \"ram_usage_percent\": %.2f,\n", mem->ram_usage_percent);
    fprintf(out, "    \"swap_total_kb\": %llu,\n", mem->swap_total_kb);
    fprintf(out, "    \"swap_used_kb\": %llu,\n", mem->swap_used_kb);
    fprintf(out, "    \"swap_free_kb\": %llu,\n", mem->swap_free_kb);
    fprintf(out, "    \"swap_usage_percent\": %.2f,\n", mem->swap_usage_percent);
    fprintf(out, "    \"swap_devices\": [\n");
    for (int i = 0; i < mem->swap_count; i++) {
        fprintf(out, "      {\n");
        json_print_str(out, "device", mem->swaps[i].device, 1);
        json_print_str(out, "type", mem->swaps[i].type, 1);
        fprintf(out, "        \"size_kb\": %llu,\n", mem->swaps[i].size_kb);
        fprintf(out, "        \"used_kb\": %llu,\n", mem->swaps[i].used_kb);
        fprintf(out, "        \"priority\": %d\n", mem->swaps[i].priority);
        fprintf(out, "      }%s\n", (i < mem->swap_count - 1) ? "," : "");
    }
    fprintf(out, "    ],\n");
    fprintf(out, "    \"swap_devices_truncated\": %s\n", mem->swaps_truncated ? "true" : "false");
    fprintf(out, "  },\n");

    /* 5. Placas de Vídeo (GPU) */
    fprintf(out, "  \"gpu\": [\n");
    for (int i = 0; i < gpu->count; i++) {
        fprintf(out, "    {\n");
        json_print_str(out, "name", gpu->gpus[i].name, 1);
        json_print_str(out, "driver", gpu->gpus[i].driver, 1);
        json_print_str(out, "pci_slot", gpu->gpus[i].pci_slot, 0);
        fprintf(out, "    }%s\n", (i < gpu->count - 1) ? "," : "");
    }
    fprintf(out, "  ],\n");
    fprintf(out, "  \"gpu_truncated\": %s,\n", gpu->truncated ? "true" : "false");

    /* 6. Armazenamento (Discos e Montagens) */
    fprintf(out, "  \"storage\": {\n");
    fprintf(out, "    \"disks\": [\n");
    for (int i = 0; i < disks->count; i++) {
        fprintf(out, "      {\n");
        json_print_str(out, "device", disks->disks[i].device, 1);
        json_print_str(out, "model", disks->disks[i].model, 1);
        json_print_str(out, "type", disks->disks[i].type, 1);
        fprintf(out, "        \"size_bytes\": %llu\n", disks->disks[i].size_bytes);
        fprintf(out, "      }%s\n", (i < disks->count - 1) ? "," : "");
    }
    fprintf(out, "    ],\n");
    fprintf(out, "    \"disks_truncated\": %s,\n", disks->truncated ? "true" : "false");
    fprintf(out, "    \"mounts\": [\n");
    for (int i = 0; i < mounts->count; i++) {
        fprintf(out, "      {\n");
        json_print_str(out, "device", mounts->mounts[i].device, 1);
        json_print_str(out, "mount_point", mounts->mounts[i].mount_point, 1);
        json_print_str(out, "fs_type", mounts->mounts[i].fs_type, 1);
        fprintf(out, "        \"total_bytes\": %llu,\n", mounts->mounts[i].total_bytes);
        fprintf(out, "        \"used_bytes\": %llu,\n", mounts->mounts[i].used_bytes);
        fprintf(out, "        \"free_bytes\": %llu,\n", mounts->mounts[i].free_bytes);
        fprintf(out, "        \"usage_percent\": %.2f\n", mounts->mounts[i].usage_percent);
        fprintf(out, "      }%s\n", (i < mounts->count - 1) ? "," : "");
    }
    fprintf(out, "    ],\n");
    fprintf(out, "    \"mounts_truncated\": %s\n", mounts->truncated ? "true" : "false");
    fprintf(out, "  },\n");

    /* 7. Interfaces de Rede */
    fprintf(out, "  \"network\": [\n");
    for (int i = 0; i < net->count; i++) {
        fprintf(out, "    {\n");
        json_print_str(out, "name", net->interfaces[i].name, 1);
        json_print_str(out, "type", net->interfaces[i].type_desc, 1);
        json_print_str(out, "driver", net->interfaces[i].driver, 1);
        json_print_identifier(out, "mac", net->interfaces[i].mac, 1, include_identifiers);
        json_print_identifier(out, "ipv4", net->interfaces[i].ipv4, 1, include_identifiers);
        json_print_identifier(out, "ipv6", net->interfaces[i].ipv6, 1, include_identifiers);
        json_print_str(out, "state", net->interfaces[i].state, 1);
        fprintf(out, "      \"is_wireless\": %s,\n", net->interfaces[i].is_wireless ? "true" : "false");
        fprintf(out, "      \"is_virtual\": %s,\n", net->interfaces[i].is_virtual ? "true" : "false");
        fprintf(out, "      \"speed_mbps\": %llu\n", net->interfaces[i].speed_mbps);
        fprintf(out, "    }%s\n", (i < net->count - 1) ? "," : "");
    }
    fprintf(out, "  ],\n");
    fprintf(out, "  \"network_truncated\": %s,\n", net->truncated ? "true" : "false");

    /* 8. Bluetooth */
    fprintf(out, "  \"bluetooth\": [\n");
    for (int i = 0; i < bt->count; i++) {
        fprintf(out, "    {\n");
        json_print_str(out, "name", bt->adapters[i].name, 1);
        json_print_identifier(out, "address", bt->adapters[i].address, 1, include_identifiers);
        json_print_str(out, "type", bt->adapters[i].type_desc, 1);
        json_print_str(out, "driver", bt->adapters[i].driver, 1);
        json_print_str(out, "manufacturer", bt->adapters[i].manufacturer, 0);
        fprintf(out, "    }%s\n", (i < bt->count - 1) ? "," : "");
    }
    fprintf(out, "  ],\n");
    fprintf(out, "  \"bluetooth_truncated\": %s\n", bt->truncated ? "true" : "false");

    fprintf(out, "}\n");
}
