//
//  Util.cpp
//  ccache
//
//  Created by king on 2022/8/1.
//

#include "Util.hpp"

#include "Fd.hpp"
#include "fmtmacros.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <iostream>
#include <random>
#include <string>
#include <sys/stat.h>

namespace fs = boost::filesystem;
namespace ccache {

const char k_dev_null_path[] = "/dev/null";

std::string Util::path_escape(const char *arg) {
    std::string str{arg};
    return path_escape(str);
}

std::string Util::path_escape(const std::string &arg) {

    boost::regex regex{"(/.*?\\s)|(/.*?\\.[a-z]{1,})"};
    boost::cmatch what;
    if (boost::regex_match(arg.c_str(), what, regex)) {
#if CCACHE_MACOSX_PLATFORM
        auto string = boost::replace_all_copy(arg, " ", "\\ ");
        //        std::cout << "ccache: The blank space \n"
        //                  << arg
        //                  << "\n"
        //                  << string
        //                  << std::endl;
        return string;
#endif
    }

    return arg;
}

std::string Util::str_rand(int length) {
    //    static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    //    std::string result;
    //    result.resize(length);
    //
    //    srand(time(NULL));
    //    for (int i = 0; i < length; i++) {
    //        result[i] = charset[rand() % charset.length()];
    //    }
    //    return result;

    using std::default_random_engine;
    using std::random_device;
    using std::string;

    char tmp;       // tmp: 暂存一个随机数
    string buffer;  // buffer: 保存返回值

    // 下面这两行比较重要:
    random_device rd;                    // 产生一个 std::random_device 对象 rd
    default_random_engine random(rd());  // 用 rd 初始化一个随机数发生器 random

    for (int i = 0; i < length; i++) {
        tmp = random() % 36;  // 随机一个小于 36 的整数，0-9、A-Z 共 36 种字符
        if (tmp < 10) {       // 如果随机数小于 10，变换成一个阿拉伯数字的 ASCII
            tmp += '0';
        } else {  // 否则，变换成一个大写字母的 ASCII
            tmp -= 10;
            tmp += 'A';
        }
        buffer += tmp;
    }
    return buffer;
}

const char *Util::get_dev_null_path() {
    return k_dev_null_path;
}

void Util::write_fd(int fd, const void *data, size_t size) {
    int64_t written = 0;
    do {
        const auto count =
            write(fd, static_cast<const uint8_t *>(data) + written, size - written);
        if (count == -1) {
            if (errno != EAGAIN && errno != EINTR) {
                throw FMT("{}", strerror(errno));
            }
        } else {
            written += count;
        }
    } while (static_cast<size_t>(written) < size);
}

void Util::send_to_fd(const std::string &text, int fd) {
    const std::string *text_to_send = &text;
    try {
        write_fd(fd, text_to_send->data(), text_to_send->length());
    } catch (...) {
    }
}

void Util::set_cloexec_flag(int fd) {
    int flags = fcntl(fd, F_GETFD, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
    }
}

std::string Util::read_file(const std::string &path) {
    if (!fs::exists(path)) {
        throw FMT("{} not exists", path);
    }

    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        throw FMT("{} Can't open", path);
    }

    struct stat sb;
    if (fstat(fileno(fp), &sb) == -1) {
        throw FMT("{} fstat error", path);
    }

    Fd fd(fileno(fp));

    size_t size_hint = sb.st_size + 1;

    int64_t ret = 0;
    size_t pos = 0;
    std::string result;
    result.resize(size_hint);

    while (true) {
        if (pos == result.size()) {
            result.resize(2 * result.size());
        }
        const size_t max_read = result.size() - pos;
        ret = read(*fd, &result[pos], max_read);
        if (ret == 0 || (ret == -1 && errno != EINTR)) {
            break;
        }
        if (ret > 0) {
            pos += ret;
            if (static_cast<size_t>(ret) < max_read) {
                break;
            }
        }
    }

    result.resize(pos);
    return result;
}

};  // namespace ccache

