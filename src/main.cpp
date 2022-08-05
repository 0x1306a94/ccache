
#include <execinfo.h>
#include <iostream>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <yaml-cpp/yaml.h>

#include "ccache.hpp"
#include "config.hpp"
#include "env_key.h"
#include "key_calculate.hpp"

void handler(int sig) {

#define BACK_TRACE_DEPTH 512
	void *array[BACK_TRACE_DEPTH];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, BACK_TRACE_DEPTH);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);

	uintptr_t vmaddr_slide = 0;
	for (uint32_t i = 0; i < _dyld_image_count(); i++) {
		const char *image_name = (char *)_dyld_get_image_name(i);
		const struct mach_header *header = _dyld_get_image_header(i);
		if (header->filetype == MH_EXECUTE) {
			vmaddr_slide = _dyld_get_image_vmaddr_slide(i);
			fprintf(stderr, "Image name %s at address 0x%llx and ALSR silde 0x%lx \n", image_name, (mach_vm_address_t)header, vmaddr_slide);
		}
	}

	exit(sig);
}

int main(int argc, char *const *argv) {

	signal(SIGILL, handler);
	signal(SIGABRT, handler);
	signal(SIGBUS, handler);  // install our handler
	signal(SIGSEGV, handler);
	signal(SIGSYS, handler);

	//    std::unique_ptr<ccache::KeyCalculate> KeyCalculate(new ccache::KeyCalculate);
	//    KeyCalculate->Init();
	//
	//    KeyCalculate->Update("ccache");
	//    KeyCalculate->Final();
	//
	//    std::cout << "md5: " << KeyCalculate->Digest() << "\n";

	const char *config_file_path = getenv(CCACHE_ENV_CONFIG_KEY);
	if (config_file_path == NULL) {
		std::cout << "Set the CCACHE_CONFIG environment variable first" << std::endl;
		return EXIT_FAILURE;
	}

	ccache::Config config = YAML::LoadFile(config_file_path).as<ccache::Config>();
	config.replace_environment_variables();
	//    std::stringstream ss;
	//    ss << YAML::Node(config);
	//
	//    std::cout << ss.str() << std::endl;

	std::unique_ptr<ccache::CCache> cache = std::make_unique<ccache::CCache>(config);
	int result = cache->compilation(argc, argv);
	return result;
}
