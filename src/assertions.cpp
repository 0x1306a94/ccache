//
//  assertions.cpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#include "assertions.hpp"

#include "Util.hpp"
#include "fmtmacros.hpp"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

void handle_failed_assertion(const char *file,
                             size_t line,
                             const char *function,
                             const char *condition) {

    fs::path path{file};

    PRINT(stderr,
          "ccache: {}:{}: {}: failed assertion: {}\n",
          path.filename().string(),
          line,
          function,
          condition);
    abort();
}

