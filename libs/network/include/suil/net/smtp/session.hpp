//
// Created by Mpho Mbotho on 2020-11-23.
//

#ifndef SUILNETWORK_SESSION_HPP
#define SUILNETWORK_SESSION_HPP

#include "suil/net/smtp/auth.hpp"
#include "suil/net/email.hpp"

#include <suil/base/file.hpp>
#include <suil/base/string.hpp>

#include <typeindex>

namespace suil::net::smtp {

    class SessionData {
    public:
        sptr(SessionData);

        virtual ~SessionData() = default;
    };

    class Session final {
    public:
        enum class State {
            ExpectHello,
            ExpectAuth,
            AuthExpectUsername,
            AuthExpectPassword,
            ExpectMail,
            ExpectRecipient,
            ExpectData,
            Quit
        };

        Session();
        State getState();
        const String& getFrom() const;
        const std::vector<String>& getRecipients() const;
        const String& getId() const;

        template<typename T, typename ...Args>
            requires std::is_base_of_v<SessionData, T>
        void setData(Args&&... args) {
            mData = std::make_shared<T>(std::forward<Args>(args)...);
            mDataId = typeid(T);
        }

        template<typename T>
            requires std::is_base_of_v<SessionData, T>
        T& data() {
            if (std::type_index{typeid(T)} != mDataId) {
                throw UnsupportedOperation("Cannot cast session data object to type ", typeid(T).name());
            }
            if (mData == nullptr) {
                throw UnsupportedOperation("Session data has not been set");
            }

            return  *dynamic_cast<T*>(mData.get());
        }


    private suil_ut:
        friend class ServerContext;
        void reset(State nextState = State::ExpectHello);
        void setAuth(AuthMechanism mechanism, const String& arg0 = {});
        void nextState(State state);

        String id;
        State state{State::ExpectHello};
        String from;
        std::vector<String> recipients;
        ssize_t expectedSize{0};
        Data  rxd;
        std::vector<char> term;
        struct {
            String arg0{};
            AuthMechanism mechanism{AuthMechanism::Plain};
        } auth;
        SessionData::Ptr mData{nullptr};
        std::type_index  mDataId;
    };
}
#endif //SUILNETWORK_SESSION_HPP
