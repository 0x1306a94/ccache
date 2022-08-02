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

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <chrono>
#include <errno.h>
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

CCache::CCache() {
    std::cout << __FUNCTION__ << std::endl;
}

CCache::~CCache() {
    std::cout << __FUNCTION__ << std::endl;
}

void CCache::initialize(Context &ctx, int argc, const char *const *argv) {

    ArgsInfo orig_args_info;
    Args orig_args;
    Args pre_args;

    std::string pre_output_dep;
    std::string pre_output_dia;

    for (int i = 0; i < argc; i++) {
        std::string arg_item = Util::path_escape(argv[i]);
        if (boost::contains(arg_item, "\"")) {
            arg_item = boost::replace_all_copy(arg_item, "\"", "\\\"");
        }
        if (boost::contains(arg_item, "=")) {
            arg_item = boost::replace_all_copy(arg_item, "=", "\\=");
        }

        orig_args.push_back(arg_item);
        if (boost::starts_with(arg_item, "-o")) {
            auto output_obj = argv[++i];
            auto path_escape_output_obj = Util::path_escape(output_obj);
            std::string obj = path_escape_output_obj;
            orig_args_info.output_obj = output_obj;

            orig_args.push_back(path_escape_output_obj);
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

            orig_args.push_back(target);

            pre_args.push_back(arg_item);
            pre_args.push_back(target);
            continue;
        }

        if (boost::starts_with(arg_item, "-isysroot")) {
            std::string isysroot = Util::path_escape(argv[++i]);
            orig_args_info.isysroot = isysroot;

            orig_args.push_back(isysroot);

            pre_args.push_back(arg_item);
            pre_args.push_back(isysroot);
            continue;
        }

        if (boost::starts_with(arg_item, "-c")) {
            auto input_file = argv[++i];
            auto path_escape_input_file = Util::path_escape(input_file);
            orig_args_info.input_file = input_file;
            orig_args.push_back(path_escape_input_file);

            std::cout << "ccache: "
                      << "input_file "
                      << input_file
                      << std::endl;

            //            pre_args.push_back("-fsyntax-only");
            pre_args.push_back("-E");
            pre_args.push_back(orig_args_info.input_file);
            continue;
        }

        // .d
        if (boost::starts_with(arg_item, "-MF")) {

            auto dep = argv[++i];
            auto path_escape_dep = Util::path_escape(dep);
            orig_args_info.output_dep = path_escape_dep;
            orig_args.push_back(path_escape_dep);

            std::cout << "ccache: "
                      << "old_output_dep "
                      << orig_args_info.output_dep
                      << std::endl;

            std::vector<std::string> res;
            boost::split(res, orig_args_info.output_dep, boost::is_any_of("/"));
            pre_output_dep = (boost::format("%1%/%2%") % ctx.temporary_dir() % res.at(res.size() - 1)).str();

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
            orig_args.push_back(path_escape_dia);

            std::cout << "ccache: "
                      << "old_output_dia "
                      << orig_args_info.output_dia
                      << std::endl;

            std::vector<std::string> res;
            boost::split(res, orig_args_info.output_dia, boost::is_any_of("/"));
            pre_output_dia = (boost::format("%1%/%2%") % ctx.temporary_dir() % res.at(res.size() - 1)).str();

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

    //    std::cout << "ccache: orig_args\n"
    //              << orig_args.to_string()
    //              << std::endl;
    ctx.set_orig_args(orig_args);
    ctx.set_orig_args_info(orig_args_info);

    ArgsInfo pre_args_info(orig_args_info);
    pre_args_info.output_dep = pre_output_dep;
    pre_args_info.output_dia = pre_output_dia;

    ctx.set_pre_args(pre_args);
    ctx.set_pre_args_info(pre_args_info);

    return;
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

std::pair<bool, std::string> CCache::do_cache_compilation(Context &ctx, int argc, const char *const *argv) {

    std::unique_ptr<DigestCalculate> digestCalculate(new DigestCalculate);
    digestCalculate->Init();

    struct SkipArgsInfo {
        std::string prefix;
        bool next = false;
    };

    std::vector<SkipArgsInfo>
        skip_argv_infos{
            {"-fmodules-cache-path"},
            {"-fbuild-session-file"},
            {"-index-store-path", true},
            {"-iquote", true},
            {"-I"},
            {"-F"},
            {"-MF", true},
            {"--serialize-diagnostics", true},
            {"-c", true},
            {"-o", true},
        };

    //    std::cout << "ccache: argvs size "
    //              << argc
    //              << "\n";
    for (int i = 1; i < argc; i++) {
        const char *item = argv[i];

        if (item == nullptr) {
            continue;
        }

        //        std::cout << item << std::endl;
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
        digestCalculate->Update(item, strlen(item));
    }

    // 执行预处理, 同时生成 .d .dia 文件, 然后基于 .d 文件 以及源码文件 计算缓存的 key
    std::string pre_full_commands = ctx.pre_args().to_string();
    std::cout << "ccache: pre gen .d \n";
    //              << pre_full_commands
    //              << std::endl;

    pre_full_commands.append(" >/dev/null 2>&1");

    int status_code = system(pre_full_commands.c_str());
    std::cout << "ccache: gen .d status_code "
              << status_code
              << std::endl;

    if (status_code != 0) {
        return std::make_pair(false, "");
    }

    do {
        std::cout << "ccache: read .d\n"
                  << ctx.pre_args_info().output_dep
                  << std::endl;
        std::ifstream ifs(ctx.pre_args_info().output_dep, std::ios::in);
        if (!ifs.is_open()) {
            return std::make_pair(false, "");
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

                digestCalculate->Update(filename);

                if (boost::contains(fix_line, "/DerivedData/")) {
                    // 自动生成的文件
                    digestCalculate->UpdateFormFile(fix_line);
                } else {
                    auto time = fs::last_write_time(path);
                    const std::string milliseconds_str = (boost::format("%1%") % time).str();
                    //                    std::cout << "ccache: .d "
                    //                              << path.string() << " "
                    //                              << milliseconds_str
                    //                              << std::endl;
                    digestCalculate->Update(milliseconds_str);
                }
            }
        }
        ifs.close();

    } while (0);

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
    const std::string path = (boost::format("%1%/%2%") % ctx.cache_dir() % digest).str();
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

    const char *env_cacche_dir = getenv("CCACHE_DIR");
    if (env_cacche_dir == NULL || strlen(env_cacche_dir) == 0) {
        std::cout << "Set the CCACHE_DIR environment variable first" << std::endl;
        return EXIT_FAILURE;
    }
    Context ctx{std::string{env_cacche_dir}};

    auto cur_duration = std::chrono::system_clock::now().time_since_epoch();
    auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(cur_duration);
    std::string cur_work_dir_name = (boost::format("%1%-%2%") % ccache::Util::str_rand(6) % mil.count()).str();
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
        std::string orig_full_commands = ctx.orig_args().to_string();
        int status_code = system(orig_full_commands.c_str());
        std::cout << "ccache: compile pch status_code " << status_code << std::endl;
        return status_code;
    }

    auto result = do_cache_compilation(ctx, argc, argv);
    auto hit_cache = result.first;
    auto cache_key = result.second;
    if (hit_cache) {
        return EXIT_SUCCESS;
    }

    ArgsInfo &orig_args_info = ctx.orig_args_info();
    ArgsInfo &pre_args_info = ctx.pre_args_info();

    //    do {
    //        pid_t ch_pid = fork();
    //        if (ch_pid == -1) {
    //            perror("fork");
    //            return EXIT_FAILURE;
    //        } else if (ch_pid > 0) {
    //
    //        } else {
    //            const char *path = argv[1];
    //            std::cout << "ccache: child compile .... "
    //                      << std::endl;
    //            execv(path, const_cast<char *const *>(argv + 1));
    //            return EXIT_SUCCESS;
    //        }
    //
    //        pid_t child_pid;
    //        while ((child_pid = wait(NULL)) > 0) {
    //        }
    //
    //    } while (0);

    std::cout << "ccache: Missed cache"
              << std::endl;
    //    const char *path = argv[1];
    //    int status_code = execv(path, const_cast<char *const *>(argv + 1));

    std::string orig_full_commands = ctx.orig_args().to_string();

    //    std::cout << "ccache: compile \n"
    //              << orig_full_commands
    //              << std::endl;

    int status_code = system(orig_full_commands.c_str());

    std::cout << "ccache: compile exit code "
              << status_code
              << std::endl;

    std::cout << "ccache: checking the build product "
              << std::endl;
    if (!fs::exists(orig_args_info.output_obj)) {
        std::cout << "ccache: output_obj not exists "
                  << std::endl;
        return status_code;
    }

    if (!fs::exists(orig_args_info.output_dep)) {
        std::cout << "ccache: output_dep not exists "
                  << std::endl;
        return status_code;
    }

    if (!fs::exists(orig_args_info.output_dia)) {
        std::cout << "ccache: output_dia not exists "
                  << std::endl;
        return status_code;
    }

    if (cache_key.empty()) {
        std::cout << "ccache: cache_key is empty "
                  << std::endl;
        return status_code;
    }

    if (status_code != 0) {
        return status_code;
    }

    std::cout << "ccache: Save the cache ..."
              << std::endl;
    fs::copy_options options = fs::copy_options::overwrite_existing | fs::copy_options::recursive;
    fs::path cache_file_path{(boost::format("%1%/%2%") % ctx.cache_dir() % cache_key).str()};
    fs::copy(orig_args_info.output_obj, cache_file_path, options);
    std::cout << "ccache: Save the cache " << cache_file_path.string() << std::endl;
    return EXIT_SUCCESS;
}

};  // namespace ccache

