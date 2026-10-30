#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env_export.h"

int main(void) {
    SysInfo sys = {0};
    HardwareIdInfo hw = {0};
    CpuInfo cpu = {0};
    MemInfo mem = {0};
    NetInfo net = {0};
    char *output = NULL;
    size_t output_size = 0;

    snprintf(sys.distro_name, sizeof(sys.distro_name), "Ubuntu");
    snprintf(sys.kernel_version, sizeof(sys.kernel_version), "7.0.0-generic");
    snprintf(sys.virtualization, sizeof(sys.virtualization), "Máquina Física (Bare Metal)");
    snprintf(hw.hardware_id, sizeof(hw.hardware_id), "abcdef123456");
    snprintf(hw.formatted_uuid, sizeof(hw.formatted_uuid), "1111-2222-3333-4444");
    snprintf(net.default_iface, sizeof(net.default_iface), "eth0");
    snprintf(net.default_gateway, sizeof(net.default_gateway), "192.168.1.1");
    cpu.physical_cores = 4;
    cpu.logical_threads = 8;
    cpu.freq_current_mhz = 2500.0;
    cpu.temp_celsius = 50.0;
    mem.total_kb = 16000000;
    mem.used_kb = 8000000;

    // Teste 1: Redacted
    FILE *stream = open_memstream(&output, &output_size);
    assert(stream != NULL);
    export_env(stream, &sys, &hw, &cpu, &mem, &net, 0);
    fclose(stream);

    assert(strstr(output, "HW_ID=\"<redacted>\"") != NULL);
    assert(strstr(output, "HW_DISTRO_NAME=\"Ubuntu\"") != NULL);
    assert(strstr(output, "HW_DEFAULT_INTERFACE=\"eth0\"") != NULL);
    assert(strstr(output, "HW_DEFAULT_GATEWAY=\"<redacted>\"") != NULL);
    assert(strstr(output, "HW_CPU_TEMP_CELSIUS=50.0") != NULL);
    free(output);

    // Teste 2: Include Identifiers
    output = NULL;
    output_size = 0;
    stream = open_memstream(&output, &output_size);
    assert(stream != NULL);
    export_env(stream, &sys, &hw, &cpu, &mem, &net, 1);
    fclose(stream);

    assert(strstr(output, "HW_ID=\"abcdef123456\"") != NULL);
    assert(strstr(output, "HW_UUID=\"1111-2222-3333-4444\"") != NULL);
    assert(strstr(output, "HW_DEFAULT_GATEWAY=\"192.168.1.1\"") != NULL);
    free(output);

    puts("ENV variables export and escaping: OK");
    return 0;
}
