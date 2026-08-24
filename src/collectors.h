/*
 * collectors.h — Estruturas de dados e protótipos dos coletores de hardware/software
 *
 * Define todas as structs (SysInfo, CpuInfo, MemInfo, GpuInfo, DiskInfo, MountInfo)
 * e as assinaturas das funções de coleta que leem dados de /proc, /sys e chamadas POSIX.
 */

#ifndef COLLECTORS_H
#define COLLECTORS_H

#include <stddef.h>

/* ─── Limites ─── */
#define MAX_CORES   256
#define MAX_GPUS    8
#define MAX_DISKS   32
#define MAX_MOUNTS  64
#define MAX_SWAPS   16
#define MAX_NETS    32
#define MAX_BT      16

/* ═══════════════════════════════════════════════════════════════════════════
 *  Estruturas de Dados
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ─── Informações do Sistema Operacional e Software ─── */
typedef struct {
    char distro_name[128];
    char distro_version[64];
    char distro_codename[64];
    char kernel_version[128];
    char architecture[32];
    char hostname[256];
    char username[64];
    long uptime_seconds;
    char uptime_formatted[64];
    char shell[128];
    char compiler[128];
} SysInfo;

/* ─── Informações de Identificação Única e Placa-Mãe (DMI/SMBIOS) ─── */
typedef struct {
    char sys_vendor[128];      /* Fabricante do Sistema (ex.: Dell, Lenovo, ASUS) */
    char product_name[128];    /* Modelo do Produto */
    char product_version[64];  /* Versão do Produto */
    char product_uuid[64];     /* UUID DMI (se com root) */
    char board_vendor[128];    /* Fabricante da Placa-Mãe */
    char board_name[128];      /* Modelo da Placa-Mãe */
    char bios_vendor[128];     /* Fabricante da BIOS/UEFI */
    char bios_version[64];     /* Versão da BIOS */
    char bios_date[32];        /* Data da BIOS */
    char machine_id[64];       /* /etc/machine-id */
    char hardware_id[65];      /* SHA-256 Hash único da impressão digital de hardware */
    char formatted_uuid[64];   /* UUID canônico sintético/DMI */
} HardwareIdInfo;

/* ─── Informações do Processador (CPU) ─── */
typedef struct {
    char model[256];
    int  physical_cores;
    int  logical_threads;
    double freq_current_mhz;
    double freq_min_mhz;
    double freq_max_mhz;
    char cache_l1d[32];
    char cache_l1i[32];
    char cache_l2[32];
    char cache_l3[32];
    double usage_total;
    double usage_per_core[MAX_CORES];
    int  num_cores_sampled;
} CpuInfo;

/* ─── Entrada de dispositivo Swap individual ─── */
typedef struct {
    char device[128];          /* ex.: /dev/zram0, /dev/sda2, /swapfile */
    char type[32];             /* partition, file, zram */
    int  priority;
    unsigned long long size_kb;
    unsigned long long used_kb;
} SwapEntry;

/* ─── Informações de Memória RAM e Swap ─── */
typedef struct {
    unsigned long long total_kb;
    unsigned long long free_kb;
    unsigned long long available_kb;
    unsigned long long buffers_kb;
    unsigned long long cached_kb;
    unsigned long long used_kb;          /* calculado: total - free - buffers - cached */
    unsigned long long swap_total_kb;
    unsigned long long swap_free_kb;
    unsigned long long swap_used_kb;     /* calculado: swap_total - swap_free */
    double ram_usage_percent;
    double swap_usage_percent;
    SwapEntry swaps[MAX_SWAPS];
    int swap_count;
} MemInfo;

/* ─── Informações de GPU (uma entrada por placa) ─── */
typedef struct {
    char name[256];
    char driver[64];
    char pci_slot[32];
} GpuEntry;

typedef struct {
    GpuEntry gpus[MAX_GPUS];
    int count;
} GpuInfo;

/* ─── Informações de Disco Físico (block device) ─── */
typedef struct {
    char device[64];           /* ex.: sda, nvme0n1 */
    char model[128];
    char type[16];             /* SSD, HDD ou NVMe */
    unsigned long long size_bytes;
} DiskEntry;

typedef struct {
    DiskEntry disks[MAX_DISKS];
    int count;
} DiskInfo;

/* ─── Informações de Montagem / Sistemas de Arquivos ─── */
typedef struct {
    char device[128];
    char mount_point[256];
    char fs_type[32];
    unsigned long long total_bytes;
    unsigned long long used_bytes;
    unsigned long long free_bytes;
    double usage_percent;
} MountEntry;

typedef struct {
    MountEntry mounts[MAX_MOUNTS];
    int count;
} MountInfo;

/* ─── Informações de Interface de Rede ─── */
typedef struct {
    char name[32];             /* ex.: eth0, wlan0, enp3s0 */
    char driver[64];           /* ex.: e1000e, iwlwifi, r8169 */
    char mac[20];              /* endereço MAC */
    char ipv4[64];             /* endereço IPv4 principal com máscara */
    char ipv6[64];             /* endereço IPv6 principal */
    char state[16];            /* up, down, unknown */
    char type_desc[32];        /* Ethernet, Wi-Fi, Loopback, Virtual, Bridge */
    int  is_wireless;
    int  is_virtual;           /* 1 se for loopback, veth, bridge, docker, etc. */
    unsigned long long speed_mbps; /* velocidade do link em Mbps (0 se indisponível) */
} NetEntry;

typedef struct {
    NetEntry interfaces[MAX_NETS];
    int count;
} NetInfo;

/* ─── Informações de Bluetooth ─── */
typedef struct {
    char name[64];             /* ex.: hci0 */
    char address[20];          /* endereço Bluetooth */
    char type_desc[32];        /* ex.: USB, UART */
    char driver[64];           /* driver do controlador */
    char manufacturer[64];     /* fabricante (se disponível) */
} BtEntry;

typedef struct {
    BtEntry adapters[MAX_BT];
    int count;
} BtInfo;


/* ═══════════════════════════════════════════════════════════════════════════
 *  Funções de Coleta — cada uma popula a struct correspondente
 * ═══════════════════════════════════════════════════════════════════════════ */

void collect_system_info(SysInfo *info);
void collect_hardware_id_info(HardwareIdInfo *info, const CpuInfo *cpu, const NetInfo *net);
void collect_cpu_info(CpuInfo *info);
void collect_memory_info(MemInfo *info);
void collect_gpu_info(GpuInfo *info);
void collect_disk_info(DiskInfo *info);
void collect_mount_info(MountInfo *info);
void collect_net_info(NetInfo *info);
void collect_bt_info(BtInfo *info);


/* ═══════════════════════════════════════════════════════════════════════════
 *  Utilidades comuns
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * Lê o conteúdo de um arquivo de texto para um buffer, de forma segura.
 * Garante terminação nula e não trava caso o arquivo não exista.
 */
void safe_read_file(const char *path, char *buf, size_t buf_size);

/*
 * Formata um valor em bytes para uma string legível (ex.: "1.50 GiB", "256 MiB").
 * Usa unidades binárias (KiB, MiB, GiB, TiB).
 */
void format_bytes(unsigned long long bytes, char *buf, size_t buf_size);

/*
 * Formata um valor em kilobytes para uma string legível.
 */
void format_kb(unsigned long long kb, char *buf, size_t buf_size);

/*
 * Remove espaços em branco no início e no fim de uma string (in-place).
 */
void trim_string(char *str);

#endif /* COLLECTORS_H */
