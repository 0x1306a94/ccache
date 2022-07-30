//
//  ccache.cpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#include "ccache.hpp"

#include <errno.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

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

    std::vector<std::string> gen_depend_argv;

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    unsigned char md5_buffer[MD5_DIGEST_LENGTH];

    char *cache_dir = getenv("CCACHE_DIR");

    std::string home_dir(getenv("HOME"));
    home_dir.append("/Library/Developer/Xcode/DerivedData");

    std::string temp_dir("/tmp/ccache/DerivedData");
    fs::path depend_file_path;
    fs::path source_file;

    for (int i = 1; i < m_argc; i++) {
        const char *argv = m_argv[i];
        //        printf("argv: %s\n", argv);
        if (strcmp(argv, "-o") == 0) {
            break;
        }

        if (strcmp(argv, "-c") == 0) {
            gen_depend_argv.push_back("-e");
            source_file = m_argv[++i];
            gen_depend_argv.push_back(source_file);
            continue;
        }

        if (strncmp(argv, home_dir.data(), strlen(home_dir.data())) == 0) {
            std::string _argv(argv);
            _argv.replace(0, home_dir.length(), temp_dir);
            if (strcmp(m_argv[i - 1], "-MF") == 0) {
                depend_file_path = _argv;
            }
            gen_depend_argv.push_back(_argv);
            continue;
        } else {
            gen_depend_argv.push_back(argv);
        }
        MD5_Update(&md5_ctx, m_argv[i], strlen(m_argv[i]));
    }

#if 0
    char *const *argv = const_cast<char *const *>(gen_depend_argv.data());
    const char *path = argv[0];
    printf("path: %s\n", path);
    for (const char *item : gen_depend_argv) {
        printf("%s ", item);
    }
    printf("\n");
    int status_code = execv(path, argv);
    if (status_code != 0) {
        return status_code;
    }
#else
    std::string command;
    for (std::string item : gen_depend_argv) {
        command.append(item);
        command.append(" ");
    }

    command.erase(command.end() - 1, command.end());
    std::cout << command << std::endl;

    fs::path depend_dir = depend_file_path.parent_path();
    if (!fs::exists(depend_dir)) {
        //        fs::create_directory(depend_dir);
        std::string command{"mkdir -p "};
        command.append(depend_dir.string());
        system(command.c_str());
    }
    //    int status_code = system(command.c_str());
    //    if (status_code != 0) {
    //        return status_code;
    //    }

    std::vector<std::string> names;
    std::ifstream ifs(depend_file_path.string(), std::ios::in);
    if (!ifs.is_open()) {
        std::cout << "Failed to open file.\n";
    } else {
        std::string s;
        while (std::getline(ifs, s)) {
            auto start = s.find("/");
            auto end = s.find("\\");
            if (start == std::string::npos) {
                continue;
            }
            if (end == std::string::npos) {
                end = s.length();
            } else {
                end -= 1;
            }
            auto ss = s.substr(start, end - start);
            auto time = fs::last_write_time(ss);
            std::cout << "ss::" << ss << " time: ";

            struct stat t_stat;
            stat(ss.c_str(), &t_stat);
            struct tm *timeinfo = localtime(&t_stat.st_mtime);  // or gmtime() depending on what you want
            printf("%lu\n", timeinfo->tm_gmtoff);
        }
        ifs.close();
    }
    ifs.close();

    MD5_Final(md5_buffer, &md5_ctx);

    printf("cache key: ");
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02x", md5_buffer[i]);
    }
    printf("\n");
#endif

    return EXIT_SUCCESS;
}

