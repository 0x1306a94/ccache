
#include <iostream>

#include "ccache.hpp"

int main(int argc, char *const *argv) {

    std::cout << "ccache" << std::endl;
    ccache::CCache cache;
    int result = cache.compilation(argc, argv);
    return result;
}

