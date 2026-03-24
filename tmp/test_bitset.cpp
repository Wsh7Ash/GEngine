#include <bitset>
#include <iostream>
#include <cstdint>

static constexpr uint32_t MAX_COMPONENTS = 128;
using Signature = std::bitset<MAX_COMPONENTS>;

int main() {
    Signature s;
    s.set(1);
    std::cout << "Bitset size: " << s.size() << std::endl;
    return 0;
}
