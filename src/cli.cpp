//
//  cli.cpp
//  ccache
//
//  Created by king on 2022/8/11.
//

#include "cli.hpp"
#include "configure.h"
#include "fmtmacros.hpp"

#include <argparse/argparse.hpp>

namespace ccace {

/* clang-format off */
constexpr const char VERSION_TEXT[] =
  R"({0} version {1}
git: https://github.com/0x1306a94/ccache {2} {3}
build: {4}
)";

/* clang-format on */

void do_cli(int argc, const char *const *argv) {

    using namespace argparse;

    ArgumentParser program{"ccache", CCACHE_VERSION_FULL, default_arguments::help};
    program.add_argument("-v", "--version")
        .default_value(false)
        .action([=](const std::string &s) {
            std::cout << FMT(VERSION_TEXT, "ccache", CCACHE_VERSION_FULL, CCACHE_GIT_BRANCH, CCACHE_GIT_COMMIT, CCACHE_BUILD_DATE);
        })
        .help("Displaying Version Information")
        .implicit_value(true)
        .nargs(0);

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        exit(EXIT_FAILURE);
    }
};
};  // namespace ccace

