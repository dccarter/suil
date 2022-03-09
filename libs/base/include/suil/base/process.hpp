//
// Created by Mpho Mbotho on 2020-10-16.
//

#ifndef SUIL_BASE_PROCESS_HPP
#define SUIL_BASE_PROCESS_HPP

#include "suil/base/buffer.hpp"
#include "suil/base/sio.hpp"
#include "suil/base/logging.hpp"
#include "suil/base/signal.hpp"
#include "suil/base/string.hpp"

#include <suil/utils/utils.hpp>
#include <suil/async/scope.hpp>

#include <deque>
#include <csignal>

namespace suil {

    define_log_tag(PROCESS);

    using Reader = std::function<void(String&&)>;

    class Process final: LOGGER(PROCESS) {
    public:
        sptr(Process);
        using OutputCallback = std::function<bool(String&)>;

        template <typename... Args>
        static UPtr launch(const char *cmd, Args... args) {
            // execute with empty environment
            return Process::launch(UnorderedMap<String>{}, cmd, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static UPtr launch(const  UnorderedMap<String>& env, const char *cmd, Args... args) {
            // reserve buffer for process arguments
            size_t argc  = sizeof...(args);
            char *argv[argc+2];
            // pack parameters into a buffer
            argv[0] = (char *) cmd;
            int index{1};
            Process::pack(index, argv, std::forward<Args>(args)...);
            return Process::start(env, cmd, argc, argv);
        }

        template <typename ...Args>
        static UPtr bash(const UnorderedMap<String>& env, const char *cmd, Args... args) {
            Buffer tmp{256};
            Process::formatArgs(tmp, cmd, std::forward<Args>(args)..., "; sleep 0.1");
            String cmdStr(tmp);
            return Process::launch(env, "bash", "-c", cmdStr());
        }

        template <typename ...Args>
        static UPtr bash(const UnorderedMap<String>&& env, const char *cmd, Args... args) {
            Buffer tmp{256};
            Process::formatArgs(tmp, cmd, std::forward<Args>(args)..., "; sleep 0.1");
            String cmdStr(tmp);
            return Process::launch(env, "bash", "-c", cmdStr());
        }

        template <typename ...Args>
        inline static UPtr bash(const char *cmd, Args... args) {
            Buffer tmp{256};
            Process::formatArgs(tmp, cmd, std::forward<Args>(args)...);
            String cmdStr(tmp);
            return Process::launch("bash", "-c", cmdStr());
        }

        bool hasExited();

        void terminate();

        AsyncVoid waitForExit(milliseconds timeout = DELAY_INF);

        inline String getStdOutput() {
            return Ego.buffer.readOutput();
        }

        inline String getStdError() {
            return Ego.buffer.readError();
        }

        template <typename... Callbacks>
        inline void readAsync(Callbacks&&... callbacks) {
            // configure the callbacks
            struct OutputCallbacks{
                OutputCallback onStdOutput;
                OutputCallback onStdError;
            } cbs{nullptr, nullptr};

            suil::applyConfig(cbs, std::forward<Callbacks>(callbacks)...);
            if (cbs.onStdError) {
                go(flushBuffers(Ego, std::move(cbs.onStdError), true));
            }
            if (cbs.onStdOutput) {
                go(flushBuffers(Ego, std::move(cbs.onStdOutput), false));
            }
        }

        Signal<bool(String&)> onStdOutput;
        Signal<bool(String&)> onStdError;

        ~Process() {
            terminate();
        }


    private:
        class IOBuffer {
        private:
            friend struct Process;
            IOBuffer() = default;

            inline void writeOutput(String &&s) {
                push(stdOutput, stdoutBytes, std::move(s));
            }

            inline void writeError(String &&s) {
                push(stdError, stderrBytes, std::move(s));
            }

            inline String readError() {
                return pop(stdError, stderrBytes);
            }

            inline String readOutput() {
                return pop(stdOutput, stdoutBytes);
            }

            inline bool hasStderr() const {
                return !stdError.empty();
            }

            inline bool hasStdout() const {
                return !stdOutput.empty();
            }

            void push(std::deque<String> &buf, size_t &sz, String &&s);

            String pop(std::deque<String> &buf, size_t &sz);

        private:
            static size_t MAXIMUM_BUFFER_SIZE;
            size_t stderrBytes;
            size_t stdoutBytes;
            std::deque<String> stdError;
            std::deque<String> stdOutput;
        };

    private:
        template <typename... Args>
        static void formatArgs(Buffer& out, Args... args) {
            (out << ... << args);
        }

        static void pack(int& index, char* argv[]) { argv[index] = (char *) NULL; }

        template <typename Arg>
        static void pack(int& index, char* argv[], Arg arg) {
            argv[index++] = (char *) arg;
            argv[index++] = (char *) NULL;
        }

        template <typename Arg, typename... Args>
        static void pack(int& index, char* argv[], const Arg arg, Args... args) {
            argv[index++] = (char *)arg;
            pack(index, argv, std::forward<Args>(args)...);
        }

        void stopReadAsync();

        void startReadAsync();

        static task<UPtr> start(const UnorderedMap<String>& env, const char* cmd, int argc, char* argv[]);

        static void onSIGCHLD(int sig, siginfo_t *info, void *context);
        static void onSIGTERM(int sig, siginfo_t *info, void *context);

        static AsyncVoid processAsyncRead(Process& proc, int fd, bool err);

        static AsyncVoid flush(Process& p, OutputCallback&& rd, bool err);

    private:
        pid_t           pid{0};
        int             stdOut{0};
        int             stdIn{0};
        int             stdErr{0};
        int             notifChan[2];
        bool            waitingExit{false};
        int             pendingReads{0};
        IOBuffer        buffer;
        OutputCallback  stdoutCallback{nullptr};
        OutputCallback  stderrCallback{nullptr};
        AsyncScope      _asyncScope{};
    };
}

#endif //SUIL_BASE_PROCESS_HPP
