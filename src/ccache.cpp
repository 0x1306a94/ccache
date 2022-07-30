//
//  ccache.cpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#include "ccache.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

extern "C" {
#include <openssl/md5.h>
};

CCache::CCache(int argc, const char *const *argv)
    : m_argc(argc)
    , m_argv(argv) {
    std::cout << __FUNCTION__ << std::endl;
    std::cout << "argc: " << m_argc << std::endl;
    std::cout << "argv: " << m_argv << std::endl;
}

CCache::~CCache() {
    std::cout << __FUNCTION__ << std::endl;
}

int CCache::compilation() {
    if (m_argc <= 1) {
        return EXIT_SUCCESS;
    }

    std::vector<char *> gen_depend_argv;

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    unsigned char md5_buffer[MD5_DIGEST_LENGTH];

    char *cache_dir = getenv("CCACHE_DIR");

    std::string home_dir(getenv("HOME"));
    home_dir.append("/Library/Developer/Xcode/DerivedData");

    std::string temp_dir("/var/folders/ccache/DerivedData");

    const char *source_file = NULL;
    for (int i = 1; i < m_argc; i++) {
        const char *argv = m_argv[i];
        if (strcmp(argv, "-o") == 0) {
            break;
        }

        if (strcmp(argv, "-c") == 0) {
            gen_depend_argv.push_back("-e");
            source_file = m_argv[++i];
            gen_depend_argv.push_back(const_cast<char *>(source_file));
            continue;
        }

        if (strncmp(m_argv[i], home_dir.data(), strlen(home_dir.data())) == 0) {
            std::string _argv(argv);
            _argv.replace(0, home_dir.length(), temp_dir);
            gen_depend_argv.push_back(const_cast<char *>(_argv.data()));
            continue;
        } else {
            gen_depend_argv.push_back(const_cast<char *>(argv));
        }
        MD5_Update(&md5_ctx, m_argv[i], strlen(m_argv[i]));
    }

    MD5_Final(md5_buffer, &md5_ctx);

    printf("cache key: ");
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02x", md5_buffer[i]);
    }
    printf("\n");

    char *const *argv = const_cast<char *const *>(gen_depend_argv.data());
    const char *path = argv[0];
    printf("path: %s\n", path);
    int status_code = execv(path, argv);
    if (status_code != 0) {
        return status_code;
    }
    return EXIT_SUCCESS;
}

