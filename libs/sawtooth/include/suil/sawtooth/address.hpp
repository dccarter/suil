//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/base/exception.hpp>
#include <suil/base/string.hpp>


namespace suil::saw {

    DECLARE_EXCEPTION(AddressEncodeError);

    class DefaultAddressEncoder final {
    public:
        static constexpr size_t ADDRESS_LENGTH = 70;
        static constexpr size_t NS_PREFIX_LENGTH = 6;

        DefaultAddressEncoder(const String& ns);
        DefaultAddressEncoder(DefaultAddressEncoder&& other) noexcept ;
        DefaultAddressEncoder&operator=(DefaultAddressEncoder&& other) noexcept ;

        DefaultAddressEncoder(const DefaultAddressEncoder&) = delete;
        DefaultAddressEncoder&operator=(const DefaultAddressEncoder&) = delete;

        String operator()(const String& key);
        const String& getPrefix();

        static void isValidAddr(const String& addr);

        static void isNamespaceValid(const String& ns);
    private:
        String mapNamespace(const String& key) const;
        String mapKey(const String& key);

    private:
        String mNamespace{};
        String mPrefix{};
    };

}

