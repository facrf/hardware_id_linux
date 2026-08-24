/*
 * colors.h — Definições de cores ANSI e estilos para terminal
 *
 * Macros para cores (foreground), negrito, reset e dim,
 * usadas pela camada de apresentação (display.c).
 */

#ifndef COLORS_H
#define COLORS_H

/* ─── Reset ─── */
#define RST       "\033[0m"

/* ─── Estilos ─── */
#define BOLD      "\033[1m"
#define DIM       "\033[2m"
#define UNDERLINE "\033[4m"

/* ─── Cores normais (foreground) ─── */
#define RED       "\033[0;31m"
#define GREEN     "\033[0;32m"
#define YELLOW    "\033[0;33m"
#define BLUE      "\033[0;34m"
#define MAGENTA   "\033[0;35m"
#define CYAN      "\033[0;36m"
#define WHITE     "\033[0;37m"

/* ─── Cores em negrito (foreground) ─── */
#define BRED      "\033[1;31m"
#define BGREEN    "\033[1;32m"
#define BYELLOW   "\033[1;33m"
#define BBLUE     "\033[1;34m"
#define BMAGENTA  "\033[1;35m"
#define BCYAN     "\033[1;36m"
#define BWHITE    "\033[1;37m"

/* ─── Cores de fundo (background) ─── */
#define BG_RED    "\033[41m"
#define BG_GREEN  "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE   "\033[44m"

#endif /* COLORS_H */
