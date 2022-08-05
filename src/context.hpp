//
//  context.hpp
//  ccache
//
//  Created by king on 2022/7/31.
//

#ifndef Context_hpp
#define Context_hpp

#include "ArgsInfo.hpp"
#include "MiniTrace.hpp"
#include "configure.h"

#include <boost/filesystem.hpp>
namespace ccache {

namespace fs = boost::filesystem;

class Args;

class Context {
  public:
    pid_t compiler_pid{0};

    Context(std::string cache_dir, std::string log_dir, std::string build_id, std::string build_task_id);
    ~Context();

    std::string temporary_dir() { return m_temporary_dir.string(); }
    std::string &apparent_cwd() { return m_apparent_cwd; }

    void append_temporary_dir(std::string &str);

    void set_orig_args(Args args) {
        m_orig_args = args;
    }

    void set_pre_args(Args args) {
        m_pre_args = args;
    }

    void set_orig_args_info(ArgsInfo args_info) {
        m_orig_args_info = args_info;
    }

    void set_pre_args_info(ArgsInfo args_info) {
        m_pre_args_info = args_info;
    }

    std::string cache_dir() { return m_cache_dir.string(); }
    std::string log_dir() { return m_log_dir.string(); }
    std::string &build_id() { return m_build_id; }
    std::string &build_task_id() { return m_build_task_id; }
    ArgsInfo &orig_args_info() { return m_orig_args_info; }
    Args &orig_args() { return m_orig_args; }

    ArgsInfo &pre_args_info() { return m_pre_args_info; }
    Args &pre_args() { return m_pre_args; }

#if defined(MTR_ENABLED)
    std::unique_ptr<MiniTrace> mini_trace;
#endif
  private:
    fs::path m_temporary_dir;
    std::string m_apparent_cwd;

    ArgsInfo m_orig_args_info;
    Args m_orig_args;

    ArgsInfo m_pre_args_info;
    Args m_pre_args;
    fs::path m_cache_dir;
    fs::path m_log_dir;
    std::string m_build_id;
    std::string m_build_task_id;
};
};  // namespace ccache

#endif /* Context_hpp */

