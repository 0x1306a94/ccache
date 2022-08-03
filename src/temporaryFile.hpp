//
//  temporaryFile.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef temporaryFile_hpp
#define temporaryFile_hpp

#include "Fd.hpp"

#include <string>
#include <string_view>

namespace ccache {
class TemporaryFile {
  public:
    TemporaryFile(std::string_view path_prefix, std::string_view suffix = {});

    TemporaryFile(TemporaryFile &&other) noexcept = default;

    TemporaryFile &operator=(TemporaryFile &&other) noexcept = default;

    // The resulting open file descriptor in read/write mode. Unset on error.
    Fd fd;

    // The actual filename. Empty on error.
    std::string path;
};
};     // namespace ccache
#endif /* temporaryFile_hpp */

