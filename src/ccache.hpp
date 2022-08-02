//
//  ccache.hpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#ifndef ccache_hpp
#define ccache_hpp

#include "context.hpp"

namespace ccache {

class CCache {

  public:
    CCache();

    ~CCache();

    int compilation(int argc, const char *const *argv);

  private:
    static void
    initialize(Context &ctx, int argc, const char *const *argv);

    std::pair<bool, std::string> do_cache_compilation(Context &ctx, int argc, const char *const *argv);

    static void find_compiler(Context &ctx);
};

}  // namespace ccache

#endif /* ccache_hpp */

