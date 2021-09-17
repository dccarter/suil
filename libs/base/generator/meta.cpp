//
// Created by Mpho Mbotho on 2020-11-05.
//

#include "meta.hpp"

#include <scc/exception.hpp>
#include <scc/program.hpp>
#include <scc/visitor.hpp>
#include <scc/includes.hpp>

using scc::Struct;
using scc::Symbol;
using scc::Field;
using scc::Node;
using scc::Native;
using scc::Visitor;
using scc::Enum;
using scc::EnumMember;

namespace suil {

    HppSuilMetaGenerator::HppSuilMetaGenerator(Kind kind)
        : mKind{kind}
    {}

    void HppSuilMetaGenerator::includes(scc::Formatter fmt, scc::IncludeBag& incs)
    {
        incs.write(fmt, "suil/base/json.hpp");
        incs.write(fmt, "suil/base/meta.hpp");
        incs.write(fmt, "suil/base/wire.hpp");
        incs.write(fmt, "cstddef");
        incs.write(fmt, "type_traits");
        incs.write(fmt, "variant");
    }

    void HppSuilMetaGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (ct.is<Enum>()) {
            generateEnum(fmt, ct.as<Enum>());
        }
        else if (ct.is<scc::Struct>()) {
            UsingNamespace(getNamespace(), fmt);
            auto& st = ct.as<Struct>();
            if (st.IsUnion) {
                generateUnion(fmt, st);
            }
            else {
                generateStruct(fmt, st);
            }

            if (is(Kind::Wire)) {
                Line(fmt) << "std::size_t maxByteSize() const;";
                Line(fmt) << "static " << st.Name << " fromWire(suil::Wire&);";
                Line(fmt) << "void toWire(suil::Wire&) const;";
                Line(fmt);
            }

            Line(--fmt)<< "};";
        }
        else {
            throw scc::Exception("suil meta generator does not support classes");
        }

        Line(fmt);
    }

    void HppSuilMetaGenerator::generateUnionVariant(scc::Formatter& fmt, const scc::Struct& st)
    {
        Line(fmt) << "using " << st.Name <<"UnionVariant = std::variant<";
        bool first{true};
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (!first) {
                fmt << ", ";
            }
            fmt << field.Type;
            first = false;
        });
        fmt << ">;";
        Line(fmt) << "template <typename T>";
        Line(fmt) << "concept Is" << st.Name <<  "UnionMember = requires {";
        ++fmt;
        first = true;
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (!first) {
                fmt << " or";
            }
            Line(fmt) << "std::is_same_v<T, " << field.Type << ">";
            first = false;
        });
        fmt << ";";
        Line(--fmt)<< "};";
        Line(fmt);
    }

    void HppSuilMetaGenerator::generateUnion(scc::Formatter& fmt, const scc::Struct& st)
    {
        int unionVersionValue{0};
        auto& unionVersion = get("sbgConfig", "unionVersion");
        if (unionVersion) {
            unionVersionValue = unionVersion;
        }
        switch (unionVersionValue) {
            case 0:
            case 1:
                generateUnionV1(fmt, st);
                break;
            case 2:
                generateUnionV2(fmt, st);
                break;
            default:
                throw scc::Exception("unsupported union version '", unionVersionValue, "'");
        }
    }

    void HppSuilMetaGenerator::generateUnionV1(scc::Formatter& fmt, const scc::Struct& st)
    {

        generateUnionVariant(fmt, st);

        Line(fmt) << "struct " << st.Name << ": iod::UnionType {";
        --fmt;

        // Generate constructor that accepts all types
        Line(fmt);
        Line(fmt) << "template <typename T>";
        Line(++fmt) << "requires Is" << st.Name << "UnionMember<T>";
        Line(--fmt) << st.Name << "(T value)";
        Line(++fmt) << ": Value(std::forward<T>(value))";
        Line(--fmt) << "{}";
        // Generate union members
        Line(fmt) << st.Name<< "() = default; ";
        Line(fmt);
        Line(fmt) << st.Name << "UnionVariant Value;";
        Line(fmt) << "template <typename T>";
        Line(++fmt) << "requires Is" << st.Name<< "UnionMember<T>";
        Line(--fmt) << "bool has() const {";
        Line(++fmt) << "return std::holds_alternative<T>(Value);";
        Line(--fmt) << "}";
        Line(fmt);
        Line(fmt) << "template <typename T>";
        Line(++fmt) << "requires Is" << st.Name << "UnionMember<T>";
        Line(--fmt) << "operator const T&() const {";
        Line(++fmt) << "return std::get<T>(Value);";
        Line(--fmt) << "}";
        Line(fmt);
        Line(fmt) << "bool Ok() const { return Value.index() != std::variant_npos; }";
        Line(fmt);

        if (is(Kind::Json)) {
            Line(fmt) << "static " << st.Name << " fromJson(const std::string& schema, iod::json::parser&);";
            Line(fmt) << "void toJson(iod::json::jstream&) const;";
            Line(fmt);
        }

        Line(fmt) << "const char* name() const;";
        Line(--fmt)<< "private:";
        Line(++fmt) << "static int index(const std::string& name);";
        Line(--fmt)<< "public:";
        Line(++fmt);
    }

    void HppSuilMetaGenerator::generateUnionV2(scc::Formatter& fmt, const scc::Struct& st)
    {}

    void HppSuilMetaGenerator::generateStruct(scc::Formatter& fmt, const scc::Struct& st)
    {
        Line(fmt) << "struct " <<  st.Name << ": iod::MetaType {";
        ++fmt;

        Line(fmt) << "typedef decltype(iod::D(";
        ++fmt;

        // create type of meta
        bool first{true};
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (!first) {
                fmt << ",";
            }

            Line(fmt) << "prop(";
            field.Name.toString(fmt);
            if (!field.Attribs.empty()) {
                fmt << "(";
                for (const auto& attrib: field.Attribs) {
                    if (&attrib != &field.Attribs.front()) {
                        fmt << ", ";
                    }
                    attrib.scopedString(fmt, "var");
                    if (attrib.Params.size() == 1) {
                        fmt << " = var(" << attrib.Params[0] << ")";
                    }
                }
                fmt << ")";
            }
            fmt << ", " << field.Type << ")";
            first = false;
        });
        Line(--fmt)<< ")) Schema;";
        Line(fmt);
        Line(fmt) << "static const Schema Meta;";
        Line(fmt);
        Visitor<Struct>(st).visit<Node>([&](const Node& node) {
            if (node.is<Native>() and !node.as<Native>().ForCpp) {
                node.toString(fmt);
            }
            else {
                if (node.is<Field>()) {
                    node.toString(fmt);
                    fmt << ';';
                } else {
                    node.toString(fmt);
                }
                Line(fmt);
            }
        });
        Line(fmt);
        if (is(Kind::Json)) {
            Line(fmt) << "static " << st.Name << " fromJson(iod::json::parser&);";
            Line(fmt) << "void toJson(iod::json::jstream&) const;";
            Line(fmt);
        }
    }

    void HppSuilMetaGenerator::generateEnum(scc::Formatter& fmt, const Enum& e)
    {
        {
            UsingNamespace(getNamespace(), fmt);
            Line(fmt) << "enum ";
            e.Name.toString(fmt);
            if (!e.Base.Content.empty()) {
                // enum has a base
                fmt << ": " << e.Base;
            }
            fmt << " {";
            Line(++fmt);

            bool first{true};
            Visitor<Enum>(e).visit<EnumMember>([&](const EnumMember& member) {
                if (!first) {
                    Line(fmt) << ',';
                }

                fmt << member.Name;
                if (!member.Value.empty()) {
                    fmt << " = " << member.Value;
                }
                first = false;
            });
            Line(--fmt)<< "};";
        }
        {
            UsingNamespace("iod", fmt);
            auto name =  getNamespace() + "::" + e.Name.Content;
            Line(fmt) << "template<>";
            Line(fmt) << "struct is_meta_enum<" << name << "> : std::true_type {};";
            Line(fmt);
            Line(fmt) << "template <>";
            Line(fmt) << "const iod::EnumMeta<" << name << ">& Meta<" << name << ">();";
        }
    }

    CppSuilMetaGenerator::CppSuilMetaGenerator(Kind kind)
        : mKind{kind}
    {}

    void CppSuilMetaGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (ct.is<Enum>()) {
            generateEnum(fmt, ct.as<Enum>());
        }
        else if (ct.is<Struct>()) {
            UsingNamespace(getNamespace(), fmt);
            const auto& st = ct.as<scc::Struct>();
            if (st.IsUnion) {
                generateUnion(fmt, st);
            } else {
                generateStruct(fmt, st);
            }
        }
        else {
            throw scc::Exception("suil meta generator supports enums and struct types only");
        }
    }

    void CppSuilMetaGenerator::generateUnion(scc::Formatter& fmt, const scc::Struct& st)
    {
        if (is(Kind::Wire)) {
            Line(fmt) << "size_t " << st.Name << "::maxByteSize() const {";
            Line(++fmt)
                << "std::size_t size{suil::VarInt(std::variant_size_v<"
                << st.Name
                << "UnionVariant>).length()};";
            Line(fmt) << "std::visit([&](const auto& arg) {";
            Line(++fmt) << "size += suil::Wire::maxByteSize(arg);";
            Line(--fmt)<< "}, Value);";
            Line(fmt) << "return size;";
            Line(--fmt)<< "}";
            Line(fmt);

            Line(fmt) << st.Name << " "<< st.Name << "::fromWire(suil::Wire& w) {";
            Line(++fmt) << st.Name << " tmp{};";
            Line(fmt) << "suil::VarInt index{0};";
            Line(fmt) << "w >> index;";
            Line(fmt) << "switch(std::size_t(index)) {";
            ++fmt;
            int index{0};
            Visitor<Struct>(st).visit<Field>([&](const Field& field) {
                Line(fmt) << "case " << index++ << ": {";
                ++fmt;
                Line(fmt) << "tmp.Value = " << field.Type << "{};";
                Line(fmt) << "w >> (std::get<" << field.Type << ">(tmp.Value));";
                Line(fmt) << "break;";
                Line(--fmt)<< "}";
            });
            Line(fmt) << "case std::variant_npos: {";
            Line(++fmt) << "// invalid variant specified on other size";
            Line(fmt) << "break;";
            Line(--fmt)<< "}";
            Line(fmt) << "default:";
            Line(++fmt)
                    << R"(throw suil::UnsupportedUnionMember("Type '", std::size_t(index), "' is not a a member of this union");)";
            --fmt;
            Line(--fmt) << "}";
            Line(fmt) << "return std::move(tmp);";
            Line(--fmt)<< "}";
            Line(fmt);
            Line(fmt) << "void " << st.Name<< "::toWire(suil::Wire& w) const {";
            Line(++fmt) << "suil::VarInt sz{Value.index()};";
            Line(fmt) << "w << sz;";
            Line(fmt) << "if (Value.index() == std::variant_npos) {";
            Line(++fmt) << "// invalid value, do not serialize";
            Line(fmt) << "return;";
            Line(--fmt)<< "}";
            Line(fmt) << "std::visit([&](const auto& arg) {";
            Line(++fmt) << "w << arg;";
            Line(--fmt)<< "}, Value);";
            Line(--fmt)<< "}";
        }

        if (is(Kind::Json)) {
            fmt << st.Name << ' ' << st.Name
                      << "::fromJson(const std::string& schema, iod::json::parser& p) {";
            Line(++fmt) << st.Name << " tmp{};";
            Line(fmt) << "switch(index(schema)) {";
            int index = 0;
            Visitor<Struct>(st).visit<Field>([&](const Field& field) {
                Line(fmt) << "case " << index++ << ": {";
                Line(++fmt) << field.Type << ' ' << field.Name << ';';
                Line(fmt) << "iod::json_internals::iod_from_json_(("
                          << field.Type << " *) 0, " << field.Name << ", p);";
                Line(fmt) << "tmp = std::move(" << field.Name << ");";
                Line(fmt) << "break;";
                Line(--fmt)<< "}";
            });
            Line(fmt) << "default:";
            Line(++fmt)
                    << R"(throw suil::UnsupportedUnionMember("Type '", std::size_t(index), "' is not a a member of this union");)";
            Line(--fmt)<< "}";
            Line(fmt) << "return std::move(tmp);";
            Line(--fmt)<< "}";
        }

        Line(fmt) << "const char* " << st.Name << "::name() const {";
        Line(++fmt) << "switch(Value.index()) {";
        int index = 0;
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            Line(fmt) << "case " << index++ << ": {";
            Line(++fmt) << "return \"" << field.Name << "\";";
            Line(fmt) << "break;";
            Line(--fmt)<< "}";
        });
        Line(fmt) << "default:";
        Line(++fmt) << R"(throw suil::UnsupportedUnionMember("Type '", std::size_t(index), "' is not a a member of this union");)";
        Line(--fmt)<< "}";
        Line(fmt) << "return \"\";";
        Line(--fmt)<< "}";
        Line(fmt);

        Line(fmt) << "int ";
        st.Name.toString(fmt);
        fmt << "::index(const std::string& name) {";
        index = 0;
        Line(++fmt);
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (index != 0) {
                fmt << "else ";
            }
            fmt << "if (name == \"" << field.Name << "\") {";
            Line(++fmt) << "return " << index++ << "; ";
            Line(--fmt)<< "}";
            Line(fmt);
        });
        Line(fmt) << R"(throw suil::UnsupportedUnionMember("Type '", name, "' is not a a member of this union");)";
        Line(fmt) << "return int(std::variant_npos);";
        Line(--fmt)<< '}';
        Line(fmt);
    }

    void CppSuilMetaGenerator::generateStruct(scc::Formatter& fmt, const scc::Struct& st)
    {
        Line(fmt) << "const " << st.Name << "::Schema " << st.Name << "::Meta{};";
        Line(fmt);
        if (is(Kind::Json)) {
            Line(fmt) << st.Name << ' ' << st.Name << "::fromJson(iod::json::parser& p) {";
            Line(++fmt) << st.Name << " tmp{};";
            Line(fmt) << "p >> p.spaces >> '{';";
            Line(fmt) << "iod::json::iod_attr_from_json(&"
                      << st.Name << "::Meta, tmp, p);";
            Line(fmt) << "p >> p.spaces >> '}';";
            Line(fmt) << "return tmp;";
            Line(--fmt) << "}";
            Line(fmt);

            Line(fmt) << "void " << st.Name << "::toJson(iod::json::jstream& ss) const {";
            Line(++fmt) << "suil::json::metaToJson(Ego, ss);";
            Line(--fmt)<< "}";
            Line(fmt);
        }

        if (is(Kind::Wire)) {
            Line(fmt) << "size_t " << st.Name << "::maxByteSize() const {";
            Line(++fmt) << "return suil::metaMaxByteSize(Ego);";
            Line(--fmt) << "}";
            Line(fmt);

            Line(fmt) << st.Name << " " << st.Name
                      << "::fromWire(suil::Wire& w) {";
            Line(++fmt) << st.Name << " tmp{};";
            Line(fmt) << "suil::metaFromWire(tmp, w);";
            Line(fmt) << "return tmp;";
            Line(--fmt)<< "}";
            Line(fmt);

            Line(fmt) << "void " << st.Name << "::toWire(suil::Wire& w) const {";
            Line(++fmt) << "suil::metaToWire(Ego, w);";
            Line(--fmt)<< "}";
        }
        Line(fmt);
    }

    void CppSuilMetaGenerator::generateEnum(scc::Formatter& fmt, const scc::Enum& e)
    {
        Line(fmt) << "using " << getNamespace() << "::" <<  e.Name << ";";
        Line(fmt);

        UsingNamespace("iod", fmt);
        Line(fmt) << "template <>";
        Line(fmt) << "const iod::EnumMeta<" << e.Name
                  << ">& Meta<" << e.Name << ">() {";
        Line(++fmt);
        Line(fmt) << "static const EnumMeta<" << e.Name << "> sMeta = {";
            Line(++fmt) << '"' << e.Name << "\",";
            Line(fmt) << "{";
                ++fmt;
                bool first{true};
                Visitor<Enum>(e).visit<EnumMember>([&](const EnumMember& member) {
                    if (!first) {
                        fmt << ',';
                    }

                    Line(fmt) << "EnumEntry(" << e.Name << ", " << member.Name << ")";
                    first = false;
                });
                --fmt;
            Line(fmt) << "},";
            if (scc::Attribute::find(e.Attribs, {"sbg", "enum"}).has("encname")) {
                Line(fmt) << "true";
            }
            else {
                Line(fmt) << "false";
            }
            --fmt;
        Line(fmt) << "};";
        Line(fmt) << "return sMeta;";

        Line(--fmt)<< "}";
    }
}

extern "C" {
int  LibInitialize(Context ctx)
{
    scc::registerLibGenerator(
            ctx,
            "meta",
            std::make_shared<suil::HppSuilMetaGenerator>(),
            std::make_shared<suil::CppSuilMetaGenerator>());
    scc::registerLibGenerator(
            ctx,
            "json",
            std::make_shared<suil::HppSuilMetaGenerator>(suil::Kind::Json),
            std::make_shared<suil::CppSuilMetaGenerator>(suil::Kind::Json));
    scc::registerLibGenerator(
            ctx,
            "wire",
            std::make_shared<suil::HppSuilMetaGenerator>(suil::Kind::Wire),
            std::make_shared<suil::CppSuilMetaGenerator>(suil::Kind::Wire));
    return 0;
}

}