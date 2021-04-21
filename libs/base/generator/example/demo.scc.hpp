
#pragma once
//
// !!!Generated by scc DO NOT MODIFY!!!
// file: "/tmp/suil/libs/base/generator/example/demo.scc.hpp"
// date: Dec 22 2020 06:09:09
//

#include <iod/symbols.hh>
#include <suil/base/json.hpp>
#include <suil/base/meta.hpp>
#include <suil/base/wire.hpp>
#include <cstddef>
#include <type_traits>
#include <variant>

// load sbg library
#include <string>
#include <iostream>
#include <iod/sio.hh>
// Load suil base generator library used to generate
// meta types
#ifndef IOD_SYMBOL_AccountId
#define IOD_SYMBOL_AccountId
    iod_define_symbol(AccountId)
#endif
#ifndef IOD_SYMBOL_Amount
#define IOD_SYMBOL_Amount
    iod_define_symbol(Amount)
#endif
#ifndef IOD_SYMBOL_Owner
#define IOD_SYMBOL_Owner
    iod_define_symbol(Owner)
#endif
#ifndef IOD_SYMBOL_AccountName
#define IOD_SYMBOL_AccountName
    iod_define_symbol(AccountName)
#endif
#ifndef IOD_SYMBOL_deposit
#define IOD_SYMBOL_deposit
    iod_define_symbol(deposit)
#endif
#ifndef IOD_SYMBOL_create
#define IOD_SYMBOL_create
    iod_define_symbol(create)
#endif
#ifndef IOD_SYMBOL_withdraw
#define IOD_SYMBOL_withdraw
    iod_define_symbol(withdraw)
#endif
#ifndef IOD_SYMBOL_obj
#define IOD_SYMBOL_obj
    iod_define_symbol(obj)
#endif
#ifndef IOD_SYMBOL_name
#define IOD_SYMBOL_name
    iod_define_symbol(name)
#endif
#ifndef IOD_SYMBOL_other
#define IOD_SYMBOL_other
    iod_define_symbol(other)
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

namespace suil {
    
    // generated by sbg/meta
    struct Deposit: iod::MetaType {
        typedef decltype(iod::D(
            prop(AccountId, std::string),
            prop(Amount, int)
        )) Schema;
        
        static const Schema Meta;
        
        
        Deposit(std::string id, int amount)
            : AccountId{std::move(id)},
              Amount{amount}
        {}
        Deposit() = default;

        
        std::string AccountId;
        
        int Amount;
        
        
        static Deposit fromJson(iod::json::parser&);
        void toJson(iod::json::jstream&) const;
        
        std::size_t maxByteSize() const;
        static Deposit fromWire(suil::Wire&);
        void toWire(suil::Wire&) const;
        
    };
    
    
    // generated by sbg/meta
    struct Create: iod::MetaType {
        typedef decltype(iod::D(
            prop(Owner, std::string),
            prop(AccountName, std::string)
        )) Schema;
        
        static const Schema Meta;
        
        
        Create(std::string owner, std::string name)
        : Owner{std::move(owner)},
          AccountName{std::move(name)}
        {}
        Create() = default;

        
        std::string Owner;
        
        std::string AccountName;
        
        
        static Create fromJson(iod::json::parser&);
        void toJson(iod::json::jstream&) const;
        
        std::size_t maxByteSize() const;
        static Create fromWire(suil::Wire&);
        void toWire(suil::Wire&) const;
        
    };
    
    
    // generated by sbg/meta
    struct Withdraw: iod::MetaType {
        typedef decltype(iod::D(
            prop(AccountId, std::string),
            prop(Amount, int)
        )) Schema;
        
        static const Schema Meta;
        
        
        Withdraw(std::string id, int amount)
            : AccountId{std::move(id)},
              Amount{amount}
        {}
        Withdraw() = default;

        
        std::string AccountId;
        
        int Amount;
        
        
        static Withdraw fromJson(iod::json::parser&);
        void toJson(iod::json::jstream&) const;
        
        std::size_t maxByteSize() const;
        static Withdraw fromWire(suil::Wire&);
        void toWire(suil::Wire&) const;
        
    };
    
    
    // generated by sbg/meta
    using CommandUnionVariant = std::variant<Deposit, Create, Withdraw>;
    template <typename T>
    concept IsCommandUnionMember = requires {
        std::is_same_v<T, Deposit> or
        std::is_same_v<T, Create> or
        std::is_same_v<T, Withdraw>;
    };
    
    struct Command: iod::UnionType {
        
        template <typename T>
            requires IsCommandUnionMember<T>
        Command(T value)
            : Value(std::forward<T>(value))
        {}
        Command() = default; 
        
        CommandUnionVariant Value;
        template <typename T>
            requires IsCommandUnionMember<T>
        bool has() const {
            return std::holds_alternative<T>(Value);
        }
        
        template <typename T>
            requires IsCommandUnionMember<T>
        operator const T&() const {
            return std::get<T>(Value);
        }
        
        bool Ok() const { return Value.index() != std::variant_npos; }
        
        static Command fromJson(const std::string& schema, iod::json::parser&);
        void toJson(iod::json::jstream&) const;
        
        const char* name() const;
    private:
        static int index(const std::string& name);
    public:
        
        std::size_t maxByteSize() const;
        static Command fromWire(suil::Wire&);
        void toWire(suil::Wire&) const;
        
    };
    
    
    // generated by sbg/json
    struct JsonOnly: iod::MetaType {
        typedef decltype(iod::D(
            prop(obj, json::Object),
            prop(name, String),
            prop(other(var(ignore), var(optional)), iod::Nullable<json::Object>)
        )) Schema;
        
        static const Schema Meta;
        
        json::Object obj;
        
        String name;
        
        [[ignore]] [[optional]]
        iod::Nullable<json::Object> other;
        
        
        static JsonOnly fromJson(iod::json::parser&);
        void toJson(iod::json::jstream&) const;
        
    };
    
    
    // generated by sbg/wire
    using WireOnlyUnionVariant = std::variant<Deposit, Withdraw, Create>;
    template <typename T>
    concept IsWireOnlyUnionMember = requires {
        std::is_same_v<T, Deposit> or
        std::is_same_v<T, Withdraw> or
        std::is_same_v<T, Create>;
    };
    
    struct WireOnly: iod::UnionType {
        
        template <typename T>
            requires IsWireOnlyUnionMember<T>
        WireOnly(T value)
            : Value(std::forward<T>(value))
        {}
        WireOnly() = default; 
        
        WireOnlyUnionVariant Value;
        template <typename T>
            requires IsWireOnlyUnionMember<T>
        bool has() const {
            return std::holds_alternative<T>(Value);
        }
        
        template <typename T>
            requires IsWireOnlyUnionMember<T>
        operator const T&() const {
            return std::get<T>(Value);
        }
        
        bool Ok() const { return Value.index() != std::variant_npos; }
        
        const char* name() const;
    private:
        static int index(const std::string& name);
    public:
        
        std::size_t maxByteSize() const;
        static WireOnly fromWire(suil::Wire&);
        void toWire(suil::Wire&) const;
        
    };
    
}

#pragma GCC diagnostic pop
