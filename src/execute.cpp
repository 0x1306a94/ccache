//
//  execute.cpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#include "execute.hpp"

#include "Fd.hpp"
#include "context.hpp"
#include "fmtmacros.hpp"

#include <sys/wait.h>
#include <unistd.h>

namespace ccache {

int execute(Context &ctx, const char *const *argv, Fd &&fd_out, Fd &&fd_err) {
	ctx.compiler_pid = fork();

	if (ctx.compiler_pid == -1) {
		throw FMT("Failed to fork: {}", strerror(errno));
	}

	if (ctx.compiler_pid == 0) {
		// Child.
		dup2(*fd_out, STDOUT_FILENO);
		fd_out.close();
		dup2(*fd_err, STDERR_FILENO);
		fd_err.close();
		exit(execv(argv[0], const_cast<char *const *>(argv)));
	}

	fd_out.close();
	fd_err.close();

	int status;
	int result;

	while ((result = waitpid(ctx.compiler_pid, &status, 0)) != ctx.compiler_pid) {
		if (result == -1 && errno == EINTR) {
			continue;
		}
		throw FMT("waitpid failed: {}", strerror(errno));
	}

	ctx.compiler_pid = 0;

	if (WEXITSTATUS(status) == 0 && WIFSIGNALED(status)) {
		return -1;
	}

	return WEXITSTATUS(status);
}

void execute_noreturn(const char *const *argv, const std::string & /*temp_dir*/) {
	execv(argv[0], const_cast<char *const *>(argv));
}

};  // namespace ccache