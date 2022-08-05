//
//  ccache.cpp
//  ccache
//
//  Created by king on 2022/7/30.
//

#include "ccache.hpp"

#include "Finalizer.hpp"
#include "Util.hpp"
#include "config.hpp"
#include "context.hpp"
#include "env_key.h"
#include "execute.hpp"
#include "fmtmacros.hpp"
#include "key_calculate.hpp"
#include "temporaryFile.hpp"
#include "configure.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
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

void CCache::init_log(Context &ctx) {
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
	if (m_config.console_log) {
		logging::add_console_log(std::cout, keywords::format = foramt);
	}
	logging::add_common_attributes();
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

			BOOST_LOG_TRIVIAL(trace) << "input_file "
			                         << input_file;

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

			BOOST_LOG_TRIVIAL(trace) << "old_output_dep "
			                         << orig_args_info.output_dep;

			std::vector<std::string> res;
			boost::split(res, orig_args_info.output_dep, boost::is_any_of("/"));
			pre_output_dep = FMT("{}/{}", ctx.temporary_dir(), res.at(res.size() - 1));

			BOOST_LOG_TRIVIAL(trace) << "pre_output_dep "
			                         << pre_output_dep;

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

			BOOST_LOG_TRIVIAL(trace) << "old_output_dia "
			                         << orig_args_info.output_dia;

			std::vector<std::string> res;
			boost::split(res, orig_args_info.output_dia, boost::is_any_of("/"));
			pre_output_dia = FMT("{}/{}", ctx.temporary_dir(), res.at(res.size() - 1));

			BOOST_LOG_TRIVIAL(trace) << "pre_output_dia "
			                         << pre_output_dia;

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

void CCache::calculate_args_key(Context &ctx, KeyCalculate &calculate) {

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
	        {"-isystem", true},
	        {"-F"},
	        {"-MMD"},
	        {"-MT"},
	        {"dependencies"},
	        {"-MF", true},
	        {"--serialize-diagnostics", true},
	        {"-c", true},
	        {"-o", true},
	        {"-include", true},
	    };

	std::vector<const char *> argv = ctx.orig_args().to_argv();
	size_t argc = argv.size();
	std::stringstream full_argv_stream;
	std::stringstream key_argv_stream;
	full_argv_stream << "full argv \n";
	key_argv_stream << "key argv \n";
	for (size_t i = 0; i < argc; i++) {
		const char *item = argv[i];
		if (item == nullptr) {
			continue;
		}

		full_argv_stream << item << "\n";
		bool skip = false;
		for (SkipArgsInfo &skip_info : skip_argv_infos) {
			if (boost::starts_with(item, skip_info.prefix)) {
				skip = true;
				if (skip_info.next) {
					i++;
					full_argv_stream << argv[i] << "\n";
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
		key_argv_stream << str << "\n";
		calculate.Update(str);
	}

	BOOST_LOG_TRIVIAL(trace) << full_argv_stream.str();
	BOOST_LOG_TRIVIAL(trace) << key_argv_stream.str();
}

bool CCache::calculate_dep_key(KeyCalculate &calculate, const std::string &dep_file) {

	BOOST_LOG_TRIVIAL(trace) << "read .d "
	                         << dep_file;

	std::ifstream ifs(dep_file, std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}

	MTR_SCOPE("calculate_key_dep", "calculate_key_dep");

	std::string line;
	while (getline(ifs, line)) {

		std::string fix_line{line};
		//        std::cout << "line: " << fix_line << "\n";
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
			MTR_BEGIN("calculate_key_dep", fix_line.c_str());
			calculate.Update(filename);

			if (boost::contains(fix_line, "/DerivedData/")) {
				BOOST_LOG_TRIVIAL(trace) << "DerivedData " << fix_line;
				// 自动生成的文件
				calculate.UpdateFormFile(fix_line);
			} else {
				if (m_config.key_by_content) {
					BOOST_LOG_TRIVIAL(trace) << ".d content "
					                         << path.string();
					calculate.UpdateFormFile(path.string());
				} else if (m_config.key_by_time) {
					auto time = fs::last_write_time(path);
					const std::string milliseconds_str = FMT("{}", time);
					BOOST_LOG_TRIVIAL(trace) << ".d time "
					                         << path.string() << " "
					                         << milliseconds_str;
					calculate.Update(milliseconds_str);
				}
			}
			MTR_END("calculate_key_dep", fix_line.c_str());
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

	std::unique_ptr<KeyCalculate> keyCalculate = std::make_unique<ccache::KeyCalculate>(m_config);
	keyCalculate->Init();

	do {
		MTR_SCOPE("do_cache", "calculate_key_compiler_version");
		// 将编译器版本加入key 计算
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
				keyCalculate->Update(compiler_version);
			}
		}
	} while (0);

	do {
		MTR_SCOPE("do_cache", "calculate_key_args");
		calculate_args_key(ctx, *keyCalculate);
	} while (0);

	do {
		MTR_SCOPE("do_cache", "gen_dep");
		// 执行预处理, 同时生成 .d .dia 文件, 然后基于 .d 文件 以及源码文件 计算缓存的 key
		BOOST_LOG_TRIVIAL(trace) << "pre gen .d";
		auto result = do_execute(ctx, ctx.pre_args());
		BOOST_LOG_TRIVIAL(trace) << "gen .d status_code "
		                         << result.exit_status;
		if (result.exit_status != EXIT_SUCCESS) {
			Util::send_to_fd(result.stderr_data, STDERR_FILENO);
			Util::send_to_fd(result.stdout_data, STDOUT_FILENO);
			return std::make_pair(false, "");
		}
	} while (0);

	if (!calculate_dep_key(*keyCalculate, ctx.pre_args_info().output_dep)) {
		return std::make_pair(false, std::string());
	};

	do {
		MTR_SCOPE("do_cache", "calculate_key_args_input_file");
		const std::string input_file = ctx.pre_args_info().input_file;
		BOOST_LOG_TRIVIAL(trace) << "key input_file " << input_file;
		keyCalculate->UpdateFormFile(input_file);
	} while (0);

	keyCalculate->Final();
	std::string digest = keyCalculate->Digest();

	BOOST_LOG_TRIVIAL(trace) << "cache key "
	                         << digest;

	ArgsInfo &orig_args_info = ctx.orig_args_info();
	ArgsInfo &pre_args_info = ctx.pre_args_info();

	fs::copy_options options = fs::copy_options::overwrite_existing | fs::copy_options::recursive;
	const std::string path = FMT("{}/{}", ctx.cache_dir(), digest);
	fs::path cache_file_path{path};

	BOOST_LOG_TRIVIAL(trace) << "cache file "
	                         << path;

	boost::system::error_code code;
	if (fs::exists(cache_file_path, code)) {
		MTR_SCOPE("do_cache", "copy_cache_file");
		BOOST_LOG_TRIVIAL(trace) << "Hit the cache";
		BOOST_LOG_TRIVIAL(trace) << "cp " << cache_file_path.string() << " " << orig_args_info.output_obj;
		BOOST_LOG_TRIVIAL(trace) << "cp " << pre_args_info.output_dep << " " << orig_args_info.output_dep;
		BOOST_LOG_TRIVIAL(trace) << "cp " << pre_args_info.output_dia << " " << orig_args_info.output_dia;
		fs::copy(cache_file_path, orig_args_info.output_obj, options);
		fs::copy(pre_args_info.output_dep, orig_args_info.output_dep, options);
		fs::copy(pre_args_info.output_dia, orig_args_info.output_dia, options);
		return std::make_pair(true, digest);
	} else {
		BOOST_LOG_TRIVIAL(trace) << "cache file exists code "
		                         << code;
	}
	return std::make_pair(false, digest);
}

int CCache::compilation(int argc, const char *const *argv) {
	if (argc <= 1) {
		return EXIT_SUCCESS;
	}

	const char *build_id = getenv(LLBUILD_BUILD_ID_ENV_KEY);
	const char *build_task_id = getenv(LLBUILD_TASK_ID_ENV_KEY);
	Context ctx{
	    m_config.file_storage.path,
	    m_config.log_dir,
	    build_id == NULL ? std::string() : std::string(build_id),
	    build_task_id == NULL ? std::string() : std::string(build_task_id),
	};

	auto cur_duration = std::chrono::system_clock::now().time_since_epoch();
	auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(cur_duration);
	std::string cur_work_dir_name = FMT("{}-{}", Util::str_rand(6), mil.count());
	ctx.append_temporary_dir(cur_work_dir_name);

	ccache::Finalizer clenup_finalizer([&ctx] {
		fs::path temporary_dir_path(ctx.temporary_dir());
		if (fs::exists(temporary_dir_path)) {
			boost::system::error_code code;
			fs::remove_all(temporary_dir_path, code);

			BOOST_LOG_TRIVIAL(trace) << "cleanup "
			                         << temporary_dir_path
			                         << " code "
			                         << code;
		}
	});

	init_log(ctx);
	if (getenv(CCACHE_INTERNAL_TRACE_ENV_KEY) != NULL) {
#if defined(MTR_ENABLED)
		if (ctx.log_dir().empty()) {
			BOOST_LOG_TRIVIAL(error) << "log_dir not configured";
		} else {
			ctx.mini_trace = std::make_unique<ccache::MiniTrace>(ctx);
		}
#else
		BOOST_LOG_TRIVIAL(error) << "MTR_ENABLED not defined";
#endif
	}

	initialize(ctx, argc, argv);

	MTR_BEGIN("main", "find_compiler");
	find_compiler(ctx);
	MTR_END("main", "find_compiler");

	if (boost::ends_with(ctx.orig_args_info().input_file, ".pch") || boost::ends_with(ctx.orig_args_info().output_obj, ".gch")) {
		MTR_BEGIN("compile", "compile_PCH");
		auto compile_result = do_execute(ctx, ctx.orig_args());
		BOOST_LOG_TRIVIAL(trace) << "compile pch status_code "
		                         << compile_result.exit_status;
		MTR_END("compile", "compile_PCH");
		Util::send_to_fd(compile_result.stderr_data, STDERR_FILENO);
		Util::send_to_fd(compile_result.stdout_data, STDOUT_FILENO);

		return compile_result.exit_status;
	}

	MTR_BEGIN("do_cache", "do_cache");
	auto result = do_cache_compilation(ctx, argc, argv);
	MTR_END("do_cache", "do_cache");
	auto hit_cache = result.first;
	auto cache_key = result.second;
	if (hit_cache) {
		return EXIT_SUCCESS;
	}

	ArgsInfo &orig_args_info = ctx.orig_args_info();
	ArgsInfo &pre_args_info = ctx.pre_args_info();

	BOOST_LOG_TRIVIAL(trace) << "Missed cache";

	MTR_BEGIN("compile", "compile_object");
	auto compile_result = do_execute(ctx, ctx.orig_args());
	MTR_END("compile", "compile_object");
	BOOST_LOG_TRIVIAL(trace) << "compile exit code "
	                         << compile_result.exit_status;

	if (compile_result.exit_status != EXIT_SUCCESS) {
		Util::send_to_fd(compile_result.stderr_data, STDERR_FILENO);
		Util::send_to_fd(compile_result.stdout_data, STDOUT_FILENO);
		return compile_result.exit_status;
	}

	BOOST_LOG_TRIVIAL(trace) << "checking the build product ";
	if (!fs::exists(orig_args_info.output_obj)) {
		BOOST_LOG_TRIVIAL(trace) << "output_obj not exists ";
		return EXIT_FAILURE;
	}

	if (!fs::exists(orig_args_info.output_dep)) {
		BOOST_LOG_TRIVIAL(trace) << "output_dep not exists ";
		return EXIT_FAILURE;
	}

	if (!fs::exists(orig_args_info.output_dia)) {
		BOOST_LOG_TRIVIAL(trace) << "output_dia not exists ";
		return EXIT_FAILURE;
	}

	if (cache_key.empty()) {
		BOOST_LOG_TRIVIAL(trace) << "cache_key is empty ";
		return EXIT_SUCCESS;
	}

	BOOST_LOG_TRIVIAL(trace) << "Save the cache ...";
	MTR_BEGIN("save", "save_object");
	fs::copy_options options = fs::copy_options::overwrite_existing | fs::copy_options::recursive;
	fs::path cache_file_path{FMT("{}/{}", ctx.cache_dir(), cache_key)};
	fs::copy(orig_args_info.output_obj, cache_file_path, options);
	MTR_END("save", "save_object");
	BOOST_LOG_TRIVIAL(trace) << "Save the cache "
	                         << cache_file_path.string();
	return EXIT_SUCCESS;
}

};  // namespace ccache
