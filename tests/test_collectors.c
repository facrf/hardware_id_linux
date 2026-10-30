#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "collectors.h"

int main(void) {
    char empty[] = "";
    char padded[] = " \tvalor\n";

    trim_string(empty);
    assert(strcmp(empty, "") == 0);
    trim_string(padded);
    assert(strcmp(padded, "valor") == 0);

    char buf[64];
    format_bytes(500, buf, sizeof(buf));
    assert(strcmp(buf, "500 B") == 0);
    format_bytes(1024 * 1024 * 2ULL, buf, sizeof(buf));
    assert(strcmp(buf, "2.00 MiB") == 0);
    format_kb(1024ULL, buf, sizeof(buf));
    assert(strcmp(buf, "1.00 MiB") == 0);

    // Teste de fast mode no CPU collector
    CpuInfo cpu;
    collect_cpu_info_ex(&cpu, 1);
    assert(cpu.usage_total == 0.0);
    assert(cpu.num_cores_sampled == 0);

    // Teste de SysInfo com detecção de virtualização
    SysInfo sys;
    collect_system_info(&sys);
    assert(strlen(sys.virtualization) > 0);

    puts("Collector utilities & fast mode: OK");
    return 0;
}
