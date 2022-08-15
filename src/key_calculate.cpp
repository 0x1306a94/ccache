//
//  KeyCalculate.cpp
//  ccache
//
//  Created by king on 2022/8/2.
//

#include "key_calculate.hpp"

#include "Fd.hpp"
#include "ProjectConfig.hpp"
#include "Util.hpp"
#include "configure.h"
#include "fmtmacros.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <sstream>
#include <stdlib.h>
extern "C" {
#if defined(USE_CACAHE_KEY_OPENSSL_MD5)
#include <openssl/evp.h>
#elif defined(USE_CACAHE_KEY_XXHASH)
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

KeyCalculate::KeyCalculate(ProjectConfig &config)
    : m_config(config)
    , PImpl(Impl::CreateImpl()) {

    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
}

KeyCalculate::~KeyCalculate() {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
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

    if (!Util::file_exists(path.c_str())) {
        BOOST_LOG_TRIVIAL(error) << "KeyCalculate file not exists";
        return;
    }

    FILE *fp = fopen(path.c_str(), "rt");
    if (fp == NULL) {
        BOOST_LOG_TRIVIAL(error) << "KeyCalculate file not open "
                                 << path;
        return;
    }

    BOOST_LOG_TRIVIAL(trace) << "KeyCalculate UpdateFormFile "
                             << path;

    Fd fd(fileno(fp));

#define BUFFER_SIZE 4096
    char temp_buffer[BUFFER_SIZE];
    while (true) {
        ssize_t ret = read(*fd, temp_buffer, BUFFER_SIZE);
        if (ret == EOF || ret == 0) {
            break;
        }

        BOOST_LOG_TRIVIAL(trace) << "KeyCalculate UpdateFormFile read size "
                                 << ret;
        PImpl->Update(temp_buffer, ret);
    }
#undef BUFFER_SIZE
}

void KeyCalculate::Final() {
    m_digest = PImpl->Final();
}

#if defined(USE_CACAHE_KEY_OPENSSL_MD5)
struct OPENSSL_MD5_IMP : Impl {
    EVP_MD_CTX *m_ctx{nullptr};
    OPENSSL_MD5_IMP() {
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    }

    ~OPENSSL_MD5_IMP() {
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
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

        std::stringstream ss;
        for (int idx = 0; idx < md5_digest_len; idx++) {
            ss << FMT("{:02x}", md5_digest[idx]);
        }

        OPENSSL_free(md5_digest);
        std::string md5_str = ss.str();
        return md5_str;
    }
};
#elif defined(USE_CACAHE_KEY_XXHASH)
struct XXHASH_IMP : Impl {
    XXH3_state_t *m_state{NULL};
    XXHASH_IMP() {
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    }

    ~XXHASH_IMP() {
        if (m_state != NULL) {
            XXH3_freeState(m_state);
            m_state = NULL;
        }
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    };

    void Init() override {
        if (m_state != NULL) {
            XXH3_freeState(m_state);
            m_state = NULL;
        }

        m_state = XXH3_createState();
        if (m_state == NULL) {
            abort();
        }

        /* Initialize state with selected seed */
        XXH64_hash_t const seed = 0; /* or any other value */
        if (XXH3_128bits_reset_withSeed(m_state, seed) == XXH_ERROR) {
            abort();
        }
    }

    void Update(const void *data, size_t len) override {
        if (m_state == NULL) {
            return;
        }
        if (XXH3_128bits_update(m_state, data, len) == XXH_ERROR) {
            abort();
        }
    }

    void Update(const std::string &content) override {
        if (m_state == NULL) {
            return;
        }
        const char *str = content.c_str();
        if (XXH3_128bits_update(m_state, str, strlen(str)) == XXH_ERROR) {
            abort();
        }
    }

    std::string Final() override {
        if (m_state == NULL) {
            return std::string();
        }
        XXH128_hash_t const hash = XXH3_128bits_digest(m_state);
        XXH128_canonical_t dst;
        XXH128_canonicalFromHash(&dst, hash);
        XXH3_freeState(m_state);
        m_state = NULL;

        size_t len = sizeof(XXH128_hash_t);
        std::stringstream ss;
#if XXH_CPU_LITTLE_ENDIAN
        for (size_t idx = len - 1; idx < len; idx--) {
            ss << FMT("{:02x}", dst.digest[idx]);
        }
#else
        for (size_t idx = 0; idx < len; idx++) {
            ss << FMT("{:02x}", dst.digest[idx]);
        }
#endif

        std::string hex = ss.str();
        return hex;
    }
};
#endif

std::unique_ptr<Impl>
Impl::CreateImpl() {
#if defined(USE_CACAHE_KEY_OPENSSL_MD5)
    return std::make_unique<OPENSSL_MD5_IMP>();
#elif defined(USE_CACAHE_KEY_XXHASH)
    return std::make_unique<XXHASH_IMP>();
#endif
}
};  // namespace ccache

