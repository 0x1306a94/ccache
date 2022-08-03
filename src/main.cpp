
#include <iostream>

#include <execinfo.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ccache.hpp"

#include "DigestCalculate.hpp"

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

    //    std::unique_ptr<ccache::DigestCalculate> digestCalculate(new ccache::DigestCalculate);
    //    digestCalculate->Init();
    //
    //    digestCalculate->Update("ccache");
    //    digestCalculate->Final();
    //
    //    std::cout << "md5: " << digestCalculate->Digest() << "\n";

    //    execv(argv[1], (argv + 1));
    //    char **clone_argv = (char **)malloc(sizeof(char **) * argc);
    //    for (int i = 0; i < argc; i++) {
    //        clone_argv[i] = strdup(argv[i]);
    //    }
    std::unique_ptr<ccache::CCache> cache = std::make_unique<ccache::CCache>();
    int result = cache->compilation(argc, argv);
    //    free(clone_argv);
    return result;
}

