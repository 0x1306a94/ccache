//
//  ccache.cpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#include "ccache.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <chrono>
#include <errno.h>
#include <filesystem>
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

namespace fs = std::filesystem;

extern "C" {
#include <openssl/md5.h>
};

namespace ccache {

CCache::CCache() {
    //    std::cout << __FUNCTION__ << std::endl;
}

CCache::~CCache() {
    //    std::cout << __FUNCTION__ << std::endl;
}

void CCache::initialize(Context &ctx, int argc, const char *const *argv) {

    ArgsInfo orig_args_info;
    Args orig_args = Args::from_argv(argc, argv);

    std::vector<std::string> commands{};
    for (int i = 0; i < argc; i++) {
        const char *item = argv[i];
        if (strcmp(item, "-o") == 0) {
            std::string obj = argv[++i];
            orig_args_info.output_obj = obj;
            //            if (boost::ends_with(obj, ".gch")) {
            //                commands.push_back(item);
            //                commands.push_back(obj);
            //            }
            continue;
        }

        if (strcmp(item, "-target") == 0) {
            std::string target = argv[++i];
            commands.push_back(item);
            commands.push_back(target);
            std::vector<std::string> split;
            boost::split(split, target, boost::is_any_of("-"));
            orig_args_info.arch = split[0];
            orig_args_info.platform = split[1];
            orig_args_info.os = split[2];
            orig_args_info.device = split[3];
            continue;
        }

        if (strcmp(item, "-isysroot") == 0) {
            std::string isysroot = argv[++i];
            orig_args_info.isysroot = isysroot;
            commands.push_back(item);
            commands.push_back(isysroot);
            continue;
        }

        if (strcmp(item, "-c") == 0) {
            orig_args_info.input_file = argv[++i];
            commands.push_back("-fsyntax-only");
            commands.push_back(orig_args_info.input_file);
            continue;
        }
        // .d
        if (strcmp(item, "-MF") == 0) {
            orig_args_info.output_dep = argv[++i];
            commands.push_back(item);
            commands.push_back(orig_args_info.output_dep);
            continue;
        }
        // .dia
        if (strcmp(item, "--serialize-diagnostics") == 0) {
            orig_args_info.output_dia = argv[++i];
            commands.push_back(item);
            commands.push_back(orig_args_info.output_dia);
            continue;
        }
        commands.push_back(item);
    }

    ctx.set_orig_args(orig_args);
    ctx.set_orig_args_info(orig_args_info);

    ArgsInfo pre_args_info(orig_args_info);

    do {

        std::vector<std::string> res;
        boost::split(res, pre_args_info.output_dep, boost::is_any_of("/"));
        std::vector<std::string> fix(res);

        if (boost::ends_with(pre_args_info.input_file, ".pch")) {
            fix.erase(fix.begin(), fix.end() - 4);
            fix.erase(fix.end() - 2, fix.end());
            res.erase(res.end() - 4, res.end());
        } else {
            fix.erase(fix.begin(), fix.end() - 6);
            fix.erase(fix.end() - 1);
            res.erase(res.end() - 6, res.end());
        }

        std::string orig_prefix = boost::join(res, "/");
        std::string new_suffix = boost::join(fix, "/");
        ctx.append_temporary_dir(new_suffix);

        std::string old_output_dep{pre_args_info.output_dep};
        std::string old_output_dia{pre_args_info.output_dia};
        //        std::string old_output_obj{pre_args_info.output_obj};

        //        fs::path dep_path{old_output_dep};
        //        fs::path dia_path{old_output_dia};
        std::cout << "ccache: "
                  << "old_output_dep "
                  << old_output_dep
                  << std::endl;

        std::cout << "ccache: "
                  << "old_output_dia "
                  << old_output_dia
                  << std::endl;

        boost::split(res, old_output_dep, boost::is_any_of("/"));
        pre_args_info.output_dep = (boost::format("%1%/%2%") % ctx.temporary_dir() % res.at(res.size() - 1)).str();

        boost::split(res, old_output_dia, boost::is_any_of("/"));
        pre_args_info.output_dia = (boost::format("%1%/%2%") % ctx.temporary_dir() % res.at(res.size() - 1)).str();

        //        if (boost::ends_with(pre_args_info.output_obj, ".gch")) {
        //
        //            std::cout << "ccache: pch "
        //                      << "old_output_obj "
        //                      << old_output_obj
        //                      << std::endl;
        //
        //            boost::split(res, old_output_obj, boost::is_any_of("/"));
        //            pre_args_info.output_obj = (boost::format("%1%/%2%") % ctx.temporary_dir() % res.at(res.size() - 1)).str();
        //        }

        //        boost::format pattern_fmt = boost::format("/(.*?)%1%/");
        //        pattern_fmt % orig_args_info.device;
        //        std::string pattern = pattern_fmt.str();
        //        boost::regex regex(pattern);

        for (int i = 1; i < commands.size(); i++) {
            if (boost::equal(commands[i], old_output_dep)) {
                commands[i] = pre_args_info.output_dep;
                continue;
            }

            if (boost::equal(commands[i], old_output_dia)) {
                commands[i] = pre_args_info.output_dia;
                continue;
            }

            //            if (boost::equal(commands[i], old_output_obj)) {
            //                commands[i] = pre_args_info.output_obj;
            //                continue;
            //            }
        }
        std::string full_commands = boost::join(commands, " ");
        Args pre_args = Args::from_string(full_commands);
        ctx.set_pre_args(pre_args);
        ctx.set_pre_args_info(pre_args_info);
    } while (0);

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

    initialize(ctx, argc, argv);

    find_compiler(ctx);

    if (boost::ends_with(ctx.orig_args_info().input_file, ".pch") || boost::ends_with(ctx.orig_args_info().output_obj, ".gch")) {
        std::string orig_full_commands = ctx.orig_args().to_string();
        int status_code = system(orig_full_commands.c_str());
        std::cout << "ccache: compile pch status_code " << status_code << std::endl;
        return status_code;
    }

    //    std::cout << ctx.orig_args().to_string() << std::endl;
    //    std::cout << ctx.pre_args().to_string() << std::endl;

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);

#if 0
    auto pre_argv = const_cast<char* const*>(ctx.pre_args().to_argv().data());
    const char *pre_path = pre_argv[0];
    printf("path: %s\n", pre_path);
    int status_code = execv(pre_path, pre_argv);
    if (status_code != 0) {
        return status_code;
    }
#else

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
    std::vector<const char *> to_argv = ctx.orig_args().to_argv();
    for (int i = 1; i < to_argv.size(); i++) {
        const char *item = to_argv[i];
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

        //        printf("md5_argv: %s\n", item);
        MD5_Update(&md5_ctx, item, strlen(item));
    }

    // 执行预处理, 同时生成 .d .dia 文件, 然后基于 .d 文件 以及源码文件 计算缓存的 key
    std::string pre_full_commands = ctx.pre_args().to_string();
    std::cout << "ccache: gen .d \n"
              << pre_full_commands
              << std::endl;

    int status_code = system(pre_full_commands.c_str());
    std::cout << "ccache: gen .d status_code "
              << status_code
              << std::endl;
    if (status_code != 0) {
        return status_code;
    }

    do {
        std::ifstream ifs(ctx.pre_args_info().output_dep, std::ios::in);
        if (!ifs.is_open()) {
            return EXIT_FAILURE;
        }

        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string content{ss.str()};

        //        content = R"(
        //dependencies: \
//  /Users/king/Desktop/CustomTabBarSample/CustomTabBarSample/main.m
        //  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator15.2.sdk/usr/include/kcdata.modulemap \
//  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator15.2.sdk/usr/include/uuid.modulemap \
//  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator15.2.sdk/usr/include/netinet6.modulemap \
//  /Users/king/Desktop/CustomTabBarSample/CustomTabBarSample/AppDelegate.h
        //)";
        //        std::cout << content << std::endl;

        //        boost::regex regex{"(/.*?\\.[a-z]{1,})"};
        boost::regex regex{"(/.*?\\s)|(/.*?\\.[a-z]{1,})"};
        std::string::const_iterator start(content.begin()), end(content.end());
        boost::smatch what;

        while (boost::regex_search(start, end, what, regex, boost::match_default)) {
            start = what[0].second;
            auto item = what[1];
            std::string str = item.str();
            if (boost::ends_with(str, " ")) {
                str.erase(str.end() - 1, str.end());
            }
            // 暂时忽略
            if (boost::contains(str, "/DerivedData/") || boost::contains(str, ".gch")) {
                continue;
            }
            fs::path path(str);

            if (fs::exists(path)) {
                auto time = fs::directory_entry(path).last_write_time();
                auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch());
                const std::string milliseconds_str = (boost::format("%1%") % milliseconds.count()).str();

                //                std::cout << "ccache: .d " << path.string() << " " << milliseconds_str << std::endl;
                MD5_Update(&md5_ctx, path.filename().c_str(), strlen(path.filename().c_str()));
                MD5_Update(&md5_ctx, milliseconds_str.c_str(), strlen(milliseconds_str.c_str()));
            }
        }

        ifs.close();

    } while (0);

    do {
        std::ifstream ifs(ctx.pre_args_info().input_file, std::ios::in);
        if (!ifs.is_open()) {
            return EXIT_FAILURE;
        }

        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string content{ss.str()};
        MD5_Update(&md5_ctx, content.c_str(), strlen(content.c_str()));
        ifs.close();
    } while (0);

    unsigned char md5_buffer[MD5_DIGEST_LENGTH];
    MD5_Final(md5_buffer, &md5_ctx);

    char md5_key[MD5_DIGEST_LENGTH * 2 + 1] = {'\0'};
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(md5_key + i * 2, "%02x", md5_buffer[i]);
    }
    printf("cache key: %s\n", md5_key);
#endif

    ArgsInfo &orig_args_info = ctx.orig_args_info();
    ArgsInfo &pre_args_info = ctx.pre_args_info();

    fs::copy_options options = fs::copy_options::overwrite_existing | fs::copy_options::recursive;
    fs::path cache_file_path{(boost::format("%1%/%2%") % ctx.cache_dir() % md5_key).str()};
    if (fs::exists(cache_file_path)) {
        std::cout << "ccache: Hit the cache" << std::endl;
        std::cout << "ccache: cp " << cache_file_path.string() << " " << orig_args_info.output_obj << std::endl;
        std::cout << "ccache: cp " << pre_args_info.output_dep << " " << orig_args_info.output_dep << std::endl;
        std::cout << "ccache: cp " << pre_args_info.output_dia << " " << orig_args_info.output_dia << std::endl;
        fs::copy(cache_file_path, orig_args_info.output_obj, options);
        fs::copy(pre_args_info.output_dep, orig_args_info.output_dep, options);
        fs::copy(pre_args_info.output_dia, orig_args_info.output_dia, options);
        return EXIT_SUCCESS;
    }

    std::string orig_full_commands = ctx.orig_args().to_string();
    status_code = system(orig_full_commands.c_str());
    std::cout << "ccache: compile status_code " << status_code << std::endl;
    if (fs::exists(orig_args_info.output_obj)) {
        fs::copy(orig_args_info.output_obj, cache_file_path);
        std::cout << "ccache: Save the cache " << cache_file_path.string() << std::endl;
    }
    return status_code;
}

};  // namespace ccache

