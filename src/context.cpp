//
//  context.cpp
//  ccache
//
//  Created by king on 2022/7/31.
//

#include "context.hpp"
#include "fmtmacros.hpp"

#include <boost/algorithm/string.hpp>

namespace ccache {
Context::Context(std::string cache_dir, std::string log_dir, std::string build_id, std::string build_task_id)
    : m_cache_dir(fs::path(cache_dir))
    , m_log_dir(fs::path(log_dir))
    , m_build_id(build_id)
    , m_build_task_id(build_task_id)
    , m_apparent_cwd(getenv("PWD")) {

	m_temporary_dir = fs::temp_directory_path().string();

	if (!fs::exists(m_cache_dir)) {
		fs::create_directories(m_cache_dir);
	}

	if (!m_log_dir.empty()) {
		if (!build_id.empty()) {
			m_log_dir.append(m_build_id);
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
	m_temporary_dir.append(str);
	if (!fs::exists(m_temporary_dir)) {
		fs::create_directories(m_temporary_dir);
	}
}
};  // namespace ccache
