//
//  ccache.cpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#include "ccache.hpp"

#include "DigestCalculate.hpp"
#include "Finalizer.hpp"
#include "Util.hpp"
#include "config.hpp"
#include "context.hpp"
#include "execute.hpp"
#include "fmtmacros.hpp"
#include "temporaryFile.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <ratio>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vector>

namespace fs = boost::filesystem;

namespace ccache {

CCache::CCache(Config &config)
    : m_config(config) {

    std::cout << __FUNCTION__ << std::endl;
}

CCache::~CCache() {

    std::cout << __FUNCTION__ << std::endl;
}

void CCache::initialize(Context &ctx, int argc, const char *const *argv) {

    ArgsInfo orig_args_info;
    Args orig_args = Args::from_argv(argc, argv);
    Args pre_args;

    std::string pre_output_dep;
    std::string pre_output_dia;

    for (int i = 0; i < argc; i++) {
        std::string arg_item = argv[i];

        if (boost::starts_with(arg_item, "-o")) {
            auto output_obj = argv[++i];
            orig_args_info.output_obj = output_obj;

            continue;
        }

        if (boost::starts_with(arg_item, "-target")) {
            std::string target = argv[++i];

            std::vector<std::string> split;
            boost::split(split, target, boost::is_any_of("-"));
            orig_args_info.arch = split[0];
            orig_args_info.platform = split[1];
            orig_args_info.os = split[2];
            if (split.size() >= 4) {
                orig_args_info.device = split[3];
            }

            pre_args.push_back(arg_item);
            pre_args.push_back(target);
            continue;
        }

        if (boost::starts_with(arg_item, "-isysroot")) {
            std::string isysroot = argv[++i];
            orig_args_info.isysroot = isysroot;

            pre_args.push_back(arg_item);
            pre_args.push_back(isysroot);
            continue;
        }

        if (boost::starts_with(arg_item, "-c")) {
            auto input_file = argv[++i];
            orig_args_info.input_file = input_file;

            std::cout << "ccache: "
                      << "input_file "
                      << input_file
                      << std::endl;

            //            pre_args.push_back("-fsyntax-only");
            pre_args.push_back("-E");
            pre_args.push_back(input_file);
            continue;
        }

        // .d
        if (boost::starts_with(arg_item, "-MF")) {

            auto dep = argv[++i];
            auto path_escape_dep = Util::path_escape(dep);
            orig_args_info.output_dep = path_escape_dep;
            //            orig_args.push_back(path_escape_dep);

            std::cout << "ccache: "
                      << "old_output_dep "
                      << orig_args_info.output_dep
                      << std::endl;

            std::vector<std::string> res;
            boost::split(res, orig_args_info.output_dep, boost::is_any_of("/"));
            pre_output_dep = FMT("{}/{}", ctx.temporary_dir(), res.at(res.size() - 1));

            std::cout << "ccache: "
                      << "pre_output_dep "
                      << pre_output_dep
                      << std::endl;

            pre_args.push_back(arg_item);
            pre_args.push_back(pre_output_dep);
            continue;
        }

        // .dia
        if (boost::starts_with(arg_item, "--serialize-diagnostics")) {
            auto dia = argv[++i];
            auto path_escape_dia = Util::path_escape(dia);
            orig_args_info.output_dia = path_escape_dia;
            //            orig_args.push_back(path_escape_dia);

            std::cout << "ccache: "
                      << "old_output_dia "
                      << orig_args_info.output_dia
                      << std::endl;

            std::vector<std::string> res;
            boost::split(res, orig_args_info.output_dia, boost::is_any_of("/"));
            pre_output_dia = FMT("{}/{}", ctx.temporary_dir(), res.at(res.size() - 1));

            std::cout << "ccache: "
                      << "pre_output_dia "
                      << pre_output_dia
                      << std::endl;

            pre_args.push_back(arg_item);
            pre_args.push_back(pre_output_dia);
            continue;
        }

        pre_args.push_back(arg_item);
    }

    ctx.set_orig_args(orig_args);
    ctx.set_orig_args_info(orig_args_info);

    ArgsInfo pre_args_info(orig_args_info);
    pre_args_info.output_dep = pre_output_dep;
    pre_args_info.output_dia = pre_output_dia;

    ctx.set_pre_args(pre_args);
    ctx.set_pre_args_info(pre_args_info);

    return;
}

void CCache::calculate_args_md5(Context &ctx, DigestCalculate &calculate) {

    struct SkipArgsInfo {
        std::string prefix;
        bool next = false;
    };

    std::vector<SkipArgsInfo>
        skip_argv_infos{
            //            {"-fmodules-cache-path"},
            //            {"-fbuild-session-file"},
            //            {"-index-store-path", true},
            //            {"-iquote", true},
            //            {"-I"},
            //            {"-F"},
            {"-MF", true},
            {"--serialize-diagnostics", true},
            {"-c", true},
            {"-o", true},
        };

    std::vector<const char *> argv = ctx.orig_args().to_argv();
    size_t argc = argv.size();
    for (size_t i = 0; i < argc; i++) {
        const char *item = argv[i];
        if (item == nullptr) {
            continue;
        }

        bool skip = false;
        for (SkipArgsInfo &skip_info : skip_argv_infos) {
            if (strncmp(item, skip_info.prefix.c_str(), strlen(skip_info.prefix.c_str())) == 0) {
                skip = true;
                if (skip_info.next) {
                    i++;
                }
                break;
            }
        }

        if (skip) {
            continue;
        }

        std::string str{item};
        for (std::string &prefix : m_config.ignore_path_prefix) {
            if (boost::contains(str, prefix)) {
                skip = true;
                break;
            }
        }

        if (skip) {
            continue;
        }

        for (std::string &prefix : m_config.remove_path_prefix) {
            if (boost::contains(str, prefix)) {
                boost::replace_all(str, prefix, "");
            }
        }

        calculate.Update(str);
    }
}

bool CCache::calculate_dep_md5(DigestCalculate &calculate, const std::string &dep_file) {

    std::cout << "ccache: read .d\n"
              << dep_file
              << std::endl;

    std::ifstream ifs(dep_file, std::ios::in);
    if (!ifs.is_open()) {
        return false;
    }

    std::string line;
    while (getline(ifs, line)) {

        std::string fix_line{line};
        //            std::cout << "line: " << fix_line << "\n";
        auto pos = fix_line.find("/");
        if (pos == std::string::npos) {
            continue;
        }
        if (pos != 0) {
            fix_line.erase(fix_line.begin(), fix_line.begin() + pos);
        }
        if (boost::ends_with(fix_line, " \\")) {
            fix_line.erase(fix_line.end() - 2, fix_line.end());
        }
        while ((pos = fix_line.find("\\")) != std::string::npos) {
            fix_line.erase(fix_line.begin() + pos, fix_line.begin() + pos + 1);
        }

        fs::path path{fix_line};
        if (fs::exists(path)) {

            const std::string filename = path.filename().string();

            calculate.Update(filename);

            if (boost::contains(fix_line, "/DerivedData/")) {
                // 自动生成的文件
                calculate.UpdateFormFile(fix_line);
            } else {
                auto time = fs::last_write_time(path);
                const std::string milliseconds_str = FMT("{}", time);
                //                    std::cout << "ccache: .d "
                //                              << path.string() << " "
                //                              << milliseconds_str
                //                              << std::endl;
                calculate.Update(milliseconds_str);
            }
        }
    }
    ifs.close();

    return true;
}

void CCache::find_compiler(Context &ctx) {

    Args &orig_args = ctx.orig_args();
    Args &pre_args = ctx.pre_args();

    size_t compiler_pos = 0;
    while (compiler_pos < orig_args.size() && boost::ends_with(orig_args[compiler_pos], "ccache")) {
        ++compiler_pos;
    }

    const std::string compiler = orig_args[compiler_pos];
    orig_args.pop_front(compiler_pos);
    orig_args[0] = compiler;
    pre_args.pop_front(compiler_pos);
    pre_args[0] = compiler;
}

struct GetTmpFdResult {
    Fd fd;
    std::string path;
};

static GetTmpFdResult
get_tmp_fd(Context &ctx,
           const std::string_view description,
           const bool capture_output) {
    if (capture_output) {
        TemporaryFile tmp_stdout(
            FMT("{}/tmp.{}", ctx.temporary_dir(), description));
        return {std::move(tmp_stdout.fd), std::move(tmp_stdout.path)};
    } else {
        const auto dev_null_path = Util::get_dev_null_path();
        return {Fd(open(dev_null_path, O_WRONLY)), dev_null_path};
    }
}

struct DoExecuteResult {
    int exit_status;
    std::string stdout_data;
    std::string stderr_data;
};

static DoExecuteResult
do_execute(Context &ctx, Args &args, const bool capture_stdout = true) {

    auto tmp_stdout = get_tmp_fd(ctx, "stdout", capture_stdout);
    auto tmp_stderr = get_tmp_fd(ctx, "stderr", true);

    int status = execute(ctx,
                         args.to_argv().data(),
                         std::move(tmp_stdout.fd),
                         std::move(tmp_stderr.fd));

    return {
        status,
        capture_stdout ? Util::read_file(tmp_stdout.path) : std::string(),
        Util::read_file(tmp_stderr.path),
    };
}

std::pair<bool, std::string> CCache::do_cache_compilation(Context &ctx, int argc, const char *const *argv) {

    std::unique_ptr<DigestCalculate> digestCalculate = std::make_unique<DigestCalculate>(m_config);
    digestCalculate->Init();

    do {
        // 将编译器版本加入md5 计算
        const std::string compiler = ctx.orig_args()[0];
        Args args;
        args.push_back(compiler);
        args.push_back("--version");
        auto result = do_execute(ctx, args);

        if (result.exit_status == EXIT_SUCCESS) {
            std::vector<std::string> res;
            boost::split(res, result.stdout_data, boost::is_any_of("\n"));
            if (!res.empty()) {
                const std::string compiler_version = res[0];
                digestCalculate->Update(compiler_version);
            }
        }
    } while (0);

    calculate_args_md5(ctx, *digestCalculate);

    do {
        // 执行预处理, 同时生成 .d .dia 文件, 然后基于 .d 文件 以及源码文件 计算缓存的 key
        std::cout << "ccache: pre gen .d"
                  << std::endl;
        auto result = do_execute(ctx, ctx.pre_args());
        std::cout << "ccache: gen .d status_code "
                  << result.exit_status
                  << std::endl;
        if (result.exit_status != EXIT_SUCCESS) {
            Util::send_to_fd(result.stderr_data, STDERR_FILENO);
            Util::send_to_fd(result.stdout_data, STDOUT_FILENO);
            return std::make_pair(false, "");
        }
    } while (0);

    if (!calculate_dep_md5(*digestCalculate, ctx.pre_args_info().output_dep)) {
        return std::make_pair(false, std::string());
    };

    do {
        const std::string input_file = ctx.pre_args_info().input_file;
        digestCalculate->UpdateFormFile(input_file);
    } while (0);

    digestCalculate->Final();
    std::string digest = digestCalculate->Digest();

    std::cout << "ccache: cache key "
              << digest
              << std::endl;

    ArgsInfo &orig_args_info = ctx.orig_args_info();
    ArgsInfo &pre_args_info = ctx.pre_args_info();

    fs::copy_options options = fs::copy_options::overwrite_existing | fs::copy_options::recursive;
    const std::string path = FMT("{}/{}", ctx.cache_dir(), digest);
    fs::path cache_file_path{path};

    std::cout << "ccache: cache file "
              << path
              << std::endl;

    boost::system::error_code code;
    if (fs::exists(cache_file_path, code)) {
        std::cout << "ccache: Hit the cache" << std::endl;
        std::cout << "ccache: cp " << cache_file_path.string() << " " << orig_args_info.output_obj << std::endl;
        std::cout << "ccache: cp " << pre_args_info.output_dep << " " << orig_args_info.output_dep << std::endl;
        std::cout << "ccache: cp " << pre_args_info.output_dia << " " << orig_args_info.output_dia << std::endl;
        fs::copy(cache_file_path, orig_args_info.output_obj, options);
        fs::copy(pre_args_info.output_dep, orig_args_info.output_dep, options);
        fs::copy(pre_args_info.output_dia, orig_args_info.output_dia, options);
        return std::make_pair(true, digest);
    } else {
        std::cout << "ccache: cache file exists code "
                  << code
                  << std::endl;
    }
    return std::make_pair(false, digest);
}

int CCache::compilation(int argc, const char *const *argv) {
    if (argc <= 1) {
        return EXIT_SUCCESS;
    }

    Context ctx{m_config.file_storage.path};

    auto cur_duration = std::chrono::system_clock::now().time_since_epoch();
    auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(cur_duration);
    std::string cur_work_dir_name = FMT("{}-{}", Util::str_rand(6), mil.count());
    ctx.append_temporary_dir(cur_work_dir_name);

    ccache::Finalizer clenup_finalizer([&ctx] {
        fs::path temporary_dir_path(ctx.temporary_dir());
        if (fs::exists(temporary_dir_path)) {
            boost::system::error_code code;
            fs::remove_all(temporary_dir_path, code);
            std::cout << "ccache: cleanup "
                      << temporary_dir_path
                      << " code "
                      << code
                      << std::endl;
        }
    });

    initialize(ctx, argc, argv);
    find_compiler(ctx);

    if (boost::ends_with(ctx.orig_args_info().input_file, ".pch") || boost::ends_with(ctx.orig_args_info().output_obj, ".gch")) {

        auto compile_result = do_execute(ctx, ctx.orig_args());
        std::cout << "ccache: compile pch status_code "
                  << compile_result.exit_status
                  << std::endl;

        Util::send_to_fd(compile_result.stderr_data, STDERR_FILENO);
        Util::send_to_fd(compile_result.stdout_data, STDOUT_FILENO);
        return compile_result.exit_status;
    }

    auto result = do_cache_compilation(ctx, argc, argv);
    auto hit_cache = result.first;
    auto cache_key = result.second;
    if (hit_cache) {
        return EXIT_SUCCESS;
    }

    ArgsInfo &orig_args_info = ctx.orig_args_info();
    ArgsInfo &pre_args_info = ctx.pre_args_info();

    std::cout << "ccache: Missed cache"
              << std::endl;

    auto compile_result = do_execute(ctx, ctx.orig_args());
    std::cout << "ccache: compile exit code "
              << compile_result.exit_status
              << std::endl;

    if (compile_result.exit_status != EXIT_SUCCESS) {
        Util::send_to_fd(compile_result.stderr_data, STDERR_FILENO);
        Util::send_to_fd(compile_result.stdout_data, STDOUT_FILENO);
        return compile_result.exit_status;
    }

    std::cout << "ccache: checking the build product "
              << std::endl;
    if (!fs::exists(orig_args_info.output_obj)) {
        std::cout << "ccache: output_obj not exists "
                  << std::endl;
        return EXIT_FAILURE;
    }

    if (!fs::exists(orig_args_info.output_dep)) {
        std::cout << "ccache: output_dep not exists "
                  << std::endl;
        return EXIT_FAILURE;
    }

    if (!fs::exists(orig_args_info.output_dia)) {
        std::cout << "ccache: output_dia not exists "
                  << std::endl;
        return EXIT_FAILURE;
    }

    if (cache_key.empty()) {
        std::cout << "ccache: cache_key is empty "
                  << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << "ccache: Save the cache ..."
              << std::endl;
    fs::copy_options options = fs::copy_options::overwrite_existing | fs::copy_options::recursive;
    fs::path cache_file_path{FMT("{}/{}", ctx.cache_dir(), cache_key)};
    fs::copy(orig_args_info.output_obj, cache_file_path, options);
    std::cout << "ccache: Save the cache " << cache_file_path.string() << std::endl;
    return EXIT_SUCCESS;
}

};  // namespace ccache

