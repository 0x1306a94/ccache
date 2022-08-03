//
//  DigestCalculate.cpp
//  ccache
//
//  Created by king on 2022/8/2.
//

#include "DigestCalculate.hpp"

#include "config.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <sstream>

namespace fs = boost::filesystem;

namespace ccache {

DigestCalculate::DigestCalculate(Config &config)
    : m_config(config) {
    std::cout << __FUNCTION__ << std::endl;
}

DigestCalculate::~DigestCalculate() {
    std::cout << __FUNCTION__ << std::endl;
    if (m_ctx != nullptr) {
        EVP_MD_CTX_free(m_ctx);
        m_ctx = nullptr;
    }
}

void DigestCalculate::Init() {
    if (m_ctx != nullptr) {
        EVP_MD_CTX_free(m_ctx);
        m_ctx = nullptr;
    }
    m_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(m_ctx, EVP_md5(), NULL);
}

void DigestCalculate::UpdateCompileArgs(int argc, const char *const *argv) {
    for (int i = 0; i < argc; i++) {
        std::string str = argv[i];
        for (std::string &prefix : m_config.ignore_path_prefix) {
            if (boost::contains(str, prefix)) {
                boost::replace_all(str, prefix, "");
            }
        }
        Update(str);
    }
}

void DigestCalculate::Update(const void *data, size_t len) {
    if (m_ctx == nullptr) {
        return;
    }
    EVP_DigestUpdate(m_ctx, data, len);
}

void DigestCalculate::Update(const std::string &content) {
    if (m_ctx == nullptr) {
        return;
    }
    EVP_DigestUpdate(m_ctx, content.c_str(), strlen(content.c_str()));
}

void DigestCalculate::UpdateFormFile(const std::string &path) {
    if (m_ctx == nullptr) {
        return;
    }

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
        EVP_DigestUpdate(m_ctx, (void *)data, ret);
    }
    fclose(fp);
    fp = NULL;
#undef BUFFER_SIZE
}

void DigestCalculate::Final() {

    unsigned int md5_digest_len = EVP_MD_size(EVP_md5());

    // MD5_Final
    unsigned char *md5_digest = (unsigned char *)OPENSSL_malloc(md5_digest_len);
    EVP_DigestFinal_ex(m_ctx, md5_digest, &md5_digest_len);
    EVP_MD_CTX_free(m_ctx);
    m_ctx = nullptr;

    char out[EVP_MAX_MD_SIZE * 2 + 1] = {'\0'};
    for (int i = 0; i < md5_digest_len; i++) {
        sprintf(out + i * 2, "%02x", md5_digest[i]);
    }

    OPENSSL_free(md5_digest);

    m_digest = out;
}
};  // namespace ccache

