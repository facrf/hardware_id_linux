#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sha256.h"

int main(void) {
    char hash[65];

    sha256_string("", hash);
    assert(strcmp(hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0);
    sha256_string("abc", hash);
    assert(strcmp(hash, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);
    sha256_string("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", hash);
    assert(strcmp(hash, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1") == 0);

    puts("SHA-256 vectors: OK");
    return 0;
}
