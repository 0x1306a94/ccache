//
//  context.hpp
//  ccache
//
//  Created by king on 2022/7/31.
//

#ifndef Context_hpp
#define Context_hpp

#include "ArgsInfo.hpp"

namespace ccache {
class Args;
class Context {
  public:
    Context(std::string cache_dir);
    ~Context();

    std::string &temporary_dir() { return m_temporary_dir; }
    std::string &apparent_cwd() { return m_apparent_cwd; }

    void append_temporary_dir(std::string &str);

    void set_orig_args(Args &args) {
        m_orig_args = args;
    }

    void set_pre_args(Args &args) {
        m_pre_args = args;
    }

    void set_orig_args_info(ArgsInfo &args_info) {
        m_orig_args_info = args_info;
    }

    void set_pre_args_info(ArgsInfo &args_info) {
        m_pre_args_info = args_info;
    }

    std::string &cache_dir() { return m_cache_dir; }
    ArgsInfo &orig_args_info() { return m_orig_args_info; }
    Args &orig_args() { return m_orig_args; }

    ArgsInfo &pre_args_info() { return m_pre_args_info; }
    Args &pre_args() { return m_pre_args; }

  private:
    std::string m_temporary_dir;
    std::string m_apparent_cwd;

    ArgsInfo m_orig_args_info;
    Args m_orig_args;

    ArgsInfo m_pre_args_info;
    Args m_pre_args;
    std::string m_cache_dir;
};
};  // namespace ccache

#endif /* Context_hpp */

