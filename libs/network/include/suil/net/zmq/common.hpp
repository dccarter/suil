//
// Created by Mpho Mbotho on 2020-12-11.
//

#ifndef LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_COMMON_HPP
#define LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_COMMON_HPP

#include <suil/base/logging.hpp>
#include <suil/base/exception.hpp>

namespace suil::net::zmq {

    define_log_tag(ZMQ_SOCK);

    DECLARE_EXCEPTION(ZmqInternalError);
}
#endif //LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_COMMON_HPP
