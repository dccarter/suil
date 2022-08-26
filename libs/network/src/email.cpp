//
// Created by Mpho Mbotho on 2020-11-21.
//

#include "suil/net/email.hpp"
#include "suil/net/socket.hpp"

#include <suil/base/base64.hpp>
#include <suil/base/datetime.hpp>
#include <suil/base/file.hpp>
#include <suil/base/logging.hpp>
#include <suil/base/mime.hpp>
#include <suil/base/regex.hpp>
#include <suil/base/units.hpp>


namespace suil::net {

    define_log_tag(EMAIL);
    static LOGGER(EMAIL) EmailLogger;

    Email::Address::Address(const String& email, const String& name)
        : owner{name.dup()},
          addr{email.dup()}
    {}

    void Email::Address::encode(Buffer& dest) const
    {
        if (!owner.empty()) {
            dest << '\'' << owner << '\'';
        }
        dest << '<' << addr << '>';
    }

    bool Email::Address::validate(const String& addr)
    {
        static std::regex emailRegex{"[\\w\\.]+\\@\\w+(\\.\\w+)*(\\.[a-zA-Z]{2,})"};
        return suil::regex::match(emailRegex, addr.data(), addr.size());
    }

    Email::Attachment::Attachment(const String& fname, CleanupFn cleanupFn)
        : fname{fname.dup()},
          mime{suil::mimetype(fname)},
          cleanupFn{std::move(cleanupFn)}
    {}

    Email::Attachment::Attachment(Attachment&& other)
        : fname{std::move(other.fname)},
          mime{std::move(other.mime)},
          cleanupFn{std::move(other.cleanupFn)}
    {}

    Email::Attachment& Email::Attachment::operator=(Attachment&& other)
    {
        fname = std::move(other.fname);
        mime = std::move(other.mime);
        cleanupFn = std::move(other.cleanupFn);

        return Ego;
    }

    void Email::Attachment::operator()(Socket& sock, const String& boundary, const Deadline& dd) const
    {
        if (!fs::exists(fname())) {
            throw EmailAttachmentError("attachment file '", fname, "' does not exist");
        }

        auto size = fs::size(fname());
        int fd = open(fname(), O_RDONLY, 0777);
        if (fd < 0) {
            throw EmailAttachmentError("could not open attachment '", fname, "': ", errno_s);
        }

        defer({
            // close file when function exits
            close(fd);
        });

        std::size_t writen{0};
        do {
            auto towrite = std::min(size_t(16_Gib), (size - writen));
            auto sent = sock.sendfile(fd, writen, towrite, dd);
            if (sent == 0) {
                throw  EmailAttachmentError("sending attachment '", fname, "' failed: ", errno_s);
            }
            writen += sent;
        } while(writen < size);
    }

    const char* Email::Attachment::filename() const
    {
        const char *tmp = strrchr(fname.data(), '/');
        if (tmp) return tmp+1;
        return fname.data();
    }

    Email::Attachment::~Attachment()
    {
        if (cleanupFn != nullptr) {
            // execute cleanup function
            cleanupFn(fname);
            cleanupFn = nullptr;
        }
    }

    Email::Email(Address addr, String subject)
        : subject{std::move(subject)}
    {
        addRecipient(std::move(addr));
        auto tmp = suil::catstr("suil-", suil::randbytes(4));
        boundary = Base64::encode(tmp);
    }

    void Email::setContentType(String ctype)
    {
        Ego.bodyType = std::move(ctype);
    }

    void Email::setBody(const String& msg, String ctype)
    {
        body() << msg;
        if (ctype) {
            Ego.bodyType = std::move(ctype);
        }
    }

    Buffer& Email::body()
    {
        return mBody;
    }

    String Email::head(const Address& from) const
    {
        constexpr const char* CRLF{"\r\n"};

        Buffer ob(127);
        ob << "From: ";
        from.encode(ob);
        ob << CRLF;

        auto appendAddrs = [&](const std::vector<Address>& addrs) {
            for (const auto& addr: addrs) {
                if (&addr != &addrs.front()) {
                    ob << ", ";
                }
                addr.encode(ob);
            }
        };

        ob << "To: ";
        appendAddrs(recipients);
        ob << CRLF;

        if (!ccs.empty()) {
            ob << "Cc: ";
            appendAddrs(ccs);
            ob << CRLF;
        }

        ob << "Subject: " << subject << CRLF;
        ob << "MIME-Version: 1.0" << CRLF;
        ob << "Date: " << Datetime()(Datetime::HTTP_FMT) << CRLF;

        if (attached.empty()) {
            ob << "Content-Type: " << bodyType << "; charset=utf-8" << CRLF;
            ob << "Content-Transfer-Encoding: 8bit" << CRLF;
        }
        else {
            ob << "Content-Type: multipart/mixed; boundary=\""
               << boundary << "\"" << CRLF;
        }
        ob << CRLF;

        return String{ob};
    }

    void Email::addRecipient(Address&& addr)
    {
        if (!willReceive(addr)) {
            recipients.push_back(std::move(addr));
        }
    }

    void Email::addCc(Address&& addr)
    {
        if (!willReceive(addr)) {
            ccs.push_back(std::move(addr));
        }
    }

    void Email::addBcc(Address&& addr)
    {
        if (!willReceive(addr)) {
            bccs.push_back(std::move(addr));
        }
    }

    void Email::addAttachment(Attachment&& attachment)
    {
        auto it = std::find_if(attached.begin(), attached.end(), [&](const Attachment& a) {
            return a.fname == attachment.fname;
        });
        if (it == attached.end()) {
            attached.push_back(std::move(attachment));
        }
    }

    bool Email::willReceive(const Address& addr)
    {
        auto hasAddr = [&addr](const auto& vec) {
            return std::find_if(vec.begin(), vec.end(), [&addr](const Address& e) {
                return addr.addr == e.addr;
            }) != vec.end();
        };

        return hasAddr(recipients) or hasAddr(ccs) or hasAddr(bccs);
    }
}