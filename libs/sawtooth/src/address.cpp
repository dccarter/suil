//
// Created by Mpho Mbotho on 2021-06-04.
//

#include <suil/sawtooth/address.hpp>

#include <suil/base/hash.hpp>

namespace suil::saw {

    void DefaultAddressEncoder::isValidAddr(const String& addr)
    {
        if (addr.size() != ADDRESS_LENGTH) {
            throw AddressEncodeError("Address size is invalid, {Expected: ",
                                    ADDRESS_LENGTH, ", Got: ", addr.size(), "}");
        }
        else if (!addr.ishex()) {
            throw AddressEncodeError("Address must be lowercase hexadecimal ONLY");
        }
    }

    void DefaultAddressEncoder::isNamespaceValid(const suil::String& ns)
    {
        if (ns.size() != NS_PREFIX_LENGTH) {
            throw AddressEncodeError("Namespace size is invalid, {Expected: ",
                                    NS_PREFIX_LENGTH, ", Got: ", ns.size(), "}");
        }
        else if (!ns.ishex()) {
            throw AddressEncodeError("Namespace prefix must be lowercase hexadecimal ONLY");
        }
    }

    DefaultAddressEncoder::DefaultAddressEncoder(const suil::String &ns)
            : mNamespace(ns.dup())
    {}

    DefaultAddressEncoder::DefaultAddressEncoder(DefaultAddressEncoder &&other) noexcept
        : mNamespace(std::move(other.mNamespace)),
          mPrefix(std::move(other.mPrefix))
    {}

    DefaultAddressEncoder& DefaultAddressEncoder::operator=(DefaultAddressEncoder &&other) noexcept
    {
        if (this != &other) {
            Ego.mNamespace = std::move(other.mNamespace);
            Ego.mPrefix = std::move(other.mPrefix);
        }
        return Ego;
    }

    suil::String DefaultAddressEncoder::mapNamespace(const suil::String &ns) const
    {
        return suil::SHA512(ns).substr(0, NS_PREFIX_LENGTH, false);
    }

    suil::String DefaultAddressEncoder::mapKey(const suil::String &key)
    {
        return suil::SHA512(key).substr(NS_PREFIX_LENGTH, ADDRESS_LENGTH-NS_PREFIX_LENGTH, false);
    }

    const suil::String& DefaultAddressEncoder::getPrefix()
    {
        if (Ego.mPrefix.empty()) {
            Ego.mPrefix = Ego.mapNamespace(Ego.mNamespace);
            isNamespaceValid(Ego.mPrefix);
        }
        return Ego.mPrefix;
    }

    suil::String DefaultAddressEncoder::operator()(const suil::String &key)
    {
        auto addr = suil::catstr(Ego.getPrefix(), Ego.mapKey(key));
        isValidAddr(addr);
        return addr;
    }


}
