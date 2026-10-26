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

    puts("Collector utilities: OK");
    return 0;
}
