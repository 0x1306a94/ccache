
#include <iostream>

#include "ccache.hpp"

int main(int argc, char *const *argv) {

	std::cout << "ccache" << std::endl;
    CCache cache(argc, argv);
    int result = cache.compilation();
	return result;
}
