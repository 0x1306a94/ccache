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

class ProjectConfig;
class KeyCalculate;
class Context;

class CCache {

  public:
    CCache(ProjectConfig &config);

    ~CCache();

    int compilation(Context &ctx, int argc, const char *const *argv);

  private:
    ProjectConfig &m_config;

    void initialize(Context &ctx, int argc, const char *const *argv);

    void calculate_args_key(Context &ctx, KeyCalculate &calculate);
    bool calculate_dep_key(KeyCalculate &calculate, const std::string &input_file, const std::string &dep_file);

    std::pair<bool, std::string> do_cache_compilation(Context &ctx, int argc, const char *const *argv);

    static void find_compiler(Context &ctx);
};

}  // namespace ccache

#endif /* ccache_hpp */

