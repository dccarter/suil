//
// Created by Mpho Mbotho on 2021-06-04.
//

#include "suil/sawtooth/wallet.hpp"

#include <suil/base/crypto.hpp>
#include <suil/base/encryption.hpp>
#include <suil/base/file.hpp>
#include <suil/base/wire.hpp>

namespace suil::saw::Client {

    static const String EMPTY{nullptr};

    Wallet::Wallet(String&& path, String&& secret)
        : mPath(std::move(path)),
          mSecret(std::move(secret))
    {}

    Wallet::~Wallet() {
        Ego.save();
    }

    Wallet Wallet::open(const String &path, const String& secret)
    {
        if (!fs::exists(path())) {
            throw WalletError("Wallet '", path, "' not found");
        }
        auto encrypted = fs::readall(path());
        if (encrypted.empty()) {
            throw WalletError("Wallet '", path, "' does not contain a valid key");
        }

        auto decrypted = suil::AES_Decrypt(secret, encrypted);
        if (decrypted.empty()) {
            throw WalletError("Unlocking wallet failed, check wallet secret");
        }
        try {
            Wallet w(path.dup(), secret.dup());
            suil::HeapBoard hb(decrypted);
            hb >> w.mData;
            return std::move(w);
        }
        catch (...) {
            throw WalletError("Invalid wallet file contents");
        }
    }

    Wallet Wallet::create(const String& path, const String& secret)
    {
        if (fs::exists(path())) {
            throw WalletError("Wallet '", path, "' already exist");
        }
        Wallet w(path.dup(), secret.dup());
        auto key = crypto::ECKey::generate();
        w.mData.MasterKey = key.getPublicKey().toString();
        w.mDirty = true;
        return w;
    }

    const String& Wallet::get(const suil::String &name) const
    {
        if (name.empty()) {
            return defaultKey();
        }

        auto lowerName = name.dup();
        lowerName.tolower();
        for (const auto& key : mData.Keys) {
            if (key.Name == lowerName) {
                return key.Key;
            }
        }
        return EMPTY;
    }

    const String& Wallet::generate(const suil::String &name)
    {
        auto lowerName = name.dup();
        lowerName.tolower();

        for (const auto& key : mData.Keys) {
            if (key.Name == lowerName) {
                throw WalletError("A key with name '", name, "' already exists in wallet");
            }
        }
        auto key = crypto::ECKey::generate();
        Ego.mData.Keys.emplace_back(std::move(lowerName), key.getPublicKey().toString());
        Ego.mDirty = true;
        return Ego.mData.Keys.back().Key;
    }


    const String& Wallet::defaultKey() const
    {
        return Ego.mData.MasterKey;
    }

    void Wallet::save()
    {
        if (mDirty) {
            mDirty = false;
            HeapBoard hb(1024);
            hb << Ego.mData;
            auto raw = hb.raw();
            auto encrypted = suil::AES_Encrypt(Ego.mSecret, raw);
            if (encrypted.empty()) {
                throw WalletError("Encrypting wallet file failed");
            }
            if (fs::exists(Ego.mPath())) {
                fs::remove(Ego.mPath());
            }
            fs::append(Ego.mPath(), encrypted.data(), encrypted.size(), false);
        }
    }

}