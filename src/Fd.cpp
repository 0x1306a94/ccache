//
//  Fd.cpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#include "Fd.hpp"

#include <unistd.h>

namespace ccache {
bool Fd::close() {
    return m_fd != -1 && ::close(release()) == 0;
}

};  // namespace ccache

