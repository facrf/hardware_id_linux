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

    puts("Collector utilities: OK");
    return 0;
}
