//
//  KeyCalculate.cpp
//  ccache
//
//  Created by king on 2022/8/2.
//

#include "key_calculate.hpp"

#include "config.hpp"
#include "fmtmacros.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <sstream>
#include <stdlib.h>
extern "C" {
#if USE_CACAHE_KEY_OPENSSL_MD5
#include <openssl/evp.h>
#elif USE_CACAHE_KEY_XXHASH
#include <xxhash.h>
#endif
};

namespace fs = boost::filesystem;

namespace ccache {

struct Impl {
    virtual ~Impl() = default;
    virtual void Init() = 0;
    virtual void Update(const void *data, size_t len) = 0;
    virtual void Update(const std::string &content) = 0;
    virtual std::string Final() = 0;

    static std::unique_ptr<Impl> CreateImpl();
};

KeyCalculate::KeyCalculate(Config &config)
    : m_config(config)
    , PImpl(Impl::CreateImpl()) {

    std::cout << __FUNCTION__ << std::endl;
}

KeyCalculate::~KeyCalculate() {
    std::cout << __FUNCTION__ << std::endl;
}

void KeyCalculate::Init() {

    PImpl->Init();
}

void KeyCalculate::UpdateCompileArgs(int argc, const char *const *argv) {
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

void KeyCalculate::Update(const void *data, size_t len) {
    PImpl->Update(data, len);
}

void KeyCalculate::Update(const std::string &content) {
    PImpl->Update(content);
}

void KeyCalculate::UpdateFormFile(const std::string &path) {

    if (!fs::exists(path)) {
        std::cout << "ccache: KeyCalculate file not exists \n"
                  << path
                  << std::endl;
        return;
    }

    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        return;
    }
#define BUFFER_SIZE 65536
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
        PImpl->Update(data, ret);
    }
    fclose(fp);
    fp = NULL;
#undef BUFFER_SIZE
}

void KeyCalculate::Final() {
    m_digest = PImpl->Final();
}

#if USE_CACAHE_KEY_OPENSSL_MD5
struct OPENSSL_MD5_IMP : Impl {
    EVP_MD_CTX *m_ctx{nullptr};

    ~OPENSSL_MD5_IMP() {
        std::cout << __FUNCTION__ << std::endl;
        if (m_ctx != nullptr) {
            EVP_MD_CTX_free(m_ctx);
            m_ctx = nullptr;
        }
    };

    void Init() override {
        if (m_ctx != nullptr) {
            EVP_MD_CTX_free(m_ctx);
            m_ctx = nullptr;
        }
        m_ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(m_ctx, EVP_md5(), NULL);
    }

    void Update(const void *data, size_t len) override {
        if (m_ctx == nullptr) {
            return;
        }
        EVP_DigestUpdate(m_ctx, data, len);
    }

    void Update(const std::string &content) override {
        if (m_ctx == nullptr) {
            return;
        }
        const char *str = content.c_str();
        EVP_DigestUpdate(m_ctx, str, strlen(str));
    }

    std::string Final() override {
        unsigned int md5_digest_len = EVP_MD_size(EVP_md5());

        unsigned char *md5_digest = (unsigned char *)OPENSSL_malloc(md5_digest_len);
        EVP_DigestFinal_ex(m_ctx, md5_digest, &md5_digest_len);
        EVP_MD_CTX_free(m_ctx);
        m_ctx = nullptr;

        char out[EVP_MAX_MD_SIZE * 2 + 1] = {'\0'};
        for (int i = 0; i < md5_digest_len; i++) {
            sprintf(out + i * 2, "%02x", md5_digest[i]);
        }

        OPENSSL_free(md5_digest);

        return std::string{out};
    }
};
#elif USE_CACAHE_KEY_XXHASH
struct XXHASH_IMP : Impl {
    XXH64_state_t *m_state{NULL};
    ~XXHASH_IMP() {
        if (m_state != NULL) {
            XXH64_freeState(m_state);
            m_state = NULL;
        }
        std::cout << __FUNCTION__ << std::endl;
    };

    void Init() override {
        if (m_state != NULL) {
            XXH64_freeState(m_state);
            m_state = NULL;
        }

        m_state = XXH64_createState();
        if (m_state == NULL) {
            abort();
        }

        /* Initialize state with selected seed */
        XXH64_hash_t const seed = 0; /* or any other value */
        if (XXH64_reset(m_state, seed) == XXH_ERROR) {
            abort();
        }
    }

    void Update(const void *data, size_t len) override {
        if (m_state == NULL) {
            return;
        }
        if (XXH64_update(m_state, data, len) == XXH_ERROR) {
            abort();
        }
    }

    void Update(const std::string &content) override {
        if (m_state == NULL) {
            return;
        }
        const char *str = content.c_str();
        if (XXH64_update(m_state, str, strlen(str)) == XXH_ERROR) {
            abort();
        }
    }

    std::string Final() override {
        if (m_state == NULL) {
            return std::string();
        }
        XXH64_hash_t const hash = XXH64_digest(m_state);
//        XXH64_canonical_t dst;
//        XXH64_canonicalFromHash(&dst, hash);
        XXH64_freeState(m_state);
        m_state = NULL;

        std::string hex = FMT("{:x}", hash);
        return hex;
    }
};
#endif

std::unique_ptr<Impl>
Impl::CreateImpl() {
#if USE_CACAHE_KEY_OPENSSL_MD5
    return std::make_unique<OPENSSL_MD5_IMP>();
#elif USE_CACAHE_KEY_XXHASH
    return std::make_unique<XXHASH_IMP>();
#endif
}
};  // namespace ccache

