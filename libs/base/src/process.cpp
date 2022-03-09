//
// Created by Mpho Mbotho on 2020-10-16.
//

#include "suil/base/process.hpp"
#include "suil/base/syson.hpp"

#include <suil/async/fdops.hpp>
#include <suil/async/fdwait.hpp>

#include <sys/wait.h>

namespace {

    void updatenv(const suil::UnorderedMap<suil::String>& env)
    {
        if (env.empty())
            return;

        for (auto& ev : env) {
            // set environment variables
            if (!ev.first.empty())
                setenv(ev.first(), ev.second(), true);
        }
    }

    bool openpipes(int in[2], int out[2], int err[2]) {
        if (pipe(in)) {
            // failed to open IO pipe
            serror("failed to pipe(input): %s", errno_s);
            return false;
        }

        if (pipe(out)) {
            // failed to open IO pipe
            serror("failed to pipe(output): %s", errno_s);
            suil::closepipe(in);
            return false;
        }

        if (pipe(err)) {
            // failed to open IO pipe
            serror("failed to pipe(error): %s", errno_s);
            suil::closepipe(in);
            suil::closepipe(out);
            return false;
        }

        return true;
    }

    void closepipes(int in[2], int out[2], int err[2]) {
        suil::closepipe(in);
        suil::closepipe(out);
        suil::closepipe(err);
    }
}
namespace suil {

    static std::map<pid_t, Process*> ActiveProcess{};
    size_t Process::IOBuffer::MAXIMUM_BUFFER_SIZE{0};

    void Process::IOBuffer::push(std::deque<String> &buf, size_t &sz, String &&s)
    {
        sz += s.size();
        while ((sz >= MAXIMUM_BUFFER_SIZE) && !buf.empty()) {
            sz -= buf.back().size();
            buf.pop_back();
        }
        buf.push_front(std::move(s));
    }

    String Process::IOBuffer::pop(std::deque<String> &buf, size_t &sz)
    {
        if (buf.empty()) {
            return "";
        }

        sz -= buf.back().size();
        auto tmp = std::move(buf.back());
        buf.pop_back();
        return std::move(tmp);
    }

    void Process::onSIGCHLD(int sig, siginfo_t *info, void *context)
    {
        pid_t pid = info? info->si_pid : -1;
        uid_t uid = info? info->si_uid : -1;
        strace("Process::on_SIGCHLD pid=%ld, uid=%ld", pid, uid);

        if (pid == -1) {
            // invalid SIGCHLD received
            serror("received an invalid SIGCHLD with null context {pid=%ld,uid=%ld}", pid, uid);
            return;
        }

        auto it = ActiveProcess.find(pid);
        if (it != ActiveProcess.end()) {
            if ((it->second != nullptr) && it->second->waitingExit)
                if (::write(it->second->notifChan[1], &sig, sizeof(sig)) == -1)
                    lerror(it->second, "error writing to process notif channel: %s", errno_s);
        }
    }

    void Process::onSIGTERM(int sig, siginfo_t *info, void *context)
    {
        swarn("received {signal=%d} to terminate all child process", sig);
        for (auto& proc : ActiveProcess) {
            // terminate all child process
            if (proc.second) {
                proc.second->terminate();
            }
        }
        // erase all entries
        ActiveProcess.clear();
    }

    task<Process::UPtr> Process::start(
            const UnorderedMap<String>& env,
            const char * cmd,
            int argc,
            char *argv[])
    {
        {
            static Once sOnceChild, sOnceExit;
            co_await On({SIGNAL_QUIT, SIGNAL_TERM, SIGNAL_INT}, Process::onSIGTERM, sOnceExit);
            co_await On({SIGNAL_CHLD}, Process::onSIGCHLD, sOnceChild);
        }
        int out[2], err[2], in[2];
        if (!openpipes(in, out, err))
            co_return nullptr;

        auto proc = Process::mkunique();
        if (pipe(proc->notifChan) == -1) {
            // forking process failed
            serror("error opening notification pipe: %s", errno_s);
            closepipes(in, out, err);
            co_return nullptr;
        }

        // the read end of the pipe should be non-blocking
        suil::nonblocking(proc->notifChan[0]);

        if ((proc->pid = fork()) == -1) {
            // forking process failed
            serror("forking failed: %s", errno_s);
            closepipes(in, out, err);
            co_return nullptr;
        }

        if (proc->pid == 0) {
            // child process IO is mapped back to process using pipes
            dup2(in[1],  STDIN_FILENO);
            dup2(out[1], STDOUT_FILENO);
            dup2(err[1], STDERR_FILENO);
            close(in[0]); close(out[0]); close(err[0]);
            // update environment variables
            updatenv(env);

            int ret = ::execvp(cmd, argv);

            // spawning process with execvp failed
            serror("launching '%s' failed: %s", errno_s);
            _exit(ret);
        }
        else {
            // close the unused ends of the pipe
            close(in[1]); close(out[1]); close(err[1]);
            proc->stdIn  = in[0];
            suil::nonblocking(proc->stdIn);
            proc->stdOut = out[0];
            suil::nonblocking(proc->stdOut);
            proc->stdErr = err[0];
            suil::nonblocking(proc->stdErr);
            ActiveProcess[proc->pid] = proc.get();
            strace("launched process pid=%ld", proc->pid);
            // start reading asynchronously
            proc->startReadAsync();
        }

        co_return proc;
    }

    bool Process::hasExited()
    {
        if (Ego.pid == -1)
            return true;
        int status{0};
        int ret = waitpid(Ego.pid, &status, WNOHANG);
        if (ret < 0) {
            if (errno != ECHILD)
                serror("process{pid=%ld} waitpid failed: %s", Ego.pid, errno_s);
            Ego.pid = -1;
            return true;
        }
        if ((ret == Ego.pid) && WIFEXITED(status)) {
            // process exited
            itrace("process{pid=%ld} exited status=%d", Ego.pid, WEXITSTATUS(status));
            Ego.pid = -1;
            return true;
        }

        return false;
    }

    void Process::terminate()
    {
        itrace("terminate process{pid=%ld} requested", Ego.pid);
        if (!Ego.hasExited()) {
            pid_t  tmp{Ego.pid};
            Ego.stopReadAsync();
            kill(tmp, SIGKILL);

        } else {
            // attempting to terminate an already exited process
            itrace("attempting to terminate an already exited process");
            Ego.stopReadAsync();
        }
        nonblocking(Ego.notifChan[0], false);
        suil::closepipe(Ego.notifChan);

    }

    AsyncVoid Process::waitForExit(milliseconds timeout)
    {
        if (Ego.hasExited())
            co_return;

        auto dd = after(timeout);

        Ego.waitingExit = true;
        do {
            auto ev = co_await fdwait(Ego.notifChan[0], FDW(FDW_IN | FDW_ERR), dd);
            if (ev & FDW_IN) {
                // notification received
                int sig{0};
                if (::read(Ego.notifChan[1], &sig, sizeof(sig)) == sizeof(sig)) {
                    if (sig != SIGCHLD) {
                        // shouldn't have received this signal
                        iwarn("received signal=%d instead of SIGCHLD", sig);
                        continue;
                    }
                    break;
                }
            }
            itrace("error while waiting for process to exit: %s", errno_s);
            break;

        } while (!Ego.hasExited());

        Ego.waitingExit = false;
    }

    void Process::startReadAsync()
    {
        _asyncScope.spawn(processAsyncRead(Ego, Ego.stdErr, true));
        _asyncScope.spawn(processAsyncRead(Ego, Ego.stdOut, false));
    }

    void Process::stopReadAsync()
    {
        if (Ego.stdOut >= 0) {
            close(Ego.stdOut);
            Ego.stdOut = -1;
        }

        if (Ego.stdIn >= 0) {
            close(Ego.stdIn);
            Ego.stdIn = -1;
        }

        if (Ego.stdErr >= 0) {
            close(Ego.stdErr);
            Ego.stdErr = -1;
        }
    }

    AsyncVoid Process::processAsyncRead(Process& proc, int fd, bool err)
    {
        proc.pendingReads++;
        char buf[1024];
        do {
            auto ev = co_await fdwait(fd, FDW(FDW_IN | FDW_ERR));
            ltrace(&proc, "read event{fd=%d} %d",fd, ev);
            if (ev == FDW_IN) {
                /* data available to read from fd */
                ssize_t nread = read(fd, buf, sizeof(buffer)-1);
                if (nread == -1) {
                    if (errno == EWOULDBLOCK)
                        continue;
                    // reading failed
                    break;
                }
                if (nread == 0)
                    continue;

                // something has been read
                buf[nread] = '\0';
                auto tmp = String{buf, (size_t)nread, false}.dup();
                if (err) {
                    if (proc.onStdError) {
                        co_await proc.onStdError(tmp);
                    }
                    else {
                        proc.buffer.writeError(std::move(tmp));
                    }
                }
                else {
                    if (proc.onStdOutput) {
                        co_await proc.onStdOutput(tmp);
                    }
                    else {
                        proc.buffer.writeOutput(std::move(tmp));
                    }
                }
            } else {
                // received an error
                ltrace(&proc, "error while waiting for {fd=%d} to be readable: %s",fd, errno_s);
                break;
            }
        } while (!proc.hasExited());

        proc.pendingReads--;
    }

    AsyncVoid Process::flush(Process& p, OutputCallback&& rd, bool err)
    {
        if (err) {
            co_await (p.onStdError += std::move(rd));
            while (p.buffer.hasStderr()) {
                auto msg = p.buffer.readError();
                co_await p.onStdError(msg);
            }
        }
        else {
            co_await (p.onStdOutput += std::move(rd));
            while (p.buffer.hasStdout()) {
                auto msg = p.buffer.readOutput();
                co_await p.onStdOutput(msg);
            }
        }
    }
}