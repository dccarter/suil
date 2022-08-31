//
// Created by Mpho Mbotho on 2020-11-26.
//

#include <suil/net/smtp/client.hpp>
#include <suil/net/smtp/server.hpp>
#include <suil/net/smtp/context.hpp>

namespace net = suil::net;
namespace smtp = net::smtp;

void sendEmail()
{
    smtp::Client client("127.0.0.1", 5000);
    auto status = client.login(opt(username, "lol@gmail.com"),
                               opt(passwd, "lolpasswd"),
                               opt(forceSsl, false));
    if (!status) {
        serror("logging into email server failed");
    }
    net::Email msg({"loller@gmail.com"}, "Hello Mr Carter");
    msg.body() << R"(Hi Carter

I am just writing to test this email client.

Sincerely,

Suil Devops)";
    auto cleaner = [](const suil::String& path){
        sdebug("cleaning up %s", path());
    };
    msg.attach(net::Email::Attachment{"Makefile", cleaner});
    net::Email::Address from{"lol@gmail.com"};
    client.send(msg, from);
    sdebug("Done!!!!");
}

void composEmail(net::Email& msg)
{

    msg.attach({"Makefile", [](const suil::String& f){
        sdebug("destroying attachment");
    }});
    msg.body() <<
                "<h1>Hello Mr Carter</h1>\n"
                "<p>Lorem Ipsum is simply dummy text of the printing and typesetting\n"
                "industry. Lorem Ipsum has been the industry's standard dummy text ever\n"
                "since the 1500s, when an unknown printer took a galley of type and\n"
                "scrambled it to make a type specimen book. It has survived not only\n"
                "five centuries, but also the leap into electronic typesetting, remaining\""
                "essentially unchanged. It was popularised in the 1960s with the release\n"
                "of Letraset sheets containing Lorem Ipsum passages, and more recently with\n"
                "desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";
    msg.setContentType("text/html");
}

void sendUsingOutbox()
{
    smtp::SmtpOutbox outbox("127.0.0.1", 5000, {"lol@gmail.com"});
    auto status = outbox.login(
            opt(username, "lol@gmail.com"),
            opt(passwd, "lol.passwd"),
            opt(forceSsl, false));
    if (!status) {
        serror("logging in to mail server failed");
        exit(EXIT_FAILURE);
    }

    auto msg1 = outbox.draft("loller1@gmail.com", "Sent From suil::net::SmtpOutbox1");
    composEmail(*msg1);
    auto msg2 = outbox.draft("loller2@gmail.com", "Sent From suil::net::SmtpOutbox2");
    composEmail(*msg2);
    auto msg3 = outbox.draft("loller3@gmail.com", "Sent From suil::net::SmtpOutbox3");
    composEmail(*msg3);
    auto msg4 = outbox.draft("loller4@gmail.com", "Sent From suil::net::SmtpOutbox4");
    composEmail(*msg4);

    // send message using outbox
    outbox.send(msg1);
    outbox.send(msg2);
    outbox.send(msg3);
    outbox.send(msg4);
    msleep(suil::Deadline{15000});
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        auto context = std::make_shared<smtp::ServerContext>();
        net::ServerConfig config;
        config.socketConfig = net::TcpSocketConfig{
                .bindAddr = {.port = 5000}
        };
        smtp::Server smtpSever(config, context);
        smtpSever.start();
    }
    else {
        sendUsingOutbox();
    }
};