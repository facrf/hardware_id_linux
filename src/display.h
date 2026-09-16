/*
 * display.h — Protótipos das funções de renderização TUI
 *
 * Interface de apresentação visual que recebe as structs preenchidas
 * pelos coletores e renderiza painéis formatados com Unicode e cores ANSI.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "collectors.h"

/* ─── Renderização de Painéis Individuais ─── */
void display_header(int width);
void display_hardware_id_panel(const HardwareIdInfo *info, int width, int include_identifiers);
void display_system_panel(const SysInfo *info, int width);
void display_cpu_panel(const CpuInfo *info, int width);
void display_memory_panel(const MemInfo *info, int width);
void display_gpu_panel(const GpuInfo *info, int width);
void display_storage_panel(const DiskInfo *disks, const MountInfo *mounts, int width);
void display_network_panel(const NetInfo *net, const BtInfo *bt, int width, int include_identifiers);
void display_footer(int width);

/* ─── Utilitários de Desenho ─── */

/* Retorna a largura atual do terminal (fallback: 80 colunas) */
int  get_terminal_width(void);
void set_color_enabled(int enabled);

/* Desenha a borda superior de um painel com título */
void draw_box_top(const char *title, int width);

/* Desenha a borda inferior de um painel */
void draw_box_bottom(int width);

/* Desenha uma linha separadora horizontal dentro de um painel */
void draw_separator(int width);

/* Imprime uma linha chave:valor alinhada dentro do painel */
void draw_kv(const char *key, const char *value, int width);

/* Desenha uma barra de progresso colorida com percentual */
void draw_progress_bar(const char *label, double percent, int width);

/* Retorna o código de cor ANSI apropriado para um percentual de uso:
 *   < 60%  → verde
 *   < 85%  → amarelo
 *   >= 85% → vermelho
 */
const char *usage_color(double percent);

#endif /* DISPLAY_H */
