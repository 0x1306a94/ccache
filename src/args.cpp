//
//  Args.cpp
//  ccache
//
//  Created by king on 2022/7/31.
//

#include "Args.hpp"

#include <boost/algorithm/string.hpp>

namespace ccache {
Args::Args(Args &&other) noexcept
    : m_args(std::move(other.m_args)) {
}

Args &
Args::operator=(Args &&other) noexcept {
    if (&other != this) {
        m_args = std::move(other.m_args);
    }
    return *this;
}

Args::~Args() {
}

Args Args::from_argv(int argc, const char *const *argv) {
    Args args;
    args.m_args.assign(argv, argv + argc);
    return args;
}

Args Args::from_string(const std::string &command) {
    Args args;

    std::vector<std::string> res;
    boost::split(res, command, boost::is_any_of(" \t\r\n"));
    for (const std::string &word : res) {
        args.push_back(word);
    }
    return args;
}

std::vector<const char *>
Args::to_argv() const {
    std::vector<const char *> result;
    result.reserve(m_args.size() + 1);
    for (const auto &arg : m_args) {
        result.push_back(arg.c_str());
    }
    result.push_back(nullptr);
    return result;
}

std::string
Args::to_string() const {
    std::string result;
    for (const auto &arg : m_args) {
        if (!result.empty()) {
            result += ' ';
        }
        result += arg;
    }
    return result;
}

void Args::erase_last(std::string_view arg) {
    const auto it = std::find(m_args.rbegin(), m_args.rend(), arg);
    if (it != m_args.rend()) {
        m_args.erase(std::next(it).base());
    }
}

void Args::erase_with_prefix(std::string_view prefix) {
    m_args.erase(std::remove_if(m_args.begin(), m_args.end(), [&prefix](const auto &s) {
                     return boost::starts_with(s, prefix);
                 }),
                 m_args.end());
}

void Args::insert(size_t index, const Args &args) {
    if (args.size() == 0) {
        return;
    }
    m_args.insert(m_args.begin() + index, args.m_args.begin(), args.m_args.end());
}

void Args::pop_back(size_t count) {
    m_args.erase(m_args.end() - count, m_args.end());
}

void Args::pop_front(size_t count) {
    m_args.erase(m_args.begin(), m_args.begin() + count);
}

void Args::push_back(const std::string &arg) {
    m_args.push_back(arg);
}

void Args::push_back(const Args &args) {
    m_args.insert(m_args.end(), args.m_args.begin(), args.m_args.end());
}

void Args::push_front(const std::string &arg) {
    m_args.push_front(arg);
}

void Args::replace(size_t index, const Args &args) {
    if (args.size() == 1) {
        // Trivial case; replace with 1 element.
        m_args[index] = args[0];
    } else {
        m_args.erase(m_args.begin() + index);
        insert(index, args);
    }
}

};  // namespace ccache

