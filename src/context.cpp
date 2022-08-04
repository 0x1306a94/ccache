//
//  context.cpp
//  ccache
//
//  Created by king on 2022/7/31.
//

#include "context.hpp"
#include "fmtmacros.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace ccache {
Context::Context(std::string cache_dir, std::string log_dir, std::string build_id, std::string build_task_id)
    : m_cache_dir(cache_dir)
    , m_log_dir(log_dir)
    , m_build_id(build_id)
    , m_build_task_id(build_task_id)
    , m_apparent_cwd(getenv("PWD")) {

    m_temporary_dir = "/tmp/ccache";

    if (!fs::exists(m_cache_dir)) {
        fs::create_directories(m_cache_dir);
    }

    if (!m_log_dir.empty()) {
        if (!build_id.empty()) {
            m_log_dir = FMT("{}/{}", m_log_dir, m_build_id);
        }

        if (!fs::exists(m_log_dir)) {
            fs::create_directories(m_log_dir);
        }
    }

    if (!fs::exists(m_temporary_dir)) {
        fs::create_directories(m_temporary_dir);
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

