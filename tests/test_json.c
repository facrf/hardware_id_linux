#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_export.h"

int main(void) {
    SysInfo sys = {0};
    HardwareIdInfo hw = {0};
    CpuInfo cpu = {0};
    MemInfo mem = {0};
    GpuInfo gpu = {0};
    DiskInfo disks = {0};
    MountInfo mounts = {0};
    NetInfo net = {0};
    BtInfo bt = {0};
    char *output = NULL;
    size_t output_size = 0;

    snprintf(sys.distro_name, sizeof(sys.distro_name), "A \"quoted\" distro\n");
    snprintf(sys.virtualization, sizeof(sys.virtualization), "Máquina Física (Bare Metal)");
    snprintf(hw.hardware_id, sizeof(hw.hardware_id), "secret");
    cpu.temp_celsius = 55.5;

    FILE *stream = open_memstream(&output, &output_size);
    assert(stream != NULL);
    export_json(stream, &sys, &hw, &cpu, &mem, &gpu, &disks, &mounts, &net, &bt, 0);
    fclose(stream);

    assert(strstr(output, "A " "\\" "\"" "quoted" "\\" "\"" " distro" "\\" "n") != NULL);
    assert(strstr(output, "\"fingerprint_sha256\": \"<redacted>\"") != NULL);
    assert(strstr(output, "\"virtualization\": \"Máquina Física (Bare Metal)\"") != NULL);
    assert(strstr(output, "\"temp_celsius\": 55.5") != NULL);
    free(output);

    // Teste com temp_celsius negativa (deve gerar null)
    cpu.temp_celsius = -1.0;
    stream = open_memstream(&output, &output_size);
    assert(stream != NULL);
    export_json(stream, &sys, &hw, &cpu, &mem, &gpu, &disks, &mounts, &net, &bt, 0);
    fclose(stream);
    assert(strstr(output, "\"temp_celsius\": null") != NULL);
    free(output);

    puts("JSON escaping, serialization and redaction: OK");
    return 0;
}
