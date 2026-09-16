/*
 * colors.h — Definições de cores ANSI e estilos para terminal
 *
 * Macros para cores (foreground), negrito, reset e dim,
 * usadas pela camada de apresentação (display.c).
 */

#ifndef COLORS_H
#define COLORS_H

/* ─── Reset ─── */
extern int hardware_id_color_enabled;
#define ANSI(code) (hardware_id_color_enabled ? code : "")
#define RST       ANSI("\033[0m")

/* ─── Estilos ─── */
#define BOLD      ANSI("\033[1m")
#define DIM       ANSI("\033[2m")
#define UNDERLINE ANSI("\033[4m")

/* ─── Cores normais (foreground) ─── */
#define RED       ANSI("\033[0;31m")
#define GREEN     ANSI("\033[0;32m")
#define YELLOW    ANSI("\033[0;33m")
#define BLUE      ANSI("\033[0;34m")
#define MAGENTA   ANSI("\033[0;35m")
#define CYAN      ANSI("\033[0;36m")
#define WHITE     ANSI("\033[0;37m")

/* ─── Cores em negrito (foreground) ─── */
#define BRED      ANSI("\033[1;31m")
#define BGREEN    ANSI("\033[1;32m")
#define BYELLOW   ANSI("\033[1;33m")
#define BBLUE     ANSI("\033[1;34m")
#define BMAGENTA  ANSI("\033[1;35m")
#define BCYAN     ANSI("\033[1;36m")
#define BWHITE    ANSI("\033[1;37m")

/* ─── Cores de fundo (background) ─── */
#define BG_RED    ANSI("\033[41m")
#define BG_GREEN  ANSI("\033[42m")
#define BG_YELLOW ANSI("\033[43m")
#define BG_BLUE   ANSI("\033[44m")

#endif /* COLORS_H */
