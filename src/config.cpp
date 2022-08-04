//
//  config.cpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#include "config.hpp"

#include "fmtmacros.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <stdlib.h>
namespace ccache {
void Config::replace_environment_variables() {

    boost::regex reg("(\\${.*?})");
    auto replace_ifneeded = [&](const std::string &item) -> std::string {
        std::string replaced{item};

        boost::smatch what;
        auto start = item.begin();
        auto end = item.end();

        while (boost::regex_search(start, end, what, reg)) {
            auto macth_str = what[0].str();
            auto l = macth_str.find("{");
            auto r = macth_str.find("}");
            auto env_name = macth_str.substr(l + 1, r - l - 1);
            const char *value = getenv(env_name.c_str());
            if (value == NULL) {
                throw FMT("{} not exists", env_name);
            }
            replaced = boost::ireplace_all_copy(replaced, macth_str, value);
            start = what[0].second;
            if (start == end) {
                break;
            }
        }

        return replaced;
    };

    std::vector<std::string> _ignore_path_prefix;
    std::vector<std::string> _remove_path_prefix;

    for (const std::string &item : ignore_path_prefix) {
        std::string replaced = replace_ifneeded(item);
        _ignore_path_prefix.push_back(replaced);
    }

    for (const std::string &item : remove_path_prefix) {
        std::string replaced = replace_ifneeded(item);
        _remove_path_prefix.push_back(replaced);
    }

    std::string file_storage_path = replace_ifneeded(file_storage.path);
    std::string log_dir_path = replace_ifneeded(log_dir);
    this->file_storage.path = file_storage_path;
    this->log_dir = log_dir_path;
    this->ignore_path_prefix = _ignore_path_prefix;
    this->remove_path_prefix = _remove_path_prefix;
}
};  // namespace ccache

