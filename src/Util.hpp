//
//  Util.hpp
//  ccache
//
//  Created by king on 2022/8/1.
//

#ifndef Util_hpp
#define Util_hpp

#include <string>

namespace ccache {
struct Util {
    static std::string path_escape(const char *arg);
    static std::string path_escape(const std::string &arg);
    static std::string str_rand(int length = 16);
};
};  // namespace ccache

#endif /* Util_hpp */

