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
        if (!ct.is<scc::Struct>()) {
            throw scc::Exception("suil meta generator can only be used on structs");
        }

        const auto& st = ct.as<scc::Struct>();
        if (st.IsUnion) {
            generateUnionVariant(fmt, st);
        }

        fmt() << "struct ";
        st.Name.toString(fmt);
        fmt(true) << (st.IsUnion? ": iod::UnionType {": ": iod::MetaType {");
        fmt.push(false);
        if (st.IsUnion) {
            generateUnion(fmt, st);
        }
        else {
            generateStruct(fmt, st);
        }

        if (is(Kind::Wire)) {
            fmt() << "std::size_t maxByteSize() const;";
            fmt() << "static ";
            st.Name.toString(fmt);
            fmt(true) << " fromWire(suil::Wire&);";
            fmt() << "void toWire(suil::Wire&) const;";
            fmt();
        }

        fmt.pop() << "};";
        fmt();
    }

    void HppSuilMetaGenerator::generateUnionVariant(scc::Formatter& fmt, const scc::Struct& st)
    {
        fmt() << "using ";
        st.Name.toString(fmt);
        fmt(true) << "UnionVariant = std::variant<";
        bool first{true};
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (!first) {
                fmt(true) << ", ";
            }
            field.Type.toString(fmt);
            first = false;
        });
        fmt(true) << ">;";
        fmt() << "template <typename T>";
        fmt() << "concept Is";
        st.Name.toString(fmt);
        fmt(true) << "UnionMember = requires {";
        fmt.push(false);
        first = true;
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (!first) {
                fmt(true) << " or";
            }
            fmt() << "std::is_same_v<T, ";
            field.Type.toString(fmt);
            fmt(true) << ">";
            first = false;
        });
        fmt(true) << ";";
        fmt.pop() << "};";
        fmt();
    }

    void HppSuilMetaGenerator::generateUnion(scc::Formatter& fmt, const scc::Struct& st)
    {
        // Generate constructor that accepts all types
        fmt();
        fmt() << "template <typename T>";
        fmt.push() << "requires Is";
        st.Name.toString(fmt);
        fmt(true) << "UnionMember<T>";
        fmt.pop();
        st.Name.toString(fmt);
        fmt(true) << "(T value)";
        fmt.push() << ": Value(std::forward<T>(value))";
        fmt.pop() << "{}";
        // Generate union members
        fmt();
        st.Name.toString(fmt);
        fmt(true) << "() = default; ";
        fmt();
        fmt();
        st.Name.toString(fmt);
        fmt(true) << "UnionVariant Value;";
        fmt() << "template <typename T>";
        fmt.push() << "requires Is";
        st.Name.toString(fmt);
        fmt(true) << "UnionMember<T>";
        fmt.pop()  << "bool has() const {";
        fmt.push() << "return std::holds_alternative<T>(Value);";
        fmt.pop()  << "}";
        fmt();
        fmt() << "template <typename T>";
        fmt.push() << "requires Is";
        st.Name.toString(fmt);
        fmt(true) << "UnionMember<T>";
        fmt.pop()  << "operator const T&() const {";
        fmt.push() << "return std::get<T>(Value);";
        fmt.pop()  << "}";
        fmt();
        fmt() << "bool Ok() const { return Value.index() != std::variant_npos; }";
        fmt();

        if (is(Kind::Json)) {
            fmt() << "static ";
            st.Name.toString(fmt);
            fmt(true) << " fromJson(const std::string& schema, iod::json::parser&);";
            fmt() << "void toJson(iod::json::jstream&) const;";
            fmt();
        }

        fmt() << "const char* name() const;";
        fmt.pop() << "private:";
        fmt.push() << "static int index(const std::string& name);";
        fmt.pop() << "public:";
        fmt.push();
    }

    void HppSuilMetaGenerator::generateStruct(scc::Formatter& fmt, const scc::Struct& st)
    {
        fmt() << "typedef decltype(iod::D(";
        fmt.push(false);

        // create type of meta
        bool first{true};
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (!first) {
                fmt(true) << ",";
            }

            fmt() << "prop(";
            field.Name.toString(fmt);
            if (!field.Attribs.empty()) {
                fmt(true) << "(";
                for (const auto& attrib: field.Attribs) {
                    if (&attrib != &field.Attribs.front()) {
                        fmt(true) << ", ";
                    }
                    attrib.scopedString(fmt, "var");
                }
                fmt(true) << ")";
            }
            fmt(true) << ", ";
            field.Type.toString(fmt);
            fmt(true) << ")";
            first = false;
        });
        fmt.pop() << ")) Schema;";
        fmt();
        fmt() << "static const Schema Meta;";
        fmt();
        Visitor<Struct>(st).visit<Node>([&](const Node& node) {
            if (node.is<Native>() and !node.as<Native>().ForCpp) {
                node.toString(fmt);
            }
            else {
                if (node.is<Field>()) {
                    node.toString(fmt);
                    fmt(true) << ';';
                } else {
                    node.toString(fmt);
                }
                fmt();
            }
        });
        fmt();
        if (is(Kind::Json)) {
            fmt() << "static ";
            st.Name.toString(fmt);
            fmt(true) << " fromJson(iod::json::parser&);";
            fmt() << "void toJson(iod::json::jstream&) const;";
            fmt();
        }
    }

    CppSuilMetaGenerator::CppSuilMetaGenerator(Kind kind)
        : mKind{kind}
    {}

    void CppSuilMetaGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (!ct.is<scc::Struct>()) {
            throw scc::Exception("suil meta generator can only be used on structs");
        }

        const auto& st = ct.as<scc::Struct>();
        if (st.IsUnion) {
            generateUnion(fmt, st);
        }
        else {
            generateStruct(fmt, st);
        }
    }

    void CppSuilMetaGenerator::generateUnion(scc::Formatter& fmt, const scc::Struct& st)
    {
        if (is(Kind::Wire)) {
            fmt();
            fmt() << "size_t ";
            st.Name.toString(fmt);
            fmt(true) << "::maxByteSize() const {";
            fmt.push() << "std::size_t size{suil::VarInt(std::variant_size_v<";
            st.Name.toString(fmt);
            fmt(true) << "UnionVariant>).length()};";
            fmt() << "std::visit([&](const auto& arg) {";
            fmt.push() << "size += suil::Wire::maxByteSize(arg);";
            fmt.pop() << "}, Value);";
            fmt() << "return size;";
            fmt.pop() << "}";
            fmt();

            fmt();
            st.Name.toString(fmt);
            fmt(true) << " ";
            st.Name.toString(fmt);
            fmt(true) << "::fromWire(suil::Wire& w) {";
            fmt.push();
            st.Name.toString(fmt);
            fmt(true) << " tmp{};";
            fmt() << "suil::VarInt index{0};";
            fmt() << "w >> index;";
            fmt() << "switch(std::size_t(index)) {";
            fmt.push(false);
            int index{0};
            Visitor<Struct>(st).visit<Field>([&](const Field& field) {
                fmt() << "case ";
                fmt(true) << index++ << ": {";
                fmt.push(false);
                fmt() << "tmp.Value = ";
                field.Type.toString(fmt);
                fmt(true) << "{};";
                fmt() << "w >> (std::get<";
                field.Type.toString(fmt);
                fmt(true) << ">(tmp.Value));";
                fmt() << "break;";
                fmt.pop() << "}";
            });
            fmt() << "case std::variant_npos: {";
            fmt.push() << "// invalid variant specified on other size";
            fmt() << "break;";
            fmt.pop() << "}";
            fmt() << "default:";
            fmt.push()
                    << R"(throw suil::UnsupportedUnionMember("Type '", std::size_t(index), "' is not a a member of this union");)";
            fmt.pop(false);
            fmt.pop() << "}";
            fmt() << "return std::move(tmp);";
            fmt.pop() << "}";
            fmt();
            fmt() << "void ";
            st.Name.toString(fmt);
            fmt(true) << "::toWire(suil::Wire& w) const {";
            fmt.push() << "suil::VarInt sz{Value.index()};";
            fmt() << "w << sz;";
            fmt() << "if (Value.index() == std::variant_npos) {";
            fmt.push() << "// invalid value, do not serialize";
            fmt() << "return;";
            fmt.pop() << "}";
            fmt() << "std::visit([&](const auto& arg) {";
            fmt.push() << "w << arg;";
            fmt.pop() << "}, Value);";
            fmt.pop() << "}";
        }

        if (is(Kind::Json)) {
            fmt();
            st.Name.toString(fmt);
            fmt(true) << ' ';
            st.Name.toString(fmt);
            fmt(true) << "::fromJson(const std::string& schema, iod::json::parser& p) {";
            fmt.push();
            st.Name.toString(fmt);
            fmt(true) << " tmp{};";
            fmt() << "switch(index(schema)) {";
            int index = 0;
            Visitor<Struct>(st).visit<Field>([&](const Field& field) {
                fmt() << "case " << index++ << ": {";
                fmt.push();
                field.Type.toString(fmt);
                fmt(true) << ' ';
                field.Name.toString(fmt);
                fmt(true) << ';';
                fmt() << "iod::json_internals::iod_from_json_((";
                field.Type.toString(fmt);
                fmt(true) << " *) 0, ";
                field.Name.toString(fmt);
                fmt(true) << ", p);";
                fmt() << "tmp = std::move(";
                field.Name.toString(fmt);
                fmt(true) << ");";
                fmt() << "break;";
                fmt.pop() << "}";
            });
            fmt() << "default:";
            fmt.push()
                    << R"(throw suil::UnsupportedUnionMember("Type '", std::size_t(index), "' is not a a member of this union");)";
            fmt.pop() << "}";
            fmt() << "return std::move(tmp);";
            fmt.pop() << "}";
        }

        fmt();
        fmt() << "const char* ";
        st.Name.toString(fmt);
        fmt(true) << "::name() const {";
        fmt.push() << "switch(Value.index()) {";
        int index = 0;
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            fmt() << "case " << index++ << ": {";
            fmt.push() << "return \"";
            field.Name.toString(fmt);
            fmt(true) << "\";";
            fmt() << "break;";
            fmt.pop() << "}";
        });
        fmt() << "default:";
        fmt.push() << R"(throw suil::UnsupportedUnionMember("Type '", std::size_t(index), "' is not a a member of this union");)";
        fmt.pop() << "}";
        fmt() << "return \"\";";
        fmt.pop() << "}";
        fmt();

        fmt() << "int ";
        st.Name.toString(fmt);
        fmt(true) << "::index(const std::string& name) {";
        index = 0;
        fmt.push();
        Visitor<Struct>(st).visit<Field>([&](const Field& field) {
            if (index != 0) {
                fmt(true) << "else ";
            }
            fmt(true) << "if (name == \"";
            field.Name.toString(fmt);
            fmt(true) << "\") {";
            fmt.push() << "return " << index++ << "; ";
            fmt.pop() << "}";
            fmt();
        });
        fmt() << R"(throw suil::UnsupportedUnionMember("Type '", name, "' is not a a member of this union");)";
        fmt() << "return int(std::variant_npos);";
        fmt.pop() << '}';
        fmt();
    }

    void CppSuilMetaGenerator::generateStruct(scc::Formatter& fmt, const scc::Struct& st)
    {
        fmt();
        fmt() << "const ";
        st.Name.toString(fmt);
        fmt(true) << "::Schema ";
        st.Name.toString(fmt);
        fmt(true) << "::Meta{};";
        fmt();
        if (is(Kind::Json)) {
            fmt();
            st.Name.toString(fmt);
            fmt(true) << ' ';
            st.Name.toString(fmt);
            fmt(true) << "::fromJson(iod::json::parser& p) {";
            fmt.push();
            st.Name.toString(fmt);
            fmt(true) << " tmp{};";
            fmt() << "p >> p.spaces >> '{';";
            fmt() << "iod::json::iod_attr_from_json(&";
            st.Name.toString(fmt);
            fmt(true) << "::Meta, tmp, p);";
            fmt() << "p >> p.spaces >> '}';";
            fmt() << "return tmp;";
            fmt.pop() << "}";
            fmt();

            fmt() << "void ";
            st.Name.toString(fmt);
            fmt(true) << "::toJson(iod::json::jstream& ss) const {";
            fmt.push() << "suil::json::metaToJson(Ego, ss);";
            fmt.pop() << "}";
            fmt();
        }

        if (is(Kind::Wire)) {
            fmt() << "size_t ";
            st.Name.toString(fmt);
            fmt(true) << "::maxByteSize() const {";
            fmt.push() << "return suil::metaMaxByteSize(Ego);";
            fmt.pop() << "}";
            fmt();

            fmt();
            st.Name.toString(fmt);
            fmt(true) << " ";
            st.Name.toString(fmt);
            fmt(true) << "::fromWire(suil::Wire& w) {";
            fmt.push();
            st.Name.toString(fmt);
            fmt(true) << " tmp{};";
            fmt() << "suil::metaFromWire(tmp, w);";
            fmt() << "return tmp;";
            fmt.pop() << "}";
            fmt();

            fmt() << "void ";
            st.Name.toString(fmt);
            fmt(true) << "::toWire(suil::Wire& w) const {";
            fmt.push() << "suil::metaToWire(Ego, w);";
            fmt.pop() << "}";
        }
        fmt();
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