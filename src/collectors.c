#include "collectors.h"
#include "sha256.h"
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
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Remove espaços e quebras de linha no início e fim da string
void trim_string(char *str) {
    if (!str) return;
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    if (*str == '\0') return;
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

// Detecta se o sistema está rodando em máquina física (bare metal), contêiner ou máquina virtual
static void detect_virtualization(char *virt, size_t size) {
    if (!virt || size == 0) return;
    virt[0] = '\0';

    // 1. Contêineres
    if (access("/.dockerenv", F_OK) == 0) {
        snprintf(virt, size, "Docker (Contêiner)");
        return;
    }
    if (access("/run/.containerenv", F_OK) == 0) {
        snprintf(virt, size, "Podman (Contêiner)");
        return;
    }

    char cgroup_buf[1024];
    safe_read_file("/proc/1/cgroup", cgroup_buf, sizeof(cgroup_buf));
    if (strstr(cgroup_buf, "docker") != NULL) {
        snprintf(virt, size, "Docker (Contêiner)");
        return;
    }
    if (strstr(cgroup_buf, "kubepods") != NULL) {
        snprintf(virt, size, "Kubernetes (Pod)");
        return;
    }
    if (strstr(cgroup_buf, "containerd") != NULL) {
        snprintf(virt, size, "Containerd (Contêiner)");
        return;
    }
    if (strstr(cgroup_buf, "lxc") != NULL) {
        snprintf(virt, size, "LXC (Contêiner)");
        return;
    }

    char environ_buf[1024];
    safe_read_file("/proc/1/environ", environ_buf, sizeof(environ_buf));
    if (strstr(environ_buf, "container=lxc") != NULL) {
        snprintf(virt, size, "LXC (Contêiner)");
        return;
    }

    // 2. WSL (Windows Subsystem for Linux)
    char ver_buf[512];
    safe_read_file("/proc/version", ver_buf, sizeof(ver_buf));
    if (strstr(ver_buf, "microsoft") != NULL || strstr(ver_buf, "WSL") != NULL) {
        snprintf(virt, size, "WSL2 (Windows Subsystem for Linux)");
        return;
    }

    // 3. DMI / SMBIOS para detecção de Hipervisor / VM
    char dmi_sys[128] = {0}, dmi_prod[128] = {0}, dmi_bios[128] = {0};
    safe_read_file("/sys/class/dmi/id/sys_vendor", dmi_sys, sizeof(dmi_sys));
    safe_read_file("/sys/class/dmi/id/product_name", dmi_prod, sizeof(dmi_prod));
    safe_read_file("/sys/class/dmi/id/bios_vendor", dmi_bios, sizeof(dmi_bios));

    if (strstr(dmi_sys, "QEMU") != NULL || strstr(dmi_prod, "KVM") != NULL ||
        strstr(dmi_bios, "QEMU") != NULL || strstr(dmi_sys, "Bochs") != NULL) {
        snprintf(virt, size, "QEMU / KVM (Máquina Virtual)");
        return;
    }
    if (strstr(dmi_sys, "VMware") != NULL || strstr(dmi_prod, "VMware") != NULL) {
        snprintf(virt, size, "VMware (Máquina Virtual)");
        return;
    }
    if (strstr(dmi_sys, "innotek") != NULL || strstr(dmi_prod, "VirtualBox") != NULL ||
        strstr(dmi_bios, "innotek") != NULL) {
        snprintf(virt, size, "VirtualBox (Máquina Virtual)");
        return;
    }
    if (strstr(dmi_sys, "Microsoft Corporation") != NULL && strstr(dmi_prod, "Virtual Machine") != NULL) {
        snprintf(virt, size, "Hyper-V (Máquina Virtual)");
        return;
    }
    if (strstr(dmi_sys, "Amazon EC2") != NULL) {
        snprintf(virt, size, "AWS EC2 (Nitro/Xen)");
        return;
    }
    if (strstr(dmi_sys, "Google") != NULL && strstr(dmi_prod, "Compute Engine") != NULL) {
        snprintf(virt, size, "Google Cloud (Compute Engine)");
        return;
    }
    if (strstr(dmi_sys, "OpenStack") != NULL || strstr(dmi_prod, "OpenStack") != NULL) {
        snprintf(virt, size, "OpenStack (Cloud VM)");
        return;
    }
    if (strstr(dmi_sys, "Parallels") != NULL || strstr(dmi_prod, "Parallels") != NULL) {
        snprintf(virt, size, "Parallels (Máquina Virtual)");
        return;
    }
    if (access("/proc/xen", F_OK) == 0 || strstr(dmi_bios, "Xen") != NULL) {
        snprintf(virt, size, "Xen (Máquina Virtual)");
        return;
    }

    char hyp_type[64] = {0};
    safe_read_file("/sys/hypervisor/type", hyp_type, sizeof(hyp_type));
    if (hyp_type[0] != '\0') {
        snprintf(virt, size, "%s (Hipervisor)", hyp_type);
        return;
    }

    snprintf(virt, size, "Máquina Física (Bare Metal)");
}

// Coleta informações gerais do sistema
void collect_system_info(SysInfo *info) {
    memset(info, 0, sizeof(*info));
    detect_virtualization(info->virtualization, sizeof(info->virtualization));
    
    // Ler informações da distribuição (/etc/os-release com fallback para /usr/lib/os-release)
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) {
        f = fopen("/usr/lib/os-release", "r");
    }
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

// Coleta identificadores de hardware, DMI/SMBIOS e gera a impressão digital (Hardware Fingerprint)
void collect_hardware_id_info(HardwareIdInfo *info, const CpuInfo *cpu, const NetInfo *net) {
    memset(info, 0, sizeof(*info));

    // 1. DMI / SMBIOS (/sys/class/dmi/id/)
    safe_read_file("/sys/class/dmi/id/sys_vendor", info->sys_vendor, sizeof(info->sys_vendor));
    safe_read_file("/sys/class/dmi/id/product_name", info->product_name, sizeof(info->product_name));
    safe_read_file("/sys/class/dmi/id/product_version", info->product_version, sizeof(info->product_version));
    safe_read_file("/sys/class/dmi/id/product_uuid", info->product_uuid, sizeof(info->product_uuid));
    safe_read_file("/sys/class/dmi/id/board_vendor", info->board_vendor, sizeof(info->board_vendor));
    safe_read_file("/sys/class/dmi/id/board_name", info->board_name, sizeof(info->board_name));
    safe_read_file("/sys/class/dmi/id/bios_vendor", info->bios_vendor, sizeof(info->bios_vendor));
    safe_read_file("/sys/class/dmi/id/bios_version", info->bios_version, sizeof(info->bios_version));
    safe_read_file("/sys/class/dmi/id/bios_date", info->bios_date, sizeof(info->bios_date));

    if (!info->sys_vendor[0]) snprintf(info->sys_vendor, sizeof(info->sys_vendor), "Desconhecido");
    if (!info->product_name[0]) snprintf(info->product_name, sizeof(info->product_name), "Desconhecido");
    if (!info->board_vendor[0]) snprintf(info->board_vendor, sizeof(info->board_vendor), "Desconhecido");
    if (!info->board_name[0]) snprintf(info->board_name, sizeof(info->board_name), "Desconhecido");
    if (!info->bios_vendor[0]) snprintf(info->bios_vendor, sizeof(info->bios_vendor), "Desconhecido");
    if (!info->bios_version[0]) snprintf(info->bios_version, sizeof(info->bios_version), "Desconhecido");
    if (!info->bios_date[0]) snprintf(info->bios_date, sizeof(info->bios_date), "Desconhecido");

    // Fallback para arquiteturas ARM/DeviceTree (ex.: Raspberry Pi, Orange Pi)
    if (strcmp(info->sys_vendor, "Desconhecido") == 0 && strcmp(info->product_name, "Desconhecido") == 0) {
        char dt_model[128] = {0};
        safe_read_file("/sys/firmware/devicetree/base/model", dt_model, sizeof(dt_model));
        if (dt_model[0]) {
            snprintf(info->product_name, sizeof(info->product_name), "%s", dt_model);
            snprintf(info->sys_vendor, sizeof(info->sys_vendor), "DeviceTree");
            snprintf(info->board_name, sizeof(info->board_name), "%s", dt_model);
        }
        char dt_serial[64] = {0};
        safe_read_file("/sys/firmware/devicetree/base/serial-number", dt_serial, sizeof(dt_serial));
        if (dt_serial[0]) {
            snprintf(info->product_uuid, sizeof(info->product_uuid), "%s", dt_serial);
        }
    }

    // 2. Machine ID do Sistema (/etc/machine-id)
    safe_read_file("/etc/machine-id", info->machine_id, sizeof(info->machine_id));
    if (!info->machine_id[0]) {
        safe_read_file("/var/lib/dbus/machine-id", info->machine_id, sizeof(info->machine_id));
    }
    if (!info->machine_id[0]) {
        snprintf(info->machine_id, sizeof(info->machine_id), "Desconhecido");
    }

    /* 3. MACs físicos, ordenados para que a ordem do sysfs não altere o hash. */
    char macs[MAX_NETS][20];
    int mac_count = 0;
    if (net) {
        for (int i = 0; i < net->count; i++) {
            if (!net->interfaces[i].is_virtual && net->interfaces[i].mac[0] &&
                strcmp(net->interfaces[i].mac, "00:00:00:00:00:00") != 0) {
                snprintf(macs[mac_count++], sizeof(macs[0]), "%s", net->interfaces[i].mac);
            }
        }
    }
    for (int i = 0; i < mac_count; i++) {
        for (int j = i + 1; j < mac_count; j++) {
            if (strcmp(macs[i], macs[j]) > 0) {
                char tmp[20];
                memcpy(tmp, macs[i], sizeof(tmp));
                memcpy(macs[i], macs[j], sizeof(tmp));
                memcpy(macs[j], tmp, sizeof(tmp));
            }
        }
    }
    char mac_list[sizeof(macs) + 1] = "";
    for (int i = 0; i < mac_count; i++) {
        size_t used = strlen(mac_list);
        snprintf(mac_list + used, sizeof(mac_list) - used, "%s%s", i ? "," : "", macs[i]);
    }
    if (!mac_list[0]) snprintf(mac_list, sizeof(mac_list), "none");

    // 4. Constrói a semente canônica para o Hardware Fingerprint
    char seed[1024];
    snprintf(seed, sizeof(seed), "v2|CPU:%s|CORES:%d|DMI_SYS:%s|DMI_PROD:%s|DMI_BOARD:%s|MACS:%s|UUID:%s",
             cpu ? cpu->model : "unknown",
             cpu ? cpu->physical_cores : 0,
             info->sys_vendor,
             info->product_name,
             info->board_name,
             mac_list,
             info->product_uuid[0] ? info->product_uuid : "none");

    // 5. Calcula o hash criptográfico SHA-256
    sha256_string(seed, info->hardware_id);
    snprintf(info->fingerprint_version, sizeof(info->fingerprint_version), "v2");

    // 6. UUID Canônico Formatado
    if (info->product_uuid[0] && strlen(info->product_uuid) >= 32) {
        snprintf(info->formatted_uuid, sizeof(info->formatted_uuid), "%s", info->product_uuid);
    } else {
        snprintf(info->formatted_uuid, sizeof(info->formatted_uuid),
                 "%.8s-%.4s-%.4s-%.4s-%.12s",
                 info->hardware_id,
                 info->hardware_id + 8,
                 info->hardware_id + 12,
                 info->hardware_id + 16,
                 info->hardware_id + 20);
    }
}

// Estrutura auxiliar para cálculo de uso de CPU
struct CpuStat {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
};

// Lê as métricas atuais de CPU do /proc/stat
static void read_cpu_stats(struct CpuStat *total, struct CpuStat cores[], int *num_cores, int *truncated) {
    memset(total, 0, sizeof(*total));
    memset(cores, 0, sizeof(struct CpuStat) * MAX_CORES);
    *num_cores = 0;
    *truncated = 0;
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return;
    char line[256];
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
            } else {
                *truncated = 1;
            }
        }
    }
    fclose(f);
}

// Lê a temperatura atual da CPU/pacote a partir de /sys/class/thermal ou /sys/class/hwmon
static double read_cpu_temp(void) {
    char path[256], type[64], val_str[32];
    for (int i = 0; i < 16; i++) {
        snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/type", i);
        if (access(path, F_OK) != 0) break;
        safe_read_file(path, type, sizeof(type));
        if (strstr(type, "x86_pkg") || strstr(type, "coretemp") ||
            strstr(type, "cpu") || strstr(type, "k10temp") || strstr(type, "soc_thermal")) {
            snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", i);
            safe_read_file(path, val_str, sizeof(val_str));
            if (val_str[0]) {
                double temp = atof(val_str) / 1000.0;
                if (temp > 0.0 && temp < 150.0) return temp;
            }
        }
    }

    safe_read_file("/sys/class/thermal/thermal_zone0/temp", val_str, sizeof(val_str));
    if (val_str[0]) {
        double temp = atof(val_str) / 1000.0;
        if (temp > 0.0 && temp < 150.0) return temp;
    }

    for (int i = 0; i < 8; i++) {
        snprintf(path, sizeof(path), "/sys/class/hwmon/hwmon%d/name", i);
        if (access(path, F_OK) != 0) continue;
        safe_read_file(path, type, sizeof(type));
        if (strstr(type, "coretemp") || strstr(type, "k10temp") || strstr(type, "cpu")) {
            snprintf(path, sizeof(path), "/sys/class/hwmon/hwmon%d/temp1_input", i);
            safe_read_file(path, val_str, sizeof(val_str));
            if (val_str[0]) {
                double temp = atof(val_str) / 1000.0;
                if (temp > 0.0 && temp < 150.0) return temp;
            }
        }
    }

    return -1.0;
}

// Coleta informações detalhadas do processador (com opção de fast_mode)
void collect_cpu_info_ex(CpuInfo *info, int fast_mode) {
    memset(info, 0, sizeof(*info));
    info->temp_celsius = read_cpu_temp();

    // Lógica para contar núcleos lógicos e físicos, além do modelo
    int phys_ids[MAX_CORES];
    int core_ids[MAX_CORES];
    int pair_count = 0;
    int logical_threads = 0;
    double fallback_mhz = 0.0;

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
            } else if (strncmp(line, "cpu MHz", 7) == 0) {
                if (fallback_mhz == 0.0) {
                    char *colon = strchr(line, ':');
                    if (colon) fallback_mhz = atof(colon + 1);
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

    // Fallback de frequência via /proc/cpuinfo caso cpufreq não esteja disponível (ex.: VMs)
    if (info->freq_current_mhz == 0.0 && fallback_mhz > 0.0) {
        info->freq_current_mhz = fallback_mhz;
    }

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
    if (fast_mode) {
        info->usage_total = 0.0;
        info->num_cores_sampled = 0;
        return;
    }

    struct CpuStat total1, total2;
    struct CpuStat cores1[MAX_CORES], cores2[MAX_CORES];
    int num_cores1 = 0, num_cores2 = 0, truncated1 = 0, truncated2 = 0;

    read_cpu_stats(&total1, cores1, &num_cores1, &truncated1);
    usleep(200000); // 200ms
    read_cpu_stats(&total2, cores2, &num_cores2, &truncated2);

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
    info->cores_truncated = truncated1 || truncated2;

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

// Wrapper padrão para manter compatibilidade binária/assinatura
void collect_cpu_info(CpuInfo *info) {
    collect_cpu_info_ex(info, 0);
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

    /* ─── Coleta dispositivos swap individuais de /proc/swaps ─── */
    info->swap_count = 0;
    FILE *sf = fopen("/proc/swaps", "r");
    if (sf) {
        char sline[512];
        /* Pula o cabeçalho */
        if (fgets(sline, sizeof(sline), sf)) {
            while (fgets(sline, sizeof(sline), sf)) {
                if (info->swap_count >= MAX_SWAPS) {
                    info->swaps_truncated = 1;
                    break;
                }
                SwapEntry *se = &info->swaps[info->swap_count];
                char dev[128], type[32];
                unsigned long long size_val = 0, used_val = 0;
                int prio = 0;

                if (sscanf(sline, "%127s %31s %llu %llu %d", dev, type, &size_val, &used_val, &prio) >= 3) {
                    snprintf(se->device, sizeof(se->device), "%s", dev);
                    se->size_kb = size_val;
                    se->used_kb = used_val;
                    se->priority = prio;

                    /* Identifica o tipo real do swap */
                    if (strstr(dev, "zram") != NULL) {
                        snprintf(se->type, sizeof(se->type), "zram");
                    } else if (strcmp(type, "partition") == 0) {
                        snprintf(se->type, sizeof(se->type), "partition");
                    } else if (strcmp(type, "file") == 0) {
                        snprintf(se->type, sizeof(se->type), "file");
                    } else {
                        snprintf(se->type, sizeof(se->type), "%s", type);
                    }

                    info->swap_count++;
                }
            }
        }
        fclose(sf);
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
        
        if (strncmp(class_str, "0x0300", 6) == 0 || strncmp(class_str, "0x0302", 6) == 0 || strncmp(class_str, "0x0380", 6) == 0) {
            if (info->count >= MAX_GPUS) { info->truncated = 1; break; }
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
            else if (strncmp(vendor_str, "0x1af4", 6) == 0) vendor_name = "VirtIO";
            else if (strncmp(vendor_str, "0x15ad", 6) == 0) vendor_name = "VMware";
            else if (strncmp(vendor_str, "0x1234", 6) == 0) vendor_name = "QEMU";
            else if (strncmp(vendor_str, "0x1b36", 6) == 0) vendor_name = "Red Hat (QXL)";
            else if (strncmp(vendor_str, "0x102b", 6) == 0) vendor_name = "Matrox";
            else if (strncmp(vendor_str, "0x1414", 6) == 0) vendor_name = "Microsoft (Hyper-V)";
            else if (strncmp(vendor_str, "0x1ab8", 6) == 0) vendor_name = "Parallels";

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
        // Ignora dispositivos de bloco virtuais ou não relevantes (incluindo zram e loop)
        if (strncmp(dn, "loop", 4) == 0 || strncmp(dn, "ram", 3) == 0 ||
            strncmp(dn, "dm-", 3) == 0 || strncmp(dn, "sr", 2) == 0 ||
            strncmp(dn, "fd", 2) == 0 || strncmp(dn, "zram", 4) == 0) {
            continue;
        }

        if (info->count >= MAX_DISKS) { info->truncated = 1; break; }
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
            // Filtrar para mostrar somente montagens de dispositivos físicos reais (ignora loop e snaps)
            if (strncmp(device, "/dev/", 5) != 0) continue;
            if (strncmp(device, "/dev/loop", 9) == 0) continue;

            const char *skip_fs[] = {"tmpfs", "devtmpfs", "sysfs", "proc", "devpts", "securityfs", "cgroup", "pstore", "debugfs", "hugetlbfs", "mqueue", "configfs", "fusectl", "overlay", "squashfs", NULL};
            int skip = 0;
            for (int i = 0; skip_fs[i] != NULL; i++) {
                if (strcmp(fs_type, skip_fs[i]) == 0) {
                    skip = 1; break;
                }
            }
            if (skip) continue;

            if (info->count >= MAX_MOUNTS) { info->truncated = 1; break; }
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

// Coleta informações sobre interfaces de rede do sistema via /sys/class/net e getifaddrs
void collect_net_info(NetInfo *info) {
    memset(info, 0, sizeof(*info));
    DIR *d = opendir("/sys/class/net");
    if (!d) return;

    NetEntry temp_all[128];
    int temp_count = 0;
    int temp_truncated = 0;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        if (temp_count >= 128) { temp_truncated = 1; break; }

        NetEntry *net = &temp_all[temp_count];
        memset(net, 0, sizeof(*net));
        snprintf(net->name, sizeof(net->name), "%s", dir->d_name);

        /* MAC Address */
        char path[512], buf[256];
        snprintf(path, sizeof(path), "/sys/class/net/%s/address", dir->d_name);
        safe_read_file(path, net->mac, sizeof(net->mac));
        if (!net->mac[0]) snprintf(net->mac, sizeof(net->mac), "00:00:00:00:00:00");

        /* Estado operacional */
        snprintf(path, sizeof(path), "/sys/class/net/%s/operstate", dir->d_name);
        safe_read_file(path, net->state, sizeof(net->state));
        if (!net->state[0]) snprintf(net->state, sizeof(net->state), "unknown");

        /* Verifica se é wireless (existência de /sys/class/net/<iface>/wireless ou phy80211) */
        snprintf(path, sizeof(path), "/sys/class/net/%s/wireless", dir->d_name);
        if (access(path, F_OK) == 0) {
            net->is_wireless = 1;
        } else {
            snprintf(path, sizeof(path), "/sys/class/net/%s/phy80211", dir->d_name);
            net->is_wireless = (access(path, F_OK) == 0) ? 1 : 0;
        }

        /* Classifica se é virtual */
        if (strcmp(dir->d_name, "lo") == 0 ||
            strncmp(dir->d_name, "veth", 4) == 0 ||
            strncmp(dir->d_name, "br-", 3) == 0 ||
            strncmp(dir->d_name, "docker", 6) == 0 ||
            strncmp(dir->d_name, "virbr", 5) == 0 ||
            strncmp(dir->d_name, "dummy", 5) == 0) {
            net->is_virtual = 1;
        } else {
            char dev_link[512];
            snprintf(dev_link, sizeof(dev_link), "/sys/class/net/%s/device", dir->d_name);
            net->is_virtual = (access(dev_link, F_OK) != 0);
        }

        /* Tipo da interface */
        snprintf(path, sizeof(path), "/sys/class/net/%s/type", dir->d_name);
        safe_read_file(path, buf, sizeof(buf));
        int if_type = buf[0] ? atoi(buf) : -1;

        if (strcmp(dir->d_name, "lo") == 0) {
            snprintf(net->type_desc, sizeof(net->type_desc), "Loopback");
        } else if (net->is_wireless) {
            snprintf(net->type_desc, sizeof(net->type_desc), "Wi-Fi");
        } else if (if_type == 1) {
            snprintf(path, sizeof(path), "/sys/class/net/%s/bridge", dir->d_name);
            if (access(path, F_OK) == 0) {
                snprintf(net->type_desc, sizeof(net->type_desc), "Bridge");
            } else if (!net->is_virtual) {
                snprintf(net->type_desc, sizeof(net->type_desc), "Ethernet");
            } else {
                snprintf(net->type_desc, sizeof(net->type_desc), "Virtual");
            }
        } else if (if_type == 772) {
            snprintf(net->type_desc, sizeof(net->type_desc), "Loopback");
        } else {
            snprintf(net->type_desc, sizeof(net->type_desc), "Outro (%d)", if_type);
        }

        /* Driver — segue o symlink /sys/class/net/<iface>/device/driver */
        snprintf(path, sizeof(path), "/sys/class/net/%s/device/driver", dir->d_name);
        char driver_target[512];
        ssize_t len = readlink(path, driver_target, sizeof(driver_target) - 1);
        if (len != -1) {
            driver_target[len] = '\0';
            char *basename_ptr = strrchr(driver_target, '/');
            if (basename_ptr)
                snprintf(net->driver, sizeof(net->driver), "%s", basename_ptr + 1);
            else
                snprintf(net->driver, sizeof(net->driver), "%s", driver_target);
        } else {
            snprintf(net->driver, sizeof(net->driver), "N/A");
        }

        /* Velocidade do link (Mbps) */
        snprintf(path, sizeof(path), "/sys/class/net/%s/speed", dir->d_name);
        safe_read_file(path, buf, sizeof(buf));
        if (buf[0] && atoi(buf) > 0) {
            net->speed_mbps = (unsigned long long)atoi(buf);
        } else {
            net->speed_mbps = 0;
        }

        net->ipv4[0] = '\0';
        net->ipv6[0] = '\0';

        temp_count++;
    }
    closedir(d);

    /* Prioriza interfaces físicas antes de interfaces virtuais */
    for (int i = 0; i < temp_count; i++) {
        if (!temp_all[i].is_virtual) {
            if (info->count < MAX_NETS) {
                info->interfaces[info->count++] = temp_all[i];
            } else {
                info->truncated = 1;
            }
        }
    }
    for (int i = 0; i < temp_count; i++) {
        if (temp_all[i].is_virtual) {
            if (info->count < MAX_NETS) {
                info->interfaces[info->count++] = temp_all[i];
            } else {
                info->truncated = 1;
            }
        }
    }
    if (temp_truncated) info->truncated = 1;

    /* Coleta endereços IPv4 e IPv6 atribuídos para cada interface via getifaddrs */
    struct ifaddrs *ifaddr = NULL, *ifa = NULL;
    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (!ifa->ifa_name || !ifa->ifa_addr) continue;

            // Procura a interface correspondente na struct
            for (int i = 0; i < info->count; i++) {
                NetEntry *net = &info->interfaces[i];
                if (strcmp(net->name, ifa->ifa_name) != 0) continue;

                int family = ifa->ifa_addr->sa_family;
                if (family == AF_INET && net->ipv4[0] == '\0') {
                    char ip[INET_ADDRSTRLEN];
                    struct sockaddr_in *p = (struct sockaddr_in *)ifa->ifa_addr;
                    if (inet_ntop(AF_INET, &p->sin_addr, ip, sizeof(ip))) {
                        // Calcula máscara CIDR se disponível
                        if (ifa->ifa_netmask) {
                            struct sockaddr_in *m = (struct sockaddr_in *)ifa->ifa_netmask;
                            uint32_t mask = ntohl(m->sin_addr.s_addr);
                            int cidr = 0;
                            while (mask) {
                                cidr += (mask & 1);
                                mask >>= 1;
                            }
                            snprintf(net->ipv4, sizeof(net->ipv4), "%s/%d", ip, cidr);
                        } else {
                            snprintf(net->ipv4, sizeof(net->ipv4), "%s", ip);
                        }
                    }
                } else if (family == AF_INET6) {
                    char ip6[INET6_ADDRSTRLEN];
                    struct sockaddr_in6 *p6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                    if (inet_ntop(AF_INET6, &p6->sin6_addr, ip6, sizeof(ip6))) {
                        // Prefere endereço IPv6 não link-local se já tivermos um
                        if (net->ipv6[0] == '\0' || strncmp(net->ipv6, "fe80", 4) == 0) {
                            snprintf(net->ipv6, sizeof(net->ipv6), "%s", ip6);
                        }
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }
}

// Coleta informações sobre adaptadores Bluetooth via /sys/class/bluetooth
void collect_bt_info(BtInfo *info) {
    memset(info, 0, sizeof(*info));
    DIR *d = opendir("/sys/class/bluetooth");
    if (!d) return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        if (info->count >= MAX_BT) { info->truncated = 1; break; }

        BtEntry *bt = &info->adapters[info->count];
        snprintf(bt->name, sizeof(bt->name), "%s", dir->d_name);

        /* Endereço Bluetooth */
        char path[512], buf[256];
        snprintf(path, sizeof(path), "/sys/class/bluetooth/%s/address", dir->d_name);
        safe_read_file(path, bt->address, sizeof(bt->address));
        if (!bt->address[0]) snprintf(bt->address, sizeof(bt->address), "N/A");

        /* Tipo (USB, UART, etc.) — checa o barramento do device */
        snprintf(path, sizeof(path), "/sys/class/bluetooth/%s/device/subsystem", dir->d_name);
        char subsys_target[512];
        ssize_t len = readlink(path, subsys_target, sizeof(subsys_target) - 1);
        if (len != -1) {
            subsys_target[len] = '\0';
            char *base = strrchr(subsys_target, '/');
            if (base) {
                base++;
                if (strcmp(base, "usb") == 0)
                    snprintf(bt->type_desc, sizeof(bt->type_desc), "USB");
                else if (strcmp(base, "sdio") == 0)
                    snprintf(bt->type_desc, sizeof(bt->type_desc), "SDIO");
                else if (strcmp(base, "serial") == 0 || strcmp(base, "platform") == 0)
                    snprintf(bt->type_desc, sizeof(bt->type_desc), "UART");
                else
                    snprintf(bt->type_desc, sizeof(bt->type_desc), "%s", base);
            } else {
                snprintf(bt->type_desc, sizeof(bt->type_desc), "Desconhecido");
            }
        } else {
            snprintf(bt->type_desc, sizeof(bt->type_desc), "Desconhecido");
        }

        /* Driver */
        snprintf(path, sizeof(path), "/sys/class/bluetooth/%s/device/driver", dir->d_name);
        char drv_target[512];
        len = readlink(path, drv_target, sizeof(drv_target) - 1);
        if (len != -1) {
            drv_target[len] = '\0';
            char *base = strrchr(drv_target, '/');
            if (base)
                snprintf(bt->driver, sizeof(bt->driver), "%s", base + 1);
            else
                snprintf(bt->driver, sizeof(bt->driver), "%s", drv_target);
        } else {
            snprintf(bt->driver, sizeof(bt->driver), "N/A");
        }

        /* Fabricante (via /sys/class/bluetooth/<hci>/device/../manufacturer ou product) */
        snprintf(path, sizeof(path), "/sys/class/bluetooth/%s/device/../product", dir->d_name);
        safe_read_file(path, buf, sizeof(buf));
        if (buf[0]) {
            snprintf(bt->manufacturer, sizeof(bt->manufacturer), "%s", buf);
        } else {
            snprintf(path, sizeof(path), "/sys/class/bluetooth/%s/device/../manufacturer", dir->d_name);
            safe_read_file(path, buf, sizeof(buf));
            if (buf[0]) {
                snprintf(bt->manufacturer, sizeof(bt->manufacturer), "%s", buf);
            } else {
                snprintf(bt->manufacturer, sizeof(bt->manufacturer), "Desconhecido");
            }
        }

        info->count++;
    }
    closedir(d);
}
