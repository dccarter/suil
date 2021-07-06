//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/base/string.hpp>
#include <suil/sawtooth/sawtooth.scc.hpp>

namespace suil::saw::Client {

    DECLARE_EXCEPTION(WalletError);

    class Wallet final {
    public:
        Wallet(Wallet&& other) = default;
        Wallet&operator=(Wallet&&) = default;

        static Wallet open(const String& path, const String& secret);
        static Wallet create(const String& path, const String& secret);
        const String& get(const String& name = "") const;
        const String& generate(const String& name);
        const String& defaultKey() const;
        const WalletSchema& schema() { return mData; }
        void save();

        ~Wallet();

    private:
        DISABLE_COPY(Wallet);
        Wallet(String&& path, String&& secret);

    private:
        WalletSchema mData;
        String mPath;
        String mSecret;
        bool mDirty{false};
    };


}
