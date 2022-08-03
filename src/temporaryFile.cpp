//
//  temporaryFile.cpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#include "Util.hpp"
#include "fmtmacros.hpp"
#include "temporaryFile.hpp"

//#include <boo>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace ccache {
TemporaryFile::TemporaryFile(std::string_view path_prefix,
                             std::string_view suffix)
    : path(FMT("{}.XXXXXX{}", path_prefix, suffix)) {
    //  Util::ensure_dir_exists(Util::dir_name(path));
    fs::path p{path};
    fs::path parent_path = p.parent_path();
    if (!fs::exists(parent_path)) {
        if (!fs::create_directory(parent_path)) {
            throw FMT(
                "Failed to create temporary file dir for {}: {}", path, strerror(errno));
        }
    }

    fd = Fd(mkstemps(&path[0], suffix.length()));

    if (!fd) {
        throw FMT(
            "Failed to create temporary file for {}: {}", path, strerror(errno));
    }

    Util::set_cloexec_flag(*fd);
    fchmod(*fd, 0666);
}
};  // namespace ccache

