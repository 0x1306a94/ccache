//
//  DigestCalculate.hpp
//  ccache
//
//  Created by king on 2022/8/2.
//

#ifndef DigestCalculate_hpp
#define DigestCalculate_hpp

#include <string>
extern "C" {
#include <openssl/evp.h>
};

namespace ccache {
class DigestCalculate {
  public:
    DigestCalculate();
    ~DigestCalculate();

    void Init();
    void Update(const void *data, size_t len);
    void Update(const std::string &content);
    void UpdateFormFile(const std::string &path);

    void Final();

    std::string Digest() { return m_digest; }

  private:
    std::string m_digest;
    EVP_MD_CTX *m_ctx{nullptr};
};
};     // namespace ccache
#endif /* DigestCalculate_hpp */

