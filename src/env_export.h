/*
 * env_export.h — Exportação de variáveis de ambiente no formato shell KEY=VALUE
 */

#ifndef ENV_EXPORT_H
#define ENV_EXPORT_H

#include <stdio.h>
#include "collectors.h"

/*
 * Exporta variáveis de ambiente no formato chave="valor" para uso com
 * eval $(hardware_id --env) ou arquivos de configuração .env.
 */
void export_env(FILE *out,
                const SysInfo *sys,
                const HardwareIdInfo *hw_id,
                const CpuInfo *cpu,
                const MemInfo *mem,
                const NetInfo *net,
                int include_identifiers);

#endif /* ENV_EXPORT_H */
