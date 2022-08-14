//
//  ArgsInfo.hpp
//  ccache
//
//  Created by king on 2022/7/31.
//

#ifndef ArgsInfo_hpp
#define ArgsInfo_hpp

#include "Args.hpp"

#include <string>
#include <vector>

namespace ccache {
struct ArgsInfo {

    //    ArgsInfo() = default;
    //    ArgsInfo(const ArgsInfo &other) = default;
    //    ArgsInfo(ArgsInfo &&other) noexcept;

    std::string input_file;
    std::string output_dep;
    std::string output_dia;
    std::string output_obj;
    std::string arch;
    std::string platform;
    std::string os;
    std::string device;
    std::string isysroot;
    bool compile_object{false};
};
};  // namespace ccache

#endif /* ArgsInfo_hpp */

