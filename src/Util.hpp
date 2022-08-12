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

    static const char *get_dev_null_path();

    static void write_fd(int fd, const void *data, size_t size);
    static void send_to_fd(const std::string &text, int fd);
    static void set_cloexec_flag(int fd);

    static std::string read_file(const std::string &path);
    
    static bool file_exists(const char *file_path);
};
};  // namespace ccache

#endif /* Util_hpp */

