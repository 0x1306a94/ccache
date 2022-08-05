//
//  MiniTrace.hpp
//  ccache
//
//  Created by king on 2022/8/5.
//

#ifndef MiniTrace_hpp
#define MiniTrace_hpp

#include <minitrace/minitrace.h>

#ifdef MTR_ENABLED

#include <string>

namespace ccache {
class Context;
class MiniTrace {
  public:
    MiniTrace(Context &ctx);
    ~MiniTrace();

  private:
    const void *const m_trace_id;
    std::string m_trace_file;
};
};  // namespace ccache
#endif

#endif /* MiniTrace_hpp */

