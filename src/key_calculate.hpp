//
//  KeyCalculate.hpp
//  ccache
//
//  Created by king on 2022/8/2.
//

#ifndef KeyCalculate_hpp
#define KeyCalculate_hpp

#include <string>

namespace ccache {
class ProjectConfig;
class Impl;
class KeyCalculate {
  public:
    KeyCalculate(ProjectConfig &config);
    ~KeyCalculate();

    void Init();
    void UpdateCompileArgs(int argc, const char *const *argv);
    void Update(const void *data, size_t len);
    void Update(const std::string &content);
    void UpdateFormFile(const std::string &path);

    void Final();

    std::string Digest() { return m_digest; }

  private:
    std::unique_ptr<Impl> PImpl;
    ProjectConfig &m_config;
    std::string m_digest;
};
};     // namespace ccache
#endif /* KeyCalculate_hpp */

