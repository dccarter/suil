/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-17
 */

#pragma once

#include <suil/utils/utils.hpp>

#include <exception>
#include <string>
#include <sstream>
#include <variant>

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
        const std::string_view& tag() const noexcept {
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
        Exception(const std::string_view&
        tag, uint64_t id, Args... args)
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
        std::uint64_t mId{};
        std::string_view mTag;
    };

#define AnT() __PRETTY_FUNCTION__, ":", __LINE__, " "

#define DECLARE_EXCEPTION(Name)                                                 \
    class Name : public suil::Exception {                                        \
    public:                                                                     \
        template <typename... Args>                                             \
        explicit Name (Args... args)                                            \
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
        operator bool() const { return std::holds_alternative<V>(*this); }
        V& operator*() { return std::get<V>(*this); }
        const V& operator*() const { return std::get<V>(*this); }
        const E& exception() const { return std::get<E>(*this); }
        const V& value() const { return std::get<V>(*this); }
        E& exception() { return std::get<E>(*this); }
        V& value() { return std::get<V>(*this); }
        void raise() {
            if (!*this) {
                throw std::move(exception());
            }
        }

        Return<V, E> operator||(V value) {
            if (!std::holds_alternative<V>(*this)) {
                return {std::move(value)};
            }
            return std::move(*this);
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