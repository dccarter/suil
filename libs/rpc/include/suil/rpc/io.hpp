//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_IO_HPP
#define SUIL_IO_HPP

#include <suil/rpc/common.scc.hpp>

#include <suil/net/socket.hpp>

#include <cinttypes>

namespace suil::rpc {

    define_log_tag(RPC_IO);

    DECLARE_EXCEPTION(RpcInternalError);
    DECLARE_EXCEPTION(RpcTransportError);
    DECLARE_EXCEPTION(RpcApiError);

    template <typename Config>
        requires (std::is_same_v<Config, RpcClientConfig> or
                  std::is_same_v<Config, RpcServerConfig>)
    class RpcIO: public LOGGER(RPC_IO) {
    public:
        virtual bool receive(net::Socket& sock, Buffer& rxb)
        {
            int64_t initialWait{0};
            if constexpr (std::is_same_v<Config, RpcClientConfig>) {
                initialWait = getConfig().receiveTimeout;
            }
            else {
                initialWait = getConfig().keepAlive;
            }

            if (protoUseSize) {
                return sizedReceive(sock, rxb, initialWait);
            }
            else {
                return bestEffortReceive(sock, rxb, initialWait);
            }
        }

        virtual bool transmit(net::Socket& sock, const suil::Data& resp)
        {
            if (Ego.protoUseSize) {
                // Send the size first
                auto size = htole64(resp.size());
                if (sock.send(&size, sizeof(size), getConfig().sendTimeout) != sizeof(size)) {
                    // failed to send
                    iwarn("RPC IO - sending response size failed: %s", errno_s);
                    return false;
                }
            }

            // send the data
            if (sock.send(resp.data(), resp.size(), getConfig().sendTimeout) != resp.size()) {
                // sending failed
                iwarn("RPC IO - sending response of size " PRId64 " failed: %s", resp.size(), errno_s);
                return false;
            }

            return sock.flush(getConfig().sendTimeout);
        }

    protected:
        virtual const Config& getConfig() const = 0;
        bool protoUseSize{false};

    private:
        bool sizedReceive(net::Socket& sock, Buffer& rxb, int64_t initialWait)
        {
            size_t size{0};
            size_t _{sizeof(size)};
            if (!sock.receive(&size, _, initialWait)) {
                // failed to receive the size
                itrace("RPC IO - failed to receive protoUseSize: %s", errno_s);
                return false;
            }

            auto nread = le64toh(size);
            itrace("RPC IO - received protoUseSize {size =" PRId64 "}", nread);
            size = nread;
            rxb.reserve(size);
            if (sock.receive(&rxb[0], nread, getConfig().receiveTimeout)) {
                ierror("RPC IO - failed to receive " PRId64 " bytes from client: %s", size, errno_s);
                return false;
            }
            rxb.seek(nread);
            return true;
        }

        bool bestEffortReceive(net::Socket& sock, Buffer& rxb, int64_t initialWait)
        {
            size_t nread{0}, tread{0};
            rxb.reserve(1024);
            nread = 1;
            // wait for at least 1 byte to be received
            if (!sock.receive(&rxb[0], nread, initialWait)) {
                // waiting for data to be available failed
                itrace("RPC IO - failed to receive data: %s", errno_s);
                return false;
            }
            tread += 1;
            rxb.seek(1);
            auto timeout = getConfig().receiveTimeout;
            bool isDone{false};
            do {
                nread = rxb.capacity();
                if (!sock.read(&rxb[tread], nread, timeout) or ((tread == 0) and (nread == 0))) {
                    // receiving failed
                    itrace("RPC IO - reading request failed: %s:%d", errno_s, nread);
                    if (!(errno == EAGAIN or errno == EWOULDBLOCK)) {
                        // EAGAIN and EWOULDBLOCK shouldn't be treated as error
                        return (errno == ETIMEDOUT) && not(rxb.empty());
                    }
                }

                if (nread) {
                    tread += nread;
                    rxb.seek(nread);
                }

                if (rxb.capacity() == 0) {
                    // conservative, the idea is that we assume we ran out of buffers
                    // while reading and try again with a smaller timeout
                    rxb.reserve(1024);
                    timeout = 100_ms;
                }
                else {
                    // if we couldn't read fill up the buffer break
                    break;
                }
            } while (true);

            return true;
        }
    };
}
#endif //SUIL_IO_HPP
