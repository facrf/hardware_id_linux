#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "display.h"
#include "colors.h"
#include "version.h"

int hardware_id_color_enabled = 1;
int hardware_id_ascii_mode = 0;

void set_color_enabled(int enabled) {
    hardware_id_color_enabled = enabled != 0;
}

void set_ascii_mode(int enabled) {
    hardware_id_ascii_mode = enabled != 0;
}

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

const char *temp_color(double temp) {
    if (temp < 60.0) return GREEN;
    if (temp < 80.0) return YELLOW;
    return RED;
}

void draw_box_top(const char *title, int width) {
    int title_len = visible_len(title);
    int used_len = 5 + title_len + 1; // "╭─── " (5) + título + " " (1)
    int remain = width - used_len - 1; // -1 para "╮"
    if (remain < 0) remain = 0;
    
    const char *tl = hardware_id_ascii_mode ? "+" : "╭";
    const char *tr = hardware_id_ascii_mode ? "+" : "╮";
    const char *hl = hardware_id_ascii_mode ? "-" : "─";

    printf("%s%s%s%s%s %s%s%s%s%s %s", BCYAN, tl, hl, hl, hl, RST, BYELLOW, BOLD, title, RST, BCYAN);
    for (int i = 0; i < remain; i++) {
        printf("%s", hl);
    }
    printf("%s%s\n", tr, RST);
}

void draw_box_bottom(int width) {
    const char *bl = hardware_id_ascii_mode ? "+" : "╰";
    const char *br = hardware_id_ascii_mode ? "+" : "╯";
    const char *hl = hardware_id_ascii_mode ? "-" : "─";
    printf("%s%s", BCYAN, bl);
    for (int i = 0; i < width - 2; i++) {
        printf("%s", hl);
    }
    printf("%s%s\n", br, RST);
}

void draw_separator(int width) {
    const char *ml = hardware_id_ascii_mode ? "+" : "├";
    const char *mr = hardware_id_ascii_mode ? "+" : "┤";
    const char *hl = hardware_id_ascii_mode ? "-" : "─";
    printf("%s%s%s", DIM, BCYAN, ml);
    for (int i = 0; i < width - 2; i++) {
        printf("%s", hl);
    }
    printf("%s%s\n", mr, RST);
}

void draw_section_header(const char *title, int width) {
    if (!title) return;
    int title_len = visible_len(title);
    int used_len = 3 + title_len; // "│  " (3) + title
    int pad_len = width - used_len - 2; // -2 para " │"
    if (pad_len < 0) pad_len = 0;

    const char *vb = hardware_id_ascii_mode ? "|" : "│";
    printf("%s%s  %s%s%s%s", BCYAN, vb, RST, BCYAN, title, RST);
    for (int i = 0; i < pad_len; i++) {
        putchar(' ');
    }
    printf("%s %s%s\n", BCYAN, vb, RST);
}

void draw_kv(const char *key, const char *value, int width) {
    if (!key) key = "";
    if (!value) value = "";

    int key_vis = visible_len(key);
    int key_pad = (key_vis < 18) ? (18 - key_vis) : 0;
    int key_col = (key_vis < 18) ? 18 : key_vis;

    int prefix_len = 3 + key_col + 3; // "│  " (3) + key_col + " : " (3)
    int max_val_len = width - prefix_len - 2; // -2 para " │"
    if (max_val_len < 0) max_val_len = 0;

    int val_vis = visible_len(value);
    char safe_val[512];
    const char *val_to_print = value;

    // Truncamento seguro contra overflow de borda em larguras pequenas
    if (val_vis > max_val_len && max_val_len >= 4) {
        int target_vis = max_val_len - 3; // reserva 3 caracteres para "..."
        int cur_vis = 0;
        int in_ansi = 0;
        size_t i = 0, out_idx = 0;
        while (value[i] && out_idx < sizeof(safe_val) - 8) {
            if (value[i] == '\033') {
                in_ansi = 1;
                safe_val[out_idx++] = value[i++];
            } else if (in_ansi) {
                safe_val[out_idx++] = value[i];
                if (value[i] == 'm') in_ansi = 0;
                i++;
            } else {
                if ((value[i] & 0xC0) != 0x80) {
                    if (cur_vis >= target_vis) break;
                    cur_vis++;
                }
                safe_val[out_idx++] = value[i++];
            }
        }
        safe_val[out_idx++] = '.';
        safe_val[out_idx++] = '.';
        safe_val[out_idx++] = '.';
        if (in_ansi) {
            memcpy(safe_val + out_idx, "\033[0m", 4);
            out_idx += 4;
        }
        safe_val[out_idx] = '\0';
        val_to_print = safe_val;
        val_vis = cur_vis + 3;
    }

    int pad_len = width - (prefix_len + val_vis) - 2; // -2 para " │"
    if (pad_len < 0) pad_len = 0;

    const char *vb = hardware_id_ascii_mode ? "|" : "│";
    printf("%s%s  %s%s%s", BCYAN, vb, BYELLOW, key, RST);
    for (int i = 0; i < key_pad; i++) {
        putchar(' ');
    }
    printf("%s : %s%s%s", RST, BWHITE, val_to_print, RST);
    for (int i = 0; i < pad_len; i++) {
        putchar(' ');
    }
    printf("%s %s%s\n", BCYAN, vb, RST);
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
    const char *vb = hardware_id_ascii_mode ? "|" : "│";
    const char *bf = hardware_id_ascii_mode ? "#" : "█";
    const char *be = hardware_id_ascii_mode ? "-" : "░";

    printf("%s%s  %s%s%s%s : %s[", BCYAN, vb, RST, BYELLOW, padded_label, RST, BCYAN);
    
    printf("%s", color);
    for (int i = 0; i < filled; i++) printf("%s", bf);
    
    printf("%s%s", RST, DIM);
    for (int i = 0; i < empty; i++) printf("%s", be);
    
    printf("%s%s] %s%5.1f%%%s", RST, BCYAN, color, percent, RST);
    
    int used_len = 3 + 18 + 3 + 1 + bar_width + 2 + 6;
    int remain = width - used_len - 2; // -2 para " │"
    if (remain < 0) remain = 0;
    
    for (int i = 0; i < remain; i++) printf(" ");
    printf("%s %s%s\n", BCYAN, vb, RST);
}

void display_header(int width) {
    const char *tl = hardware_id_ascii_mode ? "+" : "╭";
    const char *tr = hardware_id_ascii_mode ? "+" : "╮";
    const char *hl = hardware_id_ascii_mode ? "-" : "─";
    const char *vb = hardware_id_ascii_mode ? "|" : "│";

    printf("\n");
    printf("%s%s", BCYAN, tl);
    for (int i = 0; i < width - 2; i++) printf("%s", hl);
    printf("%s%s\n", tr, RST);
    
    const char *title = "HARDWARE ID — Inspetor de Sistema Linux";
    int t_len = visible_len(title);
    int pad_left = (width - 2 - t_len) / 2;
    int pad_right = width - 2 - t_len - pad_left;
    
    printf("%s%s%s", BCYAN, vb, RST);
    for (int i = 0; i < pad_left; i++) printf(" ");
    printf("%s%s%s%s", BWHITE, BOLD, title, RST);
    for (int i = 0; i < pad_right; i++) printf(" ");
    printf("%s%s%s\n", BCYAN, vb, RST);
    
    draw_box_bottom(width);
}

void display_hardware_id_panel(const HardwareIdInfo *info, int width, int include_identifiers) {
    draw_box_top("🔑 Identificação Única & Placa-Mãe (DMI)", width);

    draw_kv("Hardware ID", include_identifiers ? info->hardware_id : "<redacted>", width);
    draw_kv("Machine UUID", include_identifiers ? info->formatted_uuid : "<redacted>", width);
    draw_kv("Machine ID", include_identifiers ? info->machine_id : "<redacted>", width);

    char mobo[256];
    snprintf(mobo, sizeof(mobo), "%s %s", info->board_vendor, info->board_name);
    draw_kv("Placa-Mãe", mobo, width);

    char sys_prod[256];
    snprintf(sys_prod, sizeof(sys_prod), "%s %s (%s)", info->sys_vendor, info->product_name, info->product_version);
    draw_kv("Sistema/Chassis", sys_prod, width);

    char bios[256];
    snprintf(bios, sizeof(bios), "%s v%s (%s)", info->bios_vendor, info->bios_version, info->bios_date);
    draw_kv("BIOS / UEFI", bios, width);

    if (info->product_serial[0]) {
        draw_kv("Serial Produto", include_identifiers ? info->product_serial : "<redacted>", width);
    }
    if (info->board_serial[0]) {
        draw_kv("Serial Placa", include_identifiers ? info->board_serial : "<redacted>", width);
    }

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
    
    draw_kv("Ambiente", info->virtualization, width);
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
    if (info->freq_min_mhz > 0.0 && info->freq_max_mhz > 0.0) {
        snprintf(freq, sizeof(freq), "%.2f MHz (Min: %.2f - Max: %.2f)", info->freq_current_mhz, info->freq_min_mhz, info->freq_max_mhz);
    } else if (info->freq_current_mhz > 0.0) {
        snprintf(freq, sizeof(freq), "%.2f MHz", info->freq_current_mhz);
    } else {
        snprintf(freq, sizeof(freq), "N/A");
    }
    draw_kv("Frequência", freq, width);
    
    if (info->temp_celsius > 0.0) {
        char temp_str[64];
        snprintf(temp_str, sizeof(temp_str), "%.1f °C", info->temp_celsius);
        draw_kv("Temperatura", temp_str, width);
    }

    char cache[256];
    snprintf(cache, sizeof(cache), "L1d: %s | L1i: %s | L2: %s | L3: %s", info->cache_l1d, info->cache_l1i, info->cache_l2, info->cache_l3);
    draw_kv("Cache", cache, width);
    
    draw_separator(width);
    draw_progress_bar("Uso Total", info->usage_total, width);
    
    // Per-core display in columns (responsivo para larguras menores como 80 colunas)
    const char *vb = hardware_id_ascii_mode ? "|" : "│";
    const char *bf = hardware_id_ascii_mode ? "#" : "█";
    const char *be = hardware_id_ascii_mode ? "-" : "░";

    int cols = (width >= 86) ? 4 : (width >= 66 ? 3 : 2);
    for (int i = 0; i < info->num_cores_sampled; i += cols) {
        printf("%s%s  %s", BCYAN, vb, RST);
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
            for(int k = 0; k < filled; k++) printf("%s", bf);
            printf("%s%s", RST, DIM);
            for(int k = 0; k < bar_size - filled; k++) printf("%s", be);
            
            printf("%s %3.0f%%   ", RST, p);
            
            // "Core XX: " (9) + bar (3) + " 100%" (5) + "   " (3) = 20
            printed_chars += 20;
        }
        
        int pad = width - printed_chars - 2; // -2 para " │"
        if (pad < 0) pad = 0;
        for (int k = 0; k < pad; k++) printf(" ");
        printf("%s %s%s\n", BCYAN, vb, RST);
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
    
    if (width < 90) {
        char ram_stats1[128], ram_stats2[128];
        snprintf(ram_stats1, sizeof(ram_stats1), "Total: %s | Usada: %s", buf_total, buf_used);
        snprintf(ram_stats2, sizeof(ram_stats2), "Livre: %s | Disp: %s", buf_free, buf_avail);
        draw_kv("Status RAM", ram_stats1, width);
        draw_kv("", ram_stats2, width);
    } else {
        char ram_stats[256];
        snprintf(ram_stats, sizeof(ram_stats), "Total: %s | Usada: %s | Livre: %s | Disp: %s", buf_total, buf_used, buf_free, buf_avail);
        draw_kv("Status RAM", ram_stats, width);
    }
    
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

void display_storage_panel(const DiskInfo *disks, const MountInfo *mounts, int width, int include_identifiers) {
    draw_box_top(" Armazenamento", width);
    
    // Subseção: Dispositivos Físicos
    draw_section_header("=== Dispositivos Físicos ===", width);
    for (int i = 0; i < disks->count; i++) {
        char buf_size[32];
        format_bytes(disks->disks[i].size_bytes, buf_size, sizeof(buf_size));
        
        char disk_info[256];
        if (strcmp(disks->disks[i].serial, "N/A") != 0 && disks->disks[i].serial[0] != '\0') {
            snprintf(disk_info, sizeof(disk_info), "%s [%s] - %s (S/N: %s)",
                     disks->disks[i].model, disks->disks[i].type, buf_size,
                     include_identifiers ? disks->disks[i].serial : "<redacted>");
        } else {
            snprintf(disk_info, sizeof(disk_info), "%s [%s] - %s", disks->disks[i].model, disks->disks[i].type, buf_size);
        }
        draw_kv(disks->disks[i].device, disk_info, width);
    }
    
    draw_separator(width);
    
    // Subseção: Sistemas de Arquivos Montados
    draw_section_header("=== Sistemas de Arquivos ===", width);
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

void display_network_panel(const NetInfo *net, const BtInfo *bt, int width, int include_identifiers) {
    draw_box_top(" Rede & Conectividade", width);

    /* ─── Interfaces de Rede ─── */
    draw_section_header("=== Interfaces de Rede ===", width);

    if (net->default_gateway[0] != '\0') {
        char gw_info[128];
        snprintf(gw_info, sizeof(gw_info), "%s (via %s)",
                 include_identifiers ? net->default_gateway : "<redacted>", net->default_iface);
        draw_kv("Gateway Padrão", gw_info, width);
        draw_separator(width);
    }

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
            const char *gw_tag = iface->is_default_gateway ? " [Rota Padrão]" : "";
            if (iface->speed_mbps > 0) {
                snprintf(main_info, sizeof(main_info), "%s [%s] %s (%llu Mbps)%s",
                         iface->type_desc, iface->state, include_identifiers ? iface->mac : "<redacted>", iface->speed_mbps, gw_tag);
            } else {
                snprintf(main_info, sizeof(main_info), "%s [%s] %s%s",
                         iface->type_desc, iface->state, include_identifiers ? iface->mac : "<redacted>", gw_tag);
            }
            draw_kv(iface->name, main_info, width);

            /* Linha com IP e Driver */
            char details[256];
            if (iface->ipv4[0] && iface->ipv6[0]) {
                snprintf(details, sizeof(details), "IP: %s | %s | Driver: %s", include_identifiers ? iface->ipv4 : "<redacted>", include_identifiers ? iface->ipv6 : "<redacted>", iface->driver);
            } else if (iface->ipv4[0]) {
                snprintf(details, sizeof(details), "IP: %s | Driver: %s", include_identifiers ? iface->ipv4 : "<redacted>", iface->driver);
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
                         iface->type_desc, iface->state, include_identifiers ? iface->mac : "<redacted>", include_identifiers ? iface->ipv4 : "<redacted>");
            } else {
                snprintf(main_info, sizeof(main_info), "%s [%s] %s",
                         iface->type_desc, iface->state, include_identifiers ? iface->mac : "<redacted>");
            }
            draw_kv(iface->name, main_info, width);
            printed_virt++;
        }
    }

    /* ─── Bluetooth ─── */
    if (bt->count > 0) {
        draw_separator(width);
        draw_section_header("=== Bluetooth ===", width);

        for (int i = 0; i < bt->count; i++) {
            const BtEntry *adapter = &bt->adapters[i];

            char bt_info[256];
            snprintf(bt_info, sizeof(bt_info), "[%s] %s (%s)",
                     adapter->type_desc, include_identifiers ? adapter->address : "<redacted>", adapter->manufacturer);
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
    snprintf(footer, sizeof(footer), "Hardware ID v%s | Gerado em: %s", HARDWARE_ID_VERSION, time_str);
    
    int f_len = visible_len(footer);
    int pad_left = (width - f_len) / 2;
    
    printf("\n");
    for (int i = 0; i < pad_left; i++) printf(" ");
    printf("%s%s%s\n\n", DIM, footer, RST);
}
