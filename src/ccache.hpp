//
//  ccache.hpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#ifndef ccache_hpp
#define ccache_hpp

#include <yaml-cpp/yaml.h>

namespace ccache {

class Config;
class KeyCalculate;
class Context;

class CCache {

  public:
    CCache(Config &config);

    ~CCache();

    int compilation(int argc, const char *const *argv);

  private:
    Config &m_config;
    int m_argc;
    char **m_argv{nullptr};

    void init_log(Context &ctx);

    void initialize(Context &ctx, int argc, const char *const *argv);

    void calculate_args_md5(Context &ctx, KeyCalculate &calculate);
    bool calculate_dep_md5(KeyCalculate &calculate, const std::string &dep_file);

    std::pair<bool, std::string> do_cache_compilation(Context &ctx, int argc, const char *const *argv);

    static void find_compiler(Context &ctx);
};

}  // namespace ccache

#endif /* ccache_hpp */

