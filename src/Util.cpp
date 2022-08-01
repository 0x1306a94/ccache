//
//  Util.cpp
//  ccache
//
//  Created by king on 2022/8/1.
//

#include "Util.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <iostream>
#include <random>
#include <string>

namespace ccache {
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

};  // namespace ccache

