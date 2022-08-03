//
//  execute.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef execute_hpp
#define execute_hpp

#include <optional>
#include <string>

namespace ccache {

class Context;
class Fd;

int execute(Context &ctx, const char *const *argv, Fd &&fd_out, Fd &&fd_err);

void execute_noreturn(const char *const *argv, const std::string &temp_dir);

};  // namespace ccache

#endif /* execute_hpp */

