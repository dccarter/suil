//
// Created by Mpho Mbotho on 2020-11-21.
//

#ifndef SUILNETWORK_EMAIL_HPP
#define SUILNETWORK_EMAIL_HPP

#include <suil/base/buffer.hpp>
#include <suil/base/exception.hpp>
#include <suil/base/string.hpp>

#include <memory>


namespace suil::net {
namespace smtp { class Client; }

    class Socket;
    DECLARE_EXCEPTION(EmailAttachmentError);

    class Email {
    public:
        sptr(Email);
        DISABLE_COPY(Email);
        MOVE_CTOR(Email) noexcept = default;
        MOVE_ASSIGN(Email) noexcept = default;

        struct Address final {
            Address(const String& email, const String& name = nullptr);
            Address() = default;
            DISABLE_COPY(Address);
            MOVE_CTOR(Address) = default;
            MOVE_ASSIGN(Address) = default;

            String owner{};
            String addr{};
            operator bool() const { return !addr.empty(); }
            void encode(Buffer& dest) const;
            static bool validate(const String& addr);
        };

        struct Attachment final {
            DISABLE_COPY(Attachment);
            using CleanupFn = std::function<void(const String& fname)>;
            Attachment(const String& fname, CleanupFn cleanupFn = nullptr);

            MOVE_CTOR(Attachment);
            MOVE_ASSIGN(Attachment);

            void operator()(Socket& sock, const String& boundary, const Deadline& dd = Deadline::infinite()) const;

            const char* filename() const;

            ~Attachment();

            String fname{};
            String mime{};
            CleanupFn cleanupFn{nullptr};
        };

        Email(Address addr, String subject);

        Email() = default;

        template <typename... Addresses>
        void to(Address&& addr, Addresses&&... addresses)
        {
            addRecipient(std::move(addr));
            if constexpr (sizeof...(addresses) > 0) {
                to(std::forward<Addresses>(addresses)...);
            }
        }

        template <typename... Addresses>
        void cc(Address&& addr, Addresses&&... addresses)
        {
            addCc(std::move(addr));
            if constexpr (sizeof...(addresses) > 0) {
                cc(std::forward<Addresses>(addresses)...);
            }
        }

        template <typename... Addresses>
        void bcc(Address&& addr, Addresses&&... addresses)
        {
            addBcc(std::move(addr));
            if constexpr (sizeof...(addresses) > 0) {
                bcc(std::forward<Addresses>(addresses)...);
            }
        }

        template <typename... Attachments>
        void attach(Attachment&& obj, Attachments&&... attachments)
        {
            addAttachment(std::move(obj));
            if constexpr (sizeof...(attachments) > 0) {
                attach(std::forward<Attachments>(attachments)...);
            }
        }

        void setContentType(String ctype);

        void setBody(const String& msg, String ctype = nullptr);

        Buffer& body();

        std::size_t size() const;
    private:
        friend class smtp::Client;
        String head(const Address& from) const;
        void addRecipient(Address&& addr);
        void addCc(Address&& addr);
        void addBcc(Address&& addr);
        void addAttachment(Attachment&& attachment);
        bool willReceive(const Address& addr);
        String bodyType{"text/plain"};
        String subject{};
        String boundary{};
        std::vector<Address> recipients{};
        std::vector<Address> ccs{};
        std::vector<Address> bccs{};
        std::vector<Attachment> attached{};
        Buffer mBody;
    };
}
#endif //SUILNETWORK_EMAIL_HPP
