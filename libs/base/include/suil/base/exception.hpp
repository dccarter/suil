//
// Created by Mpho Mbotho on 2020-10-05.
//

#ifndef SUIL_BASE_EXCEPTION_HPP
#define SUIL_BASE_EXCEPTION_HPP

#include <exception>
#include <string>
#include <sstream>
#include <variant>

#include <suil/base/utils.hpp>

namespace suil {

    class Exception: public std::exception {
    public:
        explicit Exception(std::string message);
        COPY_CTOR(Exception) = default;
        MOVE_CTOR(Exception) = default;
        COPY_ASSIGN(Exception) = default;
        MOVE_ASSIGN(Exception) = default;

        /**
         * Get the message that was constructed when creating the exception
         *
         * @return the message constructed when creating the exception
         */
        [[nodiscard]]
        const std::string& message() const noexcept {
            return mMessage;
        }

        /**
         * Gets the tag of the exception instance which is basically
         * the name of the exception kind
         *
         * @return a string pointing to the name of the exception
         */
        [[nodiscard]]
        const std::string& tag() const noexcept {
            return mTag;
        }

        /**
         * Retrieve the ID of the exception which is a hash of the exception
         * kind
         *
         * @return the ID of the exception
         */
        [[nodiscard]]
        int64_t id() const noexcept {
            return mId;
        }

        /**
         * eq equality comparison operator
         * @param other the other exception to compare to
         * @return true if the exceptions have the same code
         */
        inline bool operator==(const Exception& other) const {
            return mId == other.mId;
        }

        /**
         * neq equality comparison operator
         * @param other the other exception to compare to
         * @return true if the exceptions have different codes
         */
        inline bool operator!=(const Exception& other) const {
            return mId != other.mId;
        }

        /**
         * Get the message that was created when creating the exception in C style string
         * format
         *
         * @return the message in C-style string
         */
        [[nodiscard]]
        const char* what() const noexcept override {
            return mMessage.c_str();
        }

        /**
         * Construct exception from current exception
         * @return the current exception
         *
         * @note this function is only applicable on a catch exception
         */
        static Exception fromCurrent();



    protected:
        template <typename... Args>
        Exception(const std::string& tag, uint64_t id, Args... args)
                : mTag{tag},
                  mId{id}
        {
            makeMessage(std::forward<Args>(args)...);
        }

        static std::string makeTag(const std::string_view& sv);
        static std::uint64_t makeId(const std::string& tag);

    private:
        template <typename... Args>
        void makeMessage(Args... args) {
            if (sizeof...(args)) {
                std::stringstream ss;
                (ss << ... << args);
                mMessage = ss.str();
            }
        }

        std::string mMessage{};
        const std::uint64_t mId{};
        const std::string& mTag{};
    };

#define AnT() __PRETTY_FUNCTION__, ":", __LINE__, " "
    
#define DECLARE_EXCEPTION(Name)                                                 \
    class Name : public suil::Exception {                                 \
    public:                                                                     \
        template <typename... Args>                                             \
        Name (Args... args)                                                     \
            : Exception( Name ::Tag(), Name ::Id(), std::forward<Args>(args)...)\
        {}                                                                      \
                                                                                \
        using Exception::operator=;                                             \
        using Exception::operator!=;                                            \
                                                                                \
        static const std::string& Tag() {                                       \
            static std::string _Tag{Exception::makeTag(__PRETTY_FUNCTION__)};   \
            return _Tag;                                                        \
        }                                                                       \
                                                                                \
        static const uint64_t& Id() {                                           \
            static uint64_t _Id{Exception::makeId(Tag())};                      \
            return _Id;                                                         \
        }                                                                       \
    }

    DECLARE_EXCEPTION(AccessViolation);
    DECLARE_EXCEPTION(OutOfRange);
    DECLARE_EXCEPTION(KeyNotFound);
    DECLARE_EXCEPTION(MemoryAllocationFailure);
    DECLARE_EXCEPTION(IndexOutOfBounds);
    DECLARE_EXCEPTION(InvalidArguments);
    DECLARE_EXCEPTION(UnsupportedOperation);

    template <typename V, typename E = Exception>
        requires (std::is_same_v<E, Exception> || std::is_base_of_v<Exception, E>)
    struct Return : public std::variant<V, E> {
        using std::variant<V, E>::variant;
        operator bool() const { return std::holds_alternative<V>(Ego); }
        V& operator*() { return std::get<V>(Ego); }
        const V& operator*() const { return std::get<V>(Ego); }
        const E& exception() const { return std::get<E>(Ego); }
        const V& value() const { return std::get<V>(Ego); }
        E& exception() { return std::get<E>(Ego); }
        V& value() { return std::get<V>(Ego); }
        void raise() {
            if (!Ego) {
                throw std::move(exception());
            }
        }
    };

    template <typename R, typename F, typename... Args>
    Return<R> TryCatch(F func, Args&&... args)
    {
        try {
            return {func(std::forward<Args>(args)...)};
        }
        catch (...) {
            return {Exception::fromCurrent()};
        }
    }

}

#endif //SUIL_BASE_EXCEPTION_HPP
