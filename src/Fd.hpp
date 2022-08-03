//
//  Fd.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef Fd_h
#define Fd_h

#include "NonCopyable.hpp"
#include "assertions.hpp"

namespace ccache {

class Fd : NonCopyable {
  public:
    Fd() = default;
    explicit Fd(int fd);
    Fd(Fd &&other_fd) noexcept;
    ~Fd();

    explicit operator bool() const;

    int get() const;
    int operator*() const;

    Fd &operator=(Fd &&other_fd) noexcept;

    // Close wrapped fd before the lifetime of Fd has ended.
    bool close();

    // Release ownership of wrapped fd.
    int release();

  private:
    int m_fd = -1;
};

inline Fd::Fd(int fd)
    : m_fd(fd) {
}

inline Fd::Fd(Fd &&other_fd) noexcept
    : m_fd(other_fd.release()) {
}

inline Fd::~Fd() {
    close();
}

inline Fd::operator bool() const {
    return m_fd != -1;
}

inline int
Fd::get() const {
    return m_fd;
}

inline int
Fd::operator*() const {
    ASSERT(m_fd != -1);
    return m_fd;
}

inline Fd &
Fd::operator=(Fd &&other_fd) noexcept {
    close();
    m_fd = other_fd.release();
    return *this;
}

inline int
Fd::release() {
    int fd = m_fd;
    m_fd = -1;
    return fd;
}

};  // namespace ccache

#endif /* Fd_h */

