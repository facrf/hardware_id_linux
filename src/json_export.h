/*
 * json_export.h — Exportação estruturada dos dados coletados em formato JSON
 */

#ifndef JSON_EXPORT_H
#define JSON_EXPORT_H

#include <stdio.h>
#include "collectors.h"

/*
 * Exporta todas as informações coletadas em formato JSON válido para a stream especificada (ex.: stdout).
 */
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
                 int include_identifiers);

#endif /* JSON_EXPORT_H */
