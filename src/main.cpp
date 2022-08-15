
#include <chrono>
#include <execinfo.h>
#include <iostream>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/core.hpp>
#include <boost/log/detail/format.hpp>
#include <boost/log/detail/thread_id.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <yaml-cpp/yaml.h>

#include "ProjectConfig.hpp"
#include "Util.hpp"
#include "ccache.hpp"
#include "cli.hpp"
#include "context.hpp"
#include "env_key.h"
#include "fmtmacros.hpp"
#include "key_calculate.hpp"

void handler(int sig) {

#define BACK_TRACE_DEPTH 512
    void *array[BACK_TRACE_DEPTH];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, BACK_TRACE_DEPTH);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    uintptr_t vmaddr_slide = 0;
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char *image_name = (char *)_dyld_get_image_name(i);
        const struct mach_header *header = _dyld_get_image_header(i);
        if (header->filetype == MH_EXECUTE) {
            vmaddr_slide = _dyld_get_image_vmaddr_slide(i);
            fprintf(stderr, "Image name %s at address 0x%llx and ALSR silde 0x%lx \n", image_name, (mach_vm_address_t)header, vmaddr_slide);
        }
    }

    exit(sig);
}

void init_log(ccache::Context &ctx, bool console_log) {
    namespace logging = boost::log;
    namespace src = boost::log::sources;
    namespace keywords = boost::log::keywords;
    namespace sinks = boost::log::sinks;
    namespace expr = boost::log::expressions;

    auto log_file = FMT("{}/{}.log", ctx.log_dir(), ctx.build_task_id());

    auto foramt = (expr::stream
                   << "ccache: "
                   << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                   //                            << " " << expr::attr<boost::log::aux::thread::id>("ThreadID")
                   << " " << logging::trivial::severity
                   << "] "
                   << "" << expr::smessage);

    auto sink = logging::add_file_log(
        keywords::open_mode = std::ios::trunc,
        keywords::file_name = log_file,
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::format = foramt);

    sink->locked_backend()->auto_flush(true);
    sink->imbue(std::locale("zh_CN.UTF-8"));
    if (console_log) {
        logging::add_console_log(std::cout, keywords::format = foramt);
    }
    logging::add_common_attributes();
}

int main(int argc, char *const *argv) {

    signal(SIGILL, handler);
    signal(SIGABRT, handler);
    signal(SIGBUS, handler);  // install our handler
    signal(SIGSEGV, handler);
    signal(SIGSYS, handler);

    //    std::unique_ptr<ccache::KeyCalculate> KeyCalculate(new ccache::KeyCalculate);
    //    KeyCalculate->Init();
    //
    //    KeyCalculate->Update("ccache");
    //    KeyCalculate->Final();
    //
    //    std::cout << "md5: " << KeyCalculate->Digest() << "\n";

    if (argc > 1 && argv[1][0] == '-') {
        ccace::do_cli(argc, argv);
        return EXIT_SUCCESS;
    }

    const char *config_file_path = getenv(CCACHE_PROJECT_ENV_CONFIG_KEY);
    if (config_file_path == NULL) {
        std::cout << "Set the "
                  << CCACHE_PROJECT_ENV_CONFIG_KEY
                  << " environment variable first "
                  << std::endl;
        return EXIT_FAILURE;
    }

    ccache::ProjectConfig config = YAML::LoadFile(config_file_path).as<ccache::ProjectConfig>();
    config.replace_environment_variables();
    //    std::stringstream ss;
    //    ss << YAML::Node(config);
    //    std::cout << ss.str() << std::endl;

    if (argc <= 1) {
        std::cout << "Missing parameter"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const char *build_id = getenv(LLBUILD_BUILD_ID_ENV_KEY);
    const char *build_task_id = getenv(LLBUILD_TASK_ID_ENV_KEY);
    ccache::Context ctx{
        config.file_storage.host,
        config.log_dir,
        build_id == NULL ? std::string() : std::string(build_id),
        build_task_id == NULL ? std::string() : std::string(build_task_id),
    };

    auto cur_duration = std::chrono::system_clock::now().time_since_epoch();
    auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(cur_duration);
    std::string cur_work_dir_name = FMT("{}-{}", ccache::Util::str_rand(6), mil.count());
    ctx.append_temporary_dir(cur_work_dir_name);

    init_log(ctx, config.console_log);

    std::unique_ptr<ccache::CCache> cache = std::make_unique<ccache::CCache>(config);
    int result = cache->compilation(ctx, argc, argv);
    return result;
}

