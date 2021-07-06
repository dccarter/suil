//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/sawtooth/client.hpp>
#include <suil/sawtooth/address.hpp>

#include <suil/http/client/session.hpp>

namespace suil::saw::Client {

    class RestApi {
    public:
        sptr(RestApi);
        RestApi(const String& url,
                 const String& family,
                 const String& familyVersion,
                 const String& privateKey,
                 int port = 8008);

        RestApi(RestApi&&) = delete;
        RestApi(const RestApi&) = delete;
        RestApi& operator=(RestApi&&) = delete;
        RestApi& operator=(const RestApi& ) = delete;

        Return<String> asyncBatches(const Data& payload, const Inputs& inputs = {}, const Outputs& outputs = {});
        Return<String> asyncBatches(const std::vector<Batch>& batches);
        Return<Data> getState(const String& key, bool encode = true);
        Return<std::vector<Data>> getStates(const suil::String& prefix);
        Return<String> getBatchStatus(const String& batchId, uint64 wait);

        String getPrefix();
        Encoder& encoder() { return  mEncoder; }
    protected:
        bool waitForStatus(const String& batchId, uint64 wait, std::function<bool(const String&)> checker);
        Encoder mEncoder;
        DefaultAddressEncoder mAddressEncoder;
        http::cli::Session mSession;

    private:
        static const char* BATCHES_RESOURCE;
        static const char* STATE_RESOURCE;
        static const char* BATCH_STATUSES;
    };


}