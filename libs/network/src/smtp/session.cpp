//
// Created by Mpho Mbotho on 2020-11-26.
//

#include "suil/net/smtp/session.hpp"

namespace suil::net::smtp {

    Session::Session()
        : id{suil::randbytes(4)},
          mDataId{typeid(SessionData)}
    {}

    Session::State Session::getState() {
        return state;
    }

    const String& Session::getFrom() const
    {
        return from;
    }

    const std::vector<String>& Session::getRecipients() const
    {
        return recipients;
    }

    const String & Session::getId() const
    {
        return id;
    }

    void Session::reset(State state)
    {
        from = nullptr;
        recipients.clear();
        Ego.state = state;
    }

    void Session::setAuth(AuthMechanism mechanism, const String& arg0)
    {
        auth.mechanism = mechanism;
        auth.arg0 = arg0.dup();
    }

    void Session::nextState(State state)
    {
        Ego.state = state;
    }

    AuthMechanism parseAuthMechanism(const String& str)
    {
#define code(a,b,c,d) ( (int(a)<<24) | (int(b)<<16) | (int(c)<<8) | int(d) )
        auto m = code(toupper(str[0]), toupper(str[1]), toupper(str[2]), toupper(str[3]));
        switch (m) {
            case code('L', 'O', 'G', 'I'):
                return str.compare("LOGIN", true) == 0? AuthMechanism::Login : AuthMechanism::Unknown;
            case code('P', 'L', 'A', 'I'):
                return str.compare("PLAIN", true) == 0? AuthMechanism::Plain : AuthMechanism::Unknown;
            default:
                return AuthMechanism::Unknown;
        }
    }

    String toString(AuthMechanism mechanism)
    {
        switch (mechanism) {
            case AuthMechanism::Login: return "LOGIN";
            case AuthMechanism::Plain: return "PLAIN";
            case AuthMechanism::PlainToken: return "PLAIN-TOKEN";
            case AuthMechanism::CramMD5: return "CRAM-MD5";
            case AuthMechanism::MD5: return "MD5";
            case AuthMechanism::DigestMD5: return "DIGEST-MD5";
            case AuthMechanism::OAuthBearer: return "OAUTH-BEARER";
            default:
                return "UNKNOWN";
        }
    }
}