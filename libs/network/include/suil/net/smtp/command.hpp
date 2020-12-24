//
// Created by Mpho Mbotho on 2020-11-23.
//

#ifndef SUILNETWORK_COMMAND_HPP
#define SUILNETWORK_COMMAND_HPP

namespace suil::net::smtp {

    enum class Command {
        Unrecognized,
        Hello,
        EHello,
        MailFrom,
        RcptTo,
        Data,
        Noop,
        Help,
        Verify,
        Expn,
        Quit,
        Auth,
        ATrn,
        BData,
        ETrn,
        StartTls,
        Reset
    };


}
#endif //SUILNETWORK_COMMAND_HPP
