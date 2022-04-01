#include "sha256.h"

const char* IN_HASH = "01ba4719c80b6fe911b091a7c05124b64eeece964e09c058ef8f9805daca546b";
const char* EXPECTED_HASH = "9c827201b94019b42f85706bc49c59ff84b5604d11caafb90ab94856c4e1dd7a";

int test_sha256() {
    ap_uint<256> in_hash(IN_HASH, 16);
    ap_uint<256> expected_hash(EXPECTED_HASH, 16);

    std::cout << "Starting the core..." << std::endl;
    ap_uint<256> out_hash = sha256(in_hash);
    if (out_hash != expected_hash) {
        std::cout << "out_hash = " << out_hash.to_string(16, true).c_str() << std::endl;
        return 1;
    }
    std::cout << "Success!" << std::endl;
    return 0;
}

int main() {
    int ret;

    ret = test_sha256();
    if (ret) {
        return ret;
    }

    return 0;
}
