//
//  context.cpp
//  ccache
//
//  Created by king on 2022/7/31.
//

#include "context.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace ccache {
Context::Context(std::string cache_dir)
    : m_cache_dir(cache_dir)
    , m_apparent_cwd(getenv("PWD")) {

    m_temporary_dir = "/tmp/ccache";

    if (!fs::exists(m_cache_dir)) {
        fs::create_directory(m_cache_dir);
    }

    if (!fs::exists(m_temporary_dir)) {
        fs::create_directory(m_temporary_dir);
    }
}

Context::~Context() {
}

void Context::append_temporary_dir(std::string &str) {
    std::vector<std::string> s;
    boost::split(s, str, boost::is_any_of("/"));
    if (s.empty()) {
        return;
    }
    for (std::string &item : s) {
        m_temporary_dir.append("/");
        m_temporary_dir.append(item);
        if (!fs::exists(m_temporary_dir)) {
            fs::create_directory(m_temporary_dir);
        }
    }
}
};  // namespace ccache

