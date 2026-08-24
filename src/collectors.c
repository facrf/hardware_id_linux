#include "collectors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ctype.h>
#include <pwd.h>
#include <time.h>

// Remove espaços e quebras de linha no início e fim da string
void trim_string(char *str) {
    if (!str) return;
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

// Lê o conteúdo de um arquivo de forma segura
void safe_read_file(const char *path, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    buf[0] = '\0';
    FILE *f = fopen(path, "r");
    if (f) {
        size_t bytes_read = fread(buf, 1, buf_size - 1, f);
        buf[bytes_read] = '\0';
        fclose(f);
        trim_string(buf);
    }
}

// Formata tamanho em bytes para formato legível (KiB, MiB, etc)
void format_bytes(unsigned long long bytes, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    double size = (double)bytes;
    if (size < 1024.0) {
        snprintf(buf, buf_size, "%.0f B", size);
    } else if (size < 1024.0 * 1024.0) {
        snprintf(buf, buf_size, "%.2f KiB", size / 1024.0);
    } else if (size < 1024.0 * 1024.0 * 1024.0) {
        snprintf(buf, buf_size, "%.2f MiB", size / (1024.0 * 1024.0));
    } else if (size < 1024.0 * 1024.0 * 1024.0 * 1024.0) {
        snprintf(buf, buf_size, "%.2f GiB", size / (1024.0 * 1024.0 * 1024.0));
    } else {
        snprintf(buf, buf_size, "%.2f TiB", size / (1024.0 * 1024.0 * 1024.0 * 1024.0));
    }
}

// Formata tamanho em KB para formato legível convertendo para bytes
void format_kb(unsigned long long kb, char *buf, size_t buf_size) {
    format_bytes(kb * 1024ULL, buf, buf_size);
}

// Coleta informações gerais do sistema
void collect_system_info(SysInfo *info) {
    memset(info, 0, sizeof(*info));
    
    // Ler informações da distribuição (/etc/os-release)
    FILE *f = fopen("/etc/os-release", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            trim_string(line);
            if (strncmp(line, "NAME=", 5) == 0) {
                char *val = line + 5;
                if (val[0] == '"') val++;
                size_t len = strlen(val);
                if (len > 0 && val[len-1] == '"') val[len-1] = '\0';
                snprintf(info->distro_name, sizeof(info->distro_name), "%s", val);
            } else if (strncmp(line, "VERSION_ID=", 11) == 0) {
                char *val = line + 11;
                if (val[0] == '"') val++;
                size_t len = strlen(val);
                if (len > 0 && val[len-1] == '"') val[len-1] = '\0';
                snprintf(info->distro_version, sizeof(info->distro_version), "%s", val);
            } else if (strncmp(line, "VERSION_CODENAME=", 17) == 0) {
                char *val = line + 17;
                if (val[0] == '"') val++;
                size_t len = strlen(val);
                if (len > 0 && val[len-1] == '"') val[len-1] = '\0';
                snprintf(info->distro_codename, sizeof(info->distro_codename), "%s", val);
            }
        }
        fclose(f);
    }
    if (!info->distro_name[0]) snprintf(info->distro_name, sizeof(info->distro_name), "Desconhecido");
    if (!info->distro_version[0]) snprintf(info->distro_version, sizeof(info->distro_version), "Desconhecido");
    if (!info->distro_codename[0]) snprintf(info->distro_codename, sizeof(info->distro_codename), "Desconhecido");

    // Kernel e Arquitetura
    struct utsname uts;
    if (uname(&uts) == 0) {
        snprintf(info->kernel_version, sizeof(info->kernel_version), "%s", uts.release);
        snprintf(info->architecture, sizeof(info->architecture), "%s", uts.machine);
    } else {
        snprintf(info->kernel_version, sizeof(info->kernel_version), "Desconhecido");
        snprintf(info->architecture, sizeof(info->architecture), "Desconhecido");
    }

    // Hostname
    if (gethostname(info->hostname, sizeof(info->hostname)) != 0) {
        snprintf(info->hostname, sizeof(info->hostname), "Desconhecido");
    }

    // Usuário logado
    char *user = getlogin();
    if (!user) user = getenv("USER");
    if (user) {
        snprintf(info->username, sizeof(info->username), "%s", user);
    } else {
        snprintf(info->username, sizeof(info->username), "Desconhecido");
    }

    // Uptime
    FILE *uf = fopen("/proc/uptime", "r");
    if (uf) {
        double uptime_sec;
        if (fscanf(uf, "%lf", &uptime_sec) == 1) {
            info->uptime_seconds = (long)uptime_sec;
            long dias = info->uptime_seconds / 86400;
            long horas = (info->uptime_seconds % 86400) / 3600;
            long mins = (info->uptime_seconds % 3600) / 60;
            snprintf(info->uptime_formatted, sizeof(info->uptime_formatted), "%ld dias, %ld horas, %ld min", dias, horas, mins);
        } else {
            snprintf(info->uptime_formatted, sizeof(info->uptime_formatted), "Desconhecido");
        }
        fclose(uf);
    } else {
        snprintf(info->uptime_formatted, sizeof(info->uptime_formatted), "Desconhecido");
    }

    // Shell
    char *shell = getenv("SHELL");
    if (shell) {
        snprintf(info->shell, sizeof(info->shell), "%s", shell);
    } else {
        snprintf(info->shell, sizeof(info->shell), "Desconhecido");
    }

    // Compilador
#ifdef __VERSION__
    snprintf(info->compiler, sizeof(info->compiler), "GCC %s", __VERSION__);
#else
    snprintf(info->compiler, sizeof(info->compiler), "Desconhecido");
#endif
}

// Estrutura auxiliar para cálculo de uso de CPU
struct CpuStat {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
};

// Lê as métricas atuais de CPU do /proc/stat
static void read_cpu_stats(struct CpuStat *total, struct CpuStat cores[], int *num_cores) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return;
    char line[256];
    *num_cores = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            sscanf(line + 4, "%llu %llu %llu %llu %llu %llu %llu %llu",
                &total->user, &total->nice, &total->system, &total->idle,
                &total->iowait, &total->irq, &total->softirq, &total->steal);
        } else if (strncmp(line, "cpu", 3) == 0 && isdigit((unsigned char)line[3])) {
            if (*num_cores < MAX_CORES) {
                sscanf(line + 3, "%*d %llu %llu %llu %llu %llu %llu %llu %llu",
                    &cores[*num_cores].user, &cores[*num_cores].nice, &cores[*num_cores].system,
                    &cores[*num_cores].idle, &cores[*num_cores].iowait, &cores[*num_cores].irq,
                    &cores[*num_cores].softirq, &cores[*num_cores].steal);
                (*num_cores)++;
            }
        }
    }
    fclose(f);
}

// Coleta informações detalhadas do processador
void collect_cpu_info(CpuInfo *info) {
    memset(info, 0, sizeof(*info));

    // Lógica para contar núcleos lógicos e físicos, além do modelo
    int phys_ids[MAX_CORES];
    int core_ids[MAX_CORES];
    int pair_count = 0;
    int logical_threads = 0;

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        int current_phys = -1;
        int current_core = -1;
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "processor", 9) == 0) {
                logical_threads++;
                if (current_phys != -1 && current_core != -1) {
                    int unique = 1;
                    for (int i = 0; i < pair_count; i++) {
                        if (phys_ids[i] == current_phys && core_ids[i] == current_core) {
                            unique = 0;
                            break;
                        }
                    }
                    if (unique && pair_count < MAX_CORES) {
                        phys_ids[pair_count] = current_phys;
                        core_ids[pair_count] = current_core;
                        pair_count++;
                    }
                }
                current_phys = -1;
                current_core = -1;
            } else if (strncmp(line, "model name", 10) == 0) {
                if (info->model[0] == '\0') {
                    char *colon = strchr(line, ':');
                    if (colon) {
                        snprintf(info->model, sizeof(info->model), "%s", colon + 1);
                        trim_string(info->model);
                    }
                }
            } else if (strncmp(line, "physical id", 11) == 0) {
                char *colon = strchr(line, ':');
                if (colon) current_phys = atoi(colon + 1);
            } else if (strncmp(line, "core id", 7) == 0) {
                char *colon = strchr(line, ':');
                if (colon) current_core = atoi(colon + 1);
            }
        }
        if (current_phys != -1 && current_core != -1) {
            int unique = 1;
            for (int i = 0; i < pair_count; i++) {
                if (phys_ids[i] == current_phys && core_ids[i] == current_core) {
                    unique = 0;
                    break;
                }
            }
            if (unique && pair_count < MAX_CORES) {
                phys_ids[pair_count] = current_phys;
                core_ids[pair_count] = current_core;
                pair_count++;
            }
        }
        fclose(f);
    }
    
    info->logical_threads = logical_threads;
    info->physical_cores = pair_count > 0 ? pair_count : logical_threads; // fallback
    if (!info->model[0]) snprintf(info->model, sizeof(info->model), "Desconhecido");

    // Frequências
    char buf[128];
    safe_read_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", buf, sizeof(buf));
    if (buf[0]) info->freq_current_mhz = atof(buf) / 1000.0; else info->freq_current_mhz = 0.0;

    safe_read_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq", buf, sizeof(buf));
    if (buf[0]) info->freq_min_mhz = atof(buf) / 1000.0; else info->freq_min_mhz = 0.0;

    safe_read_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", buf, sizeof(buf));
    if (buf[0]) info->freq_max_mhz = atof(buf) / 1000.0; else info->freq_max_mhz = 0.0;

    // Cache
    snprintf(info->cache_l1d, sizeof(info->cache_l1d), "N/A");
    snprintf(info->cache_l1i, sizeof(info->cache_l1i), "N/A");
    snprintf(info->cache_l2, sizeof(info->cache_l2), "N/A");
    snprintf(info->cache_l3, sizeof(info->cache_l3), "N/A");

    for (int i = 0; i <= 3; i++) {
        char path_lvl[128], path_type[128], path_sz[128];
        snprintf(path_lvl, sizeof(path_lvl), "/sys/devices/system/cpu/cpu0/cache/index%d/level", i);
        snprintf(path_type, sizeof(path_type), "/sys/devices/system/cpu/cpu0/cache/index%d/type", i);
        snprintf(path_sz, sizeof(path_sz), "/sys/devices/system/cpu/cpu0/cache/index%d/size", i);

        char lvl[32] = {0}, type[32] = {0}, sz[32] = {0};
        safe_read_file(path_lvl, lvl, sizeof(lvl));
        safe_read_file(path_type, type, sizeof(type));
        safe_read_file(path_sz, sz, sizeof(sz));

        if (lvl[0] && type[0] && sz[0]) {
            if (strcmp(lvl, "1") == 0) {
                if (strcmp(type, "Data") == 0) snprintf(info->cache_l1d, sizeof(info->cache_l1d), "%s", sz);
                else if (strcmp(type, "Instruction") == 0) snprintf(info->cache_l1i, sizeof(info->cache_l1i), "%s", sz);
            } else if (strcmp(lvl, "2") == 0) {
                snprintf(info->cache_l2, sizeof(info->cache_l2), "%s", sz);
            } else if (strcmp(lvl, "3") == 0) {
                snprintf(info->cache_l3, sizeof(info->cache_l3), "%s", sz);
            }
        }
    }

    // Uso de CPU
    struct CpuStat total1, total2;
    struct CpuStat cores1[MAX_CORES], cores2[MAX_CORES];
    int num_cores1 = 0, num_cores2 = 0;

    read_cpu_stats(&total1, cores1, &num_cores1);
    usleep(200000); // 200ms
    read_cpu_stats(&total2, cores2, &num_cores2);

    unsigned long long active1 = total1.user + total1.nice + total1.system + total1.irq + total1.softirq + total1.steal;
    unsigned long long total_time1 = active1 + total1.idle + total1.iowait;

    unsigned long long active2 = total2.user + total2.nice + total2.system + total2.irq + total2.softirq + total2.steal;
    unsigned long long total_time2 = active2 + total2.idle + total2.iowait;

    if (total_time2 > total_time1) {
        info->usage_total = 100.0 * (active2 - active1) / (double)(total_time2 - total_time1);
    } else {
        info->usage_total = 0.0;
    }

    int num_cores = num_cores1 < num_cores2 ? num_cores1 : num_cores2;
    if (num_cores > MAX_CORES) num_cores = MAX_CORES;
    info->num_cores_sampled = num_cores;

    for (int i = 0; i < num_cores; i++) {
        unsigned long long c_active1 = cores1[i].user + cores1[i].nice + cores1[i].system + cores1[i].irq + cores1[i].softirq + cores1[i].steal;
        unsigned long long c_total1 = c_active1 + cores1[i].idle + cores1[i].iowait;

        unsigned long long c_active2 = cores2[i].user + cores2[i].nice + cores2[i].system + cores2[i].irq + cores2[i].softirq + cores2[i].steal;
        unsigned long long c_total2 = c_active2 + cores2[i].idle + cores2[i].iowait;

        if (c_total2 > c_total1) {
            info->usage_per_core[i] = 100.0 * (c_active2 - c_active1) / (double)(c_total2 - c_total1);
        } else {
            info->usage_per_core[i] = 0.0;
        }
    }
}

// Coleta estatísticas de memória RAM e Swap
void collect_memory_info(MemInfo *info) {
    memset(info, 0, sizeof(*info));
    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "MemTotal:", 9) == 0) sscanf(line + 9, "%llu", &info->total_kb);
            else if (strncmp(line, "MemFree:", 8) == 0) sscanf(line + 8, "%llu", &info->free_kb);
            else if (strncmp(line, "MemAvailable:", 13) == 0) sscanf(line + 13, "%llu", &info->available_kb);
            else if (strncmp(line, "Buffers:", 8) == 0) sscanf(line + 8, "%llu", &info->buffers_kb);
            else if (strncmp(line, "Cached:", 7) == 0) sscanf(line + 7, "%llu", &info->cached_kb);
            else if (strncmp(line, "SwapTotal:", 10) == 0) sscanf(line + 10, "%llu", &info->swap_total_kb);
            else if (strncmp(line, "SwapFree:", 9) == 0) sscanf(line + 9, "%llu", &info->swap_free_kb);
        }
        fclose(f);
    }
    
    long long used = (long long)info->total_kb - info->free_kb - info->buffers_kb - info->cached_kb;
    info->used_kb = used > 0 ? used : 0;

    info->swap_used_kb = info->swap_total_kb > info->swap_free_kb ? info->swap_total_kb - info->swap_free_kb : 0;

    if (info->total_kb > 0) {
        info->ram_usage_percent = 100.0 * (double)info->used_kb / (double)info->total_kb;
    }
    if (info->swap_total_kb > 0) {
        info->swap_usage_percent = 100.0 * (double)info->swap_used_kb / (double)info->swap_total_kb;
    }
}

// Coleta informações sobre as GPUs do sistema utilizando sysfs
void collect_gpu_info(GpuInfo *info) {
    memset(info, 0, sizeof(*info));
    DIR *d = opendir("/sys/bus/pci/devices");
    if (!d) return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        
        char class_path[512];
        snprintf(class_path, sizeof(class_path), "/sys/bus/pci/devices/%s/class", dir->d_name);
        char class_str[32];
        safe_read_file(class_path, class_str, sizeof(class_str));
        
        if (strncmp(class_str, "0x0300", 6) == 0 || strncmp(class_str, "0x0302", 6) == 0) {
            if (info->count >= MAX_GPUS) break;
            GpuEntry *gpu = &info->gpus[info->count];
            snprintf(gpu->pci_slot, sizeof(gpu->pci_slot), "%s", dir->d_name);

            char vendor_path[512], device_path[512];
            snprintf(vendor_path, sizeof(vendor_path), "/sys/bus/pci/devices/%s/vendor", dir->d_name);
            snprintf(device_path, sizeof(device_path), "/sys/bus/pci/devices/%s/device", dir->d_name);
            
            char vendor_str[32], device_str[32];
            safe_read_file(vendor_path, vendor_str, sizeof(vendor_str));
            safe_read_file(device_path, device_str, sizeof(device_str));

            const char *vendor_name = "Desconhecido";
            if (strncmp(vendor_str, "0x10de", 6) == 0) vendor_name = "NVIDIA";
            else if (strncmp(vendor_str, "0x1002", 6) == 0) vendor_name = "AMD/ATI";
            else if (strncmp(vendor_str, "0x8086", 6) == 0) vendor_name = "Intel";

            char label_path[512], label_str[256] = {0};
            snprintf(label_path, sizeof(label_path), "/sys/bus/pci/devices/%s/label", dir->d_name);
            safe_read_file(label_path, label_str, sizeof(label_str));
            if (label_str[0]) {
                snprintf(gpu->name, sizeof(gpu->name), "%s", label_str);
            } else {
                snprintf(gpu->name, sizeof(gpu->name), "%s %s", vendor_name, device_str);
            }

            // Descobre o driver em uso
            char driver_symlink[512];
            snprintf(driver_symlink, sizeof(driver_symlink), "/sys/bus/pci/devices/%s/driver", dir->d_name);
            char driver_target[512];
            ssize_t len = readlink(driver_symlink, driver_target, sizeof(driver_target) - 1);
            if (len != -1) {
                driver_target[len] = '\0';
                char *basename = strrchr(driver_target, '/');
                if (basename) snprintf(gpu->driver, sizeof(gpu->driver), "%s", basename + 1);
                else snprintf(gpu->driver, sizeof(gpu->driver), "%s", driver_target);
            } else {
                snprintf(gpu->driver, sizeof(gpu->driver), "N/A");
            }

            info->count++;
        }
    }
    closedir(d);
}

// Coleta informações sobre os discos de bloco instalados
void collect_disk_info(DiskInfo *info) {
    memset(info, 0, sizeof(*info));
    DIR *d = opendir("/sys/block");
    if (!d) return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        const char *dn = dir->d_name;
        // Ignora dispositivos de bloco virtuais ou não relevantes
        if (strncmp(dn, "loop", 4) == 0 || strncmp(dn, "ram", 3) == 0 ||
            strncmp(dn, "dm-", 3) == 0 || strncmp(dn, "sr", 2) == 0 ||
            strncmp(dn, "fd", 2) == 0) {
            continue;
        }

        if (info->count >= MAX_DISKS) break;
        DiskEntry *disk = &info->disks[info->count];
        snprintf(disk->device, sizeof(disk->device), "%s", dn);

        char model_path[512], model_str[256];
        snprintf(model_path, sizeof(model_path), "/sys/block/%s/device/model", dn);
        safe_read_file(model_path, model_str, sizeof(model_str));
        if (model_str[0]) {
            snprintf(disk->model, sizeof(disk->model), "%s", model_str);
        } else {
            snprintf(disk->model, sizeof(disk->model), "Desconhecido");
        }

        char rot_path[512], rot_str[32];
        snprintf(rot_path, sizeof(rot_path), "/sys/block/%s/queue/rotational", dn);
        safe_read_file(rot_path, rot_str, sizeof(rot_str));
        if (strcmp(rot_str, "1") == 0) {
            snprintf(disk->type, sizeof(disk->type), "HDD");
        } else {
            if (strncmp(dn, "nvme", 4) == 0) {
                snprintf(disk->type, sizeof(disk->type), "NVMe");
            } else {
                snprintf(disk->type, sizeof(disk->type), "SSD");
            }
        }

        char size_path[512], size_str[64];
        snprintf(size_path, sizeof(size_path), "/sys/block/%s/size", dn);
        safe_read_file(size_path, size_str, sizeof(size_str));
        if (size_str[0]) {
            disk->size_bytes = strtoull(size_str, NULL, 10) * 512ULL;
        }

        info->count++;
    }
    closedir(d);
}

// Coleta pontos de montagem e espaços em uso
void collect_mount_info(MountInfo *info) {
    memset(info, 0, sizeof(*info));
    FILE *f = fopen("/proc/mounts", "r");
    if (!f) return;
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char device[256], mount_point[256], fs_type[64];
        if (sscanf(line, "%255s %255s %63s", device, mount_point, fs_type) == 3) {
            // Filtrar para mostrar somente montagens de dispositivos físicos reais
            if (strncmp(device, "/dev/", 5) != 0) continue;

            const char *skip_fs[] = {"tmpfs", "devtmpfs", "sysfs", "proc", "devpts", "securityfs", "cgroup", "pstore", "debugfs", "hugetlbfs", "mqueue", "configfs", "fusectl", "overlay", NULL};
            int skip = 0;
            for (int i = 0; skip_fs[i] != NULL; i++) {
                if (strcmp(fs_type, skip_fs[i]) == 0) {
                    skip = 1; break;
                }
            }
            if (skip) continue;

            if (info->count >= MAX_MOUNTS) break;
            MountEntry *mnt = &info->mounts[info->count];
            snprintf(mnt->device, sizeof(mnt->device), "%s", device);
            snprintf(mnt->mount_point, sizeof(mnt->mount_point), "%s", mount_point);
            snprintf(mnt->fs_type, sizeof(mnt->fs_type), "%s", fs_type);

            struct statvfs sv;
            if (statvfs(mount_point, &sv) == 0) {
                mnt->total_bytes = (unsigned long long)sv.f_blocks * sv.f_frsize;
                mnt->free_bytes = (unsigned long long)sv.f_bavail * sv.f_frsize;
                mnt->used_bytes = mnt->total_bytes > mnt->free_bytes ? mnt->total_bytes - mnt->free_bytes : 0;
                if (mnt->total_bytes > 0) {
                    mnt->usage_percent = 100.0 * (double)mnt->used_bytes / (double)mnt->total_bytes;
                }
            }
            info->count++;
        }
    }
    fclose(f);
}
