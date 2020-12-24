//
// Created by Mpho Mbotho on 2020-12-11.
//

#include "suil/net/zmq/patterns.hpp"

namespace suil::net::zmq {

    bool SubscribeSocket::doSubscribe(const String& topic)
    {
        if (!sock()) {
            iwarn("Subscriber::doSubscribe(...) socket is invalid");
            return false;
        }
        return Option::setSubscribe(sock(), topic);
    }

    void SubscribeSocket::doUnsubscribe(const String& topic)
    {
        if (!sock()) {
            iwarn("Subscriber::doSubscribe(...) socket is invalid");
            return;
        }

        Option::setUnsubscribe(sock(), topic);
    }

}