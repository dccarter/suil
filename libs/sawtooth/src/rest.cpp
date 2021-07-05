//
// Created by Mpho Mbotho on 2021-06-04.
//

#include "suil/sawtooth/rest.hpp"

#include <suil/base/base64.hpp>

namespace suil::saw::Client {

    const char* RestApi::BATCHES_RESOURCE{"/batches"};
    const char* RestApi::STATE_RESOURCE{"/state"};
    const char* RestApi::BATCH_STATUSES{"/batch_statuses"};

    RestApi::RestApi(
            const suil::String &url,
            const suil::String &family,
            const suil::String &familyVersion,
            const suil::String &privateKey, int port)
        : mEncoder{family, familyVersion.dup(), privateKey.dup()},
          mAddressEncoder{family},
          mSession(http::cli::loadSession(url, port, {}))
    {}

    Return<String> RestApi::asyncBatches(const Data &payload, const Inputs& inputs, const Outputs& outputs)
    {
        std::vector<Batch> batches;
        batches.push_back(Ego.mEncoder.encode(payload, inputs, outputs));
        return Ego.asyncBatches(batches);
    }

    Return<String> RestApi::asyncBatches(const std::vector<Batch> &batches)
    {
        return TryCatch<String>([&]() -> Return<String> {
            auto resp = http::cli::post(mSession, BATCHES_RESOURCE, [batches](http::cli::Request& req) {
                Encoder::encode(req("application/octet-stream"), batches);
                return true;
            });

            auto body = resp.str();
            if (resp.status() == http::Status::Accepted) {
                auto obj = json::Object::decode(body);
                auto link = ((String) obj("link"));
                auto start = link.find('=') + 1;
                return link.substr(start).dup();
            }
            else {
                return http::HttpError{resp.status(), body};
            }
        });
    }

    Return<Data> RestApi::getState(const suil::String &key, bool encode)
    {
        auto resource = suil::catstr(STATE_RESOURCE, "/", (encode? mAddressEncoder(key): key));
        auto resp = http::cli::get(mSession, resource);
        return TryCatch<Data>([&]() -> Return<Data> {
            auto body = resp.str();
            if (resp.status() == http::Status::Ok) {
                auto res = json::Object::decode(body);
                auto data = (String) res("data");
                Buffer ob;
                Base64::decode(ob, (const uint8_t*) data.data(), data.size());
                return Data(ob.release(), ob.size(), true);
            }
            else {
                return http::HttpError(resp.status(), body);
            }
        });
    }

    Return<std::vector<suil::Data>> RestApi::getStates(const suil::String &prefix)
    {
        return TryCatch<std::vector<suil::Data>>([&]() -> Return<std::vector<suil::Data>> {
            auto resource = suil::catstr(STATE_RESOURCE, "?address=", prefix);
            auto resp = http::cli::get(mSession, resource());
            auto body = resp.str();
            if (resp.status() == http::Status::Ok) {
                auto res = json::Object::decode(body);
                auto data = res("data");
                std::vector<suil::Data> out;
                for (auto&[_, obj] : data) {
                    Buffer ob;
                    auto entry = (String) obj("data");
                    Base64::decode(ob, (const uint8_t*) entry.data(), entry.size());
                    out.emplace_back(ob.release(), ob.size(), true);
                }
                return {std::move(out)};
            } else {
                return http::HttpError{resp.status(), std::move(body)};
            }
        });
    }

    suil::String RestApi::getPrefix()
    {
        return Ego.mAddressEncoder.getPrefix().peek();
    }

    Return<String> RestApi::getBatchStatus(const String& batchId, uint64 wait)
    {
        return TryCatch<String>([&]() -> Return<String> {
            auto resource = suil::catstr(BATCH_STATUSES, "?id=", batchId, "&wait=", wait);
            auto resp = http::cli::get(mSession, resource);

            auto body = resp.str();
            if (resp.status() != http::Ok) {
                return http::HttpError{resp.status(), body};
            }

            auto res = json::Object::decode(body);
            auto status = (String) res("data")(0)("status");
            if (!status) {
                return http::HttpError{http::Unknown, "status not found"};
            }

            return {status.dup()};
        });
    }

    bool RestApi::waitForStatus(const String& batchId, uint64 wait, std::function<bool(const String&)> checker)
    {
        if (checker == nullptr or batchId.empty() or wait == 0) {
            serror("waitForStatus invalid parameters");
            return false;
        }

        auto resource = suil::catstr(BATCH_STATUSES, "?id=", batchId, "&wait=", wait);
        int64_t until = Deadline{int64_t(wait)};
        auto now = mnow();
        while (now < until) {
            auto resp = http::cli::get(mSession, resource);

            auto body = resp.str();
            if (resp.status() != http::Ok) {
                serror("retrieving batch status failed: " PRIs " - " PRIs,
                       _PRIs(http::toString(resp.status())),
                       _PRIs(body));
                return false;
            }
            auto res = json::Object::decode(body);
            auto status = (String) res("data")(0)("status");
            if (checker(status)) {
                return true;
            }
            now = mnow();
        }

        return false;
    }

}