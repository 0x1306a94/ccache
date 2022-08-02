//
//  DigestCalculate.cpp
//  ccache
//
//  Created by king on 2022/8/2.
//

#include "DigestCalculate.hpp"

#include <boost/filesystem.hpp>
#include <iostream>

namespace fs = boost::filesystem;

namespace ccache {

DigestCalculate::DigestCalculate() {
    MD5_Init(&m_ctx);
    std::cout << __FUNCTION__ << std::endl;
}

DigestCalculate::~DigestCalculate() {
    std::cout << __FUNCTION__ << std::endl;
}

void DigestCalculate::Update(const void *data, size_t len) {
    MD5_Update(&m_ctx, data, len);
}

void DigestCalculate::Update(const std::string &content) {
    MD5_Update(&m_ctx, content.c_str(), strlen(content.c_str()));
}

void DigestCalculate::UpdateFormFile(const std::string &path) {
    if (!fs::exists(path)) {
        std::cout << "ccache: DigestCalculate file not exists \n"
                  << path
                  << std::endl;
        return;
    }

    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        return;
    }
#define BUFFER_SIZE 1024
    unsigned char data[BUFFER_SIZE];
    size_t ret = 0;
    while (true) {
        ret = fread(data, BUFFER_SIZE, 1, fp);
        if (ret == EOF) {
            break;
        }
        if (ret == 0) {
            break;
        }
        MD5_Update(&m_ctx, (void *)data, ret);
    }
    fclose(fp);
    fp = NULL;
#undef BUFFER_SIZE
}

void DigestCalculate::Final() {
    unsigned char buffer[MD5_DIGEST_LENGTH];
    MD5_Final(buffer, &m_ctx);

    char md5_key[MD5_DIGEST_LENGTH * 2 + 1] = {'\0'};
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(md5_key + i * 2, "%02x", buffer[i]);
    }

    m_digest = md5_key;
}
};  // namespace ccache

