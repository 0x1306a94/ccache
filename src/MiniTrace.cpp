//
//  MiniTrace.cpp
//  ccache
//
//  Created by king on 2022/8/5.
//

#ifdef MTR_ENABLED

#include "MiniTrace.hpp"
#include "context.hpp"
#include "fmtmacros.hpp"

#include <chrono>
#include <unistd.h>

namespace ccache {
MiniTrace::MiniTrace(Context &ctx)
    : m_trace_id(reinterpret_cast<void *>(getpid())) {
    m_trace_file = FMT("{}/{}_trace.json", ctx.log_dir(), ctx.build_task_id());

    mtr_init(m_trace_file.c_str());
    auto cur_duration = std::chrono::system_clock::now().time_since_epoch();
    auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(cur_duration);
	m_start_time = FMT("{}", mil.count());
	MTR_INSTANT_C("", "", "time", m_start_time.c_str());
	MTR_META_PROCESS_NAME("ccache");
    MTR_START("program", "ccache", m_trace_id);
}

MiniTrace::~MiniTrace() {
    MTR_FINISH("program", "ccache", m_trace_id);
    mtr_flush();
    mtr_shutdown();
}
};  // namespace ccache

#endif

