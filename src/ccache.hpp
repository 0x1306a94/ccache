//
//  ccache.hpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#ifndef ccache_hpp
#define ccache_hpp

#include <stdio.h>

class CCache {

  public:
    CCache(int argc, const char *const *argv);

    ~CCache();
    
    int compilation();

  private:
    int m_argc;
    const char *const *m_argv;
};

#endif /* ccache_hpp */

