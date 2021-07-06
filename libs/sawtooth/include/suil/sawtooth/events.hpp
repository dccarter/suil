//
// Created by Mpho Mbotho on 2021-06-30.
//

#include <suil/sawtooth/protos.hpp>

#include <suil/net/zmq/patterns.hpp>
#include <suil/net/zmq/context.hpp>

#include <libmill/libmill.hpp>

namespace suil::saw {

    struct FilterType {
        static constexpr  auto Unset{sawtooth::protos::EventFilter::FilterType::EventFilter_FilterType_FILTER_TYPE_UNSET};
        static constexpr  auto SimpleAny{sawtooth::protos::EventFilter::FilterType::EventFilter_FilterType_SIMPLE_ANY};
        static constexpr  auto SimpleAll{sawtooth::protos::EventFilter::FilterType::EventFilter_FilterType_SIMPLE_ALL};
        static constexpr  auto RegexAny{sawtooth::protos::EventFilter::FilterType::EventFilter_FilterType_REGEX_ANY};
        static constexpr  auto RegexAll{sawtooth::protos::EventFilter::FilterType::EventFilter_FilterType_REGEX_ALL};
    };

    using sawtooth::protos::Event;
    struct Filter {
        Filter(String key, String matchString, sawtooth::protos::EventFilter_FilterType filterType);
        sawtooth::protos::EventFilter& operator()() { return mEventFilter; }
    private:
        sawtooth::protos::EventFilter mEventFilter{};
    };


    define_log_tag(SAW_EVENTS);
    DECLARE_EXCEPTION(EventSubscriptionError);

    class EventSubscription : LOGGER(SAW_EVENTS) {
    public:
        using Handler = std::function<void(Event)>;
        EventSubscription(net::zmq::Context& context, String url);
        void subscribe(const String& eventType, std::vector<Filter> filters);
        void operator()(Handler handler);

        void close();

        ~EventSubscription();

    private:
        DISABLE_COPY(EventSubscription);
        DISABLE_MOVE(EventSubscription);

        void sendSubscription(suil::Data payload);
        void sendCancelSubscription();
        static coroutine void receiveEventsAsync(EventSubscription& S);
        net::zmq::Context& mCtx;
        net::zmq::DealerSocket mSock;
        Handler mHandler{nullptr};
        String mId;
        mill::Event mCancelWait{};
    };
}
