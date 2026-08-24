#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "display.h"
#include "colors.h"

// Função auxiliar para contar o tamanho visível (ignora sequências ANSI e calcula caracteres UTF-8)
static int visible_len(const char *str) {
    int len = 0;
    int in_ansi = 0;
    while (*str) {
        if (*str == '\033') {
            in_ansi = 1;
        } else if (in_ansi) {
            if (*str == 'm') {
                in_ansi = 0;
            }
        } else {
            // Conta caracteres UTF-8 (ignora bytes de continuação 10xxxxxx)
            if ((*str & 0xC0) != 0x80) {
                len++;
            }
        }
        str++;
    }
    return len;
}

int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 80;
    }
    int width = w.ws_col;
    if (width < 60) width = 60;
    if (width > 120) width = 120;
    return width;
}

const char *usage_color(double percent) {
    if (percent < 60.0) return GREEN;
    if (percent < 85.0) return YELLOW;
    return RED;
}

void draw_box_top(const char *title, int width) {
    int title_len = visible_len(title);
    int used_len = 5 + title_len + 1; // "╭─── " (5) + título + " " (1)
    int remain = width - used_len - 1; // -1 para "╮"
    if (remain < 0) remain = 0;
    
    printf("%s╭─── %s%s%s%s%s %s", BCYAN, RST, BYELLOW, BOLD, title, RST, BCYAN);
    for (int i = 0; i < remain; i++) {
        printf("─");
    }
    printf("╮%s\n", RST);
}

void draw_box_bottom(int width) {
    printf("%s╰", BCYAN);
    for (int i = 0; i < width - 2; i++) {
        printf("─");
    }
    printf("╯%s\n", RST);
}

void draw_separator(int width) {
    printf("%s├", DIM BCYAN);
    for (int i = 0; i < width - 2; i++) {
        printf("─");
    }
    printf("┤%s\n", RST);
}

void draw_kv(const char *key, const char *value, int width) {
    char padded_key[32];
    snprintf(padded_key, sizeof(padded_key), "%-18s", key);
    
    int val_len = visible_len(value);
    int used_len = 3 + 18 + 3 + val_len; // "│  " (3) + key (18) + " : " (3) + value
    int pad_len = width - used_len - 2; // -2 para " │"
    if (pad_len < 0) pad_len = 0;
    
    printf("%s│  %s%s%s%s : %s%s%s", BCYAN, RST, BYELLOW, padded_key, RST, BWHITE, value, RST);
    for (int i = 0; i < pad_len; i++) {
        putchar(' ');
    }
    printf("%s │%s\n", BCYAN, RST);
}

void draw_progress_bar(const char *label, double percent, int width) {
    if (percent < 0.0) percent = 0.0;
    if (percent > 100.0) percent = 100.0;
    
    int bar_width = width - 40;
    if (bar_width < 10) bar_width = 10;
    
    int filled = (int)((percent / 100.0) * bar_width);
    if (filled > bar_width) filled = bar_width;
    if (filled < 0) filled = 0;
    int empty = bar_width - filled;
    
    char padded_label[32];
    snprintf(padded_label, sizeof(padded_label), "%-18s", label);
    
    const char *color = usage_color(percent);
    
    printf("%s│  %s%s%s%s : %s[", BCYAN, RST, BYELLOW, padded_label, RST, BCYAN);
    
    printf("%s", color);
    for (int i = 0; i < filled; i++) printf("█");
    
    printf("%s%s", RST, DIM);
    for (int i = 0; i < empty; i++) printf("░");
    
    printf("%s%s] %s%5.1f%%%s", RST, BCYAN, color, percent, RST);
    
    int used_len = 3 + 18 + 3 + 1 + bar_width + 2 + 6;
    int remain = width - used_len - 2; // -2 para " │"
    if (remain < 0) remain = 0;
    
    for (int i = 0; i < remain; i++) printf(" ");
    printf("%s │%s\n", BCYAN, RST);
}

void display_header(int width) {
    printf("\n");
    printf("%s╭", BCYAN);
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("╮%s\n", RST);
    
    const char *title = "HARDWARE ID — Inspetor de Sistema Linux";
    int t_len = visible_len(title);
    int pad_left = (width - 2 - t_len) / 2;
    int pad_right = width - 2 - t_len - pad_left;
    
    printf("%s│%s", BCYAN, RST);
    for (int i = 0; i < pad_left; i++) printf(" ");
    printf("%s%s%s%s", BWHITE, BOLD, title, RST);
    for (int i = 0; i < pad_right; i++) printf(" ");
    printf("%s│%s\n", BCYAN, RST);
    
    draw_box_bottom(width);
}

void display_hardware_id_panel(const HardwareIdInfo *info, int width) {
    draw_box_top("🔑 Identificação Única & Placa-Mãe (DMI)", width);

    draw_kv("Hardware ID", info->hardware_id, width);
    draw_kv("Machine UUID", info->formatted_uuid, width);
    draw_kv("Machine ID", info->machine_id, width);

    char mobo[256];
    snprintf(mobo, sizeof(mobo), "%s %s", info->board_vendor, info->board_name);
    draw_kv("Placa-Mãe", mobo, width);

    char sys_prod[256];
    snprintf(sys_prod, sizeof(sys_prod), "%s %s (%s)", info->sys_vendor, info->product_name, info->product_version);
    draw_kv("Sistema/Chassis", sys_prod, width);

    char bios[256];
    snprintf(bios, sizeof(bios), "%s v%s (%s)", info->bios_vendor, info->bios_version, info->bios_date);
    draw_kv("BIOS / UEFI", bios, width);

    draw_box_bottom(width);
}

void display_system_panel(const SysInfo *info, int width) {
    draw_box_top(" Sistema Operacional", width);
    
    char distro[256];
    snprintf(distro, sizeof(distro), "%s %s (%s)", info->distro_name, info->distro_version, info->distro_codename);
    draw_kv("Distribuição", distro, width);
    
    char kernel[256];
    snprintf(kernel, sizeof(kernel), "%s (%s)", info->kernel_version, info->architecture);
    draw_kv("Kernel", kernel, width);
    
    draw_kv("Hostname", info->hostname, width);
    draw_kv("Usuário", info->username, width);
    draw_kv("Uptime", info->uptime_formatted, width);
    draw_kv("Shell", info->shell, width);
    draw_kv("Compilador", info->compiler, width);
    
    draw_box_bottom(width);
}

void display_cpu_panel(const CpuInfo *info, int width) {
    draw_box_top(" Processador (CPU)", width);
    
    draw_kv("Modelo", info->model, width);
    
    char cores[64];
    snprintf(cores, sizeof(cores), "%d núcleos / %d threads", info->physical_cores, info->logical_threads);
    draw_kv("Núcleos/Threads", cores, width);
    
    char freq[128];
    snprintf(freq, sizeof(freq), "%.2f MHz (Min: %.2f - Max: %.2f)", info->freq_current_mhz, info->freq_min_mhz, info->freq_max_mhz);
    draw_kv("Frequência", freq, width);
    
    char cache[256];
    snprintf(cache, sizeof(cache), "L1d: %s | L1i: %s | L2: %s | L3: %s", info->cache_l1d, info->cache_l1i, info->cache_l2, info->cache_l3);
    draw_kv("Cache", cache, width);
    
    draw_separator(width);
    draw_progress_bar("Uso Total", info->usage_total, width);
    
    // Per-core display in columns
    int cols = 4;
    for (int i = 0; i < info->num_cores_sampled; i += cols) {
        printf("%s│  %s", BCYAN, RST);
        int printed_chars = 3; // "│  "
        
        for (int j = 0; j < cols && i + j < info->num_cores_sampled; j++) {
            int core_idx = i + j;
            double p = info->usage_per_core[core_idx];
            const char *c = usage_color(p);
            
            char core_lbl[32];
            snprintf(core_lbl, sizeof(core_lbl), "Core %2d: ", core_idx);
            printf("%s%s", BWHITE, core_lbl);
            
            int bar_size = 3;
            int filled = (int)((p / 100.0) * bar_size);
            if (filled > bar_size) filled = bar_size;
            if (filled < 0) filled = 0;
            
            printf("%s", c);
            for(int k = 0; k < filled; k++) printf("█");
            printf("%s%s", RST, DIM);
            for(int k = 0; k < bar_size - filled; k++) printf("░");
            
            printf("%s %3.0f%%   ", RST, p);
            
            // "Core XX: " (9) + bar (3) + " 100%" (5) + "   " (3) = 20
            printed_chars += 20;
        }
        
        int pad = width - printed_chars - 2; // -2 para " │"
        if (pad < 0) pad = 0;
        for (int k = 0; k < pad; k++) printf(" ");
        printf("%s │%s\n", BCYAN, RST);
    }
    
    draw_box_bottom(width);
}

void display_memory_panel(const MemInfo *info, int width) {
    draw_box_top(" Memória RAM & Swap", width);
    
    char buf_total[32], buf_used[32], buf_free[32], buf_avail[32], buf_buf[32], buf_cache[32];
    format_kb(info->total_kb, buf_total, sizeof(buf_total));
    format_kb(info->used_kb, buf_used, sizeof(buf_used));
    format_kb(info->free_kb, buf_free, sizeof(buf_free));
    format_kb(info->available_kb, buf_avail, sizeof(buf_avail));
    format_kb(info->buffers_kb, buf_buf, sizeof(buf_buf));
    format_kb(info->cached_kb, buf_cache, sizeof(buf_cache));
    
    char ram_stats[256];
    snprintf(ram_stats, sizeof(ram_stats), "Total: %s | Usada: %s | Livre: %s | Disp: %s", buf_total, buf_used, buf_free, buf_avail);
    draw_kv("Status RAM", ram_stats, width);
    
    char ram_extra[256];
    snprintf(ram_extra, sizeof(ram_extra), "Buffers: %s | Cached: %s", buf_buf, buf_cache);
    draw_kv("Extra", ram_extra, width);
    
    draw_progress_bar("Uso RAM", info->ram_usage_percent, width);
    draw_separator(width);
    
    char buf_stotal[32], buf_sused[32], buf_sfree[32];
    format_kb(info->swap_total_kb, buf_stotal, sizeof(buf_stotal));
    format_kb(info->swap_used_kb, buf_sused, sizeof(buf_sused));
    format_kb(info->swap_free_kb, buf_sfree, sizeof(buf_sfree));
    
    char swap_stats[256];
    snprintf(swap_stats, sizeof(swap_stats), "Total: %s | Usada: %s | Livre: %s", buf_stotal, buf_sused, buf_sfree);
    draw_kv("Status Swap", swap_stats, width);

    /* Exibe cada dispositivo swap com seu tipo */
    for (int i = 0; i < info->swap_count; i++) {
        char s_size[32], s_used[32];
        format_kb(info->swaps[i].size_kb, s_size, sizeof(s_size));
        format_kb(info->swaps[i].used_kb, s_used, sizeof(s_used));

        char swap_dev_info[256];
        snprintf(swap_dev_info, sizeof(swap_dev_info), "[%s] %s (Usado: %s, Prio: %d)",
                 info->swaps[i].type, s_size, s_used, info->swaps[i].priority);
        draw_kv(info->swaps[i].device, swap_dev_info, width);
    }
    if (info->swap_count == 0 && info->swap_total_kb == 0) {
        draw_kv("Tipo Swap", "Nenhum swap configurado", width);
    }

    draw_progress_bar("Uso Swap", info->swap_usage_percent, width);
    
    draw_box_bottom(width);
}

void display_gpu_panel(const GpuInfo *info, int width) {
    draw_box_top(" Placa de Vídeo (GPU)", width);
    
    if (info->count == 0) {
        draw_kv("Status", "Nenhuma GPU detectada", width);
    } else {
        for (int i = 0; i < info->count; i++) {
            if (i > 0) draw_separator(width);
            draw_kv("Nome", info->gpus[i].name, width);
            draw_kv("Driver", info->gpus[i].driver, width);
            draw_kv("Slot PCI", info->gpus[i].pci_slot, width);
        }
    }
    
    draw_box_bottom(width);
}

void display_storage_panel(const DiskInfo *disks, const MountInfo *mounts, int width) {
    draw_box_top(" Armazenamento", width);
    
    // Subseção: Dispositivos Físicos
    draw_kv("=== Dispositivos Físicos ===", "", width);
    for (int i = 0; i < disks->count; i++) {
        char buf_size[32];
        format_bytes(disks->disks[i].size_bytes, buf_size, sizeof(buf_size));
        
        char disk_info[256];
        snprintf(disk_info, sizeof(disk_info), "%s [%s] - %s", disks->disks[i].model, disks->disks[i].type, buf_size);
        draw_kv(disks->disks[i].device, disk_info, width);
    }
    
    draw_separator(width);
    
    // Subseção: Sistemas de Arquivos Montados
    draw_kv("=== Sistemas de Arquivos ===", "", width);
    for (int i = 0; i < mounts->count; i++) {
        char buf_total[32], buf_used[32];
        format_bytes(mounts->mounts[i].total_bytes, buf_total, sizeof(buf_total));
        format_bytes(mounts->mounts[i].used_bytes, buf_used, sizeof(buf_used));
        
        char mnt_info[256];
        snprintf(mnt_info, sizeof(mnt_info), "→ %s (%s) [%s / %s]", mounts->mounts[i].mount_point, mounts->mounts[i].fs_type, buf_used, buf_total);
        draw_kv(mounts->mounts[i].device, mnt_info, width);
        
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "Uso %s", mounts->mounts[i].mount_point);
        draw_progress_bar(lbl, mounts->mounts[i].usage_percent, width);
    }
    
    draw_box_bottom(width);
}

void display_network_panel(const NetInfo *net, const BtInfo *bt, int width) {
    draw_box_top(" Rede & Conectividade", width);

    /* ─── Interfaces de Rede ─── */
    draw_kv("=== Interfaces de Rede ===", "", width);

    if (net->count == 0) {
        draw_kv("Status", "Nenhuma interface detectada", width);
    } else {
        /* Conta interfaces físicas vs virtuais */
        int phys_count = 0, virt_count = 0;
        for (int i = 0; i < net->count; i++) {
            if (!net->interfaces[i].is_virtual) phys_count++;
            else virt_count++;
        }

        /* 1. Interfaces Físicas & Wi-Fi */
        for (int i = 0; i < net->count; i++) {
            const NetEntry *iface = &net->interfaces[i];
            if (iface->is_virtual && strcmp(iface->name, "lo") != 0) continue;

            char main_info[256];
            if (iface->speed_mbps > 0) {
                snprintf(main_info, sizeof(main_info), "%s [%s] %s (%llu Mbps)",
                         iface->type_desc, iface->state, iface->mac, iface->speed_mbps);
            } else {
                snprintf(main_info, sizeof(main_info), "%s [%s] %s",
                         iface->type_desc, iface->state, iface->mac);
            }
            draw_kv(iface->name, main_info, width);

            /* Linha com IP e Driver */
            char details[256];
            if (iface->ipv4[0] && iface->ipv6[0]) {
                snprintf(details, sizeof(details), "IP: %s | %s | Driver: %s", iface->ipv4, iface->ipv6, iface->driver);
            } else if (iface->ipv4[0]) {
                snprintf(details, sizeof(details), "IP: %s | Driver: %s", iface->ipv4, iface->driver);
            } else {
                snprintf(details, sizeof(details), "Driver: %s", iface->driver);
            }
            draw_kv("", details, width);

            if (i < net->count - 1 && (phys_count > 1 || virt_count > 0)) {
                draw_separator(width);
            }
        }

        /* 2. Interfaces Virtuais / Pontes / Docker */
        int printed_virt = 0;
        for (int i = 0; i < net->count; i++) {
            const NetEntry *iface = &net->interfaces[i];
            if (!iface->is_virtual || strcmp(iface->name, "lo") == 0) continue;

            // Se houver mais de 6 interfaces virtuais, lista as 5 primeiras e resume o restante
            if (virt_count > 6 && printed_virt >= 5) {
                char sum_buf[128];
                snprintf(sum_buf, sizeof(sum_buf), "+ %d outras interfaces virtuais ativas...", virt_count - printed_virt);
                draw_kv("Virtuais (Docker)", sum_buf, width);
                break;
            }

            char main_info[256];
            if (iface->ipv4[0]) {
                snprintf(main_info, sizeof(main_info), "%s [%s] %s (IP: %s)",
                         iface->type_desc, iface->state, iface->mac, iface->ipv4);
            } else {
                snprintf(main_info, sizeof(main_info), "%s [%s] %s",
                         iface->type_desc, iface->state, iface->mac);
            }
            draw_kv(iface->name, main_info, width);
            printed_virt++;
        }
    }

    /* ─── Bluetooth ─── */
    if (bt->count > 0) {
        draw_separator(width);
        draw_kv("=== Bluetooth ===", "", width);

        for (int i = 0; i < bt->count; i++) {
            const BtEntry *adapter = &bt->adapters[i];

            char bt_info[256];
            snprintf(bt_info, sizeof(bt_info), "[%s] %s (%s)",
                     adapter->type_desc, adapter->address, adapter->manufacturer);
            draw_kv(adapter->name, bt_info, width);

            char bt_drv[128];
            snprintf(bt_drv, sizeof(bt_drv), "Driver: %s", adapter->driver);
            draw_kv("", bt_drv, width);
        }
    }

    draw_box_bottom(width);
}

void display_footer(int width) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
    
    char footer[128];
    snprintf(footer, sizeof(footer), "Hardware ID v1.0 | Gerado em: %s", time_str);
    
    int f_len = visible_len(footer);
    int pad_left = (width - f_len) / 2;
    
    printf("\n");
    for (int i = 0; i < pad_left; i++) printf(" ");
    printf("%s%s%s\n\n", DIM, footer, RST);
}
