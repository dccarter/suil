//
// Created by Mpho Mbotho on 2020-11-05.
//

#ifndef SUILBASE_META_HPP
#define SUILBASE_META_HPP

#include "scc/generator.hpp"

namespace scc {
    class Struct;
}

namespace suil {

    enum Kind: int {
        Json = 0x01,
        Wire = 0x02,
        Meta = 0x03 // Both Json and Wire
    };

    class HppSuilMetaGenerator: public scc::HppGenerator {
    public:
        HppSuilMetaGenerator(Kind kind = Kind::Meta);
        void generate(scc::Formatter fmt, const scc::Type &ct) override;
        void includes(scc::Formatter fmt, scc::IncludeBag &incs) override;
    private:
        void generateUnion(scc::Formatter& fmt, const scc::Struct& st);
        void generateUnionV1(scc::Formatter& fmt, const scc::Struct& st);
        void generateUnionV2(scc::Formatter& fmt, const scc::Struct& st);
        void generateEnum(scc::Formatter& fmt, const scc::Enum& e);
        void generateUnionVariant(scc::Formatter& fmt, const scc::Struct& st);
        void generateStruct(scc::Formatter& fmt, const scc::Struct& st);
        bool is(Kind kind) { return (kind & mKind) == kind; }
        Kind mKind;
    };

    class CppSuilMetaGenerator: public scc::CppGenerator {
    public:
        CppSuilMetaGenerator(Kind kind = Kind::Meta);
        void generate(scc::Formatter fmt, const scc::Type &ct) override;

    private:
        void generateUnion(scc::Formatter& fmt, const scc::Struct& st);
        void generateUnionV1(scc::Formatter& fmt, const scc::Struct& st);
        void generateUnionV2(scc::Formatter& fmt, const scc::Struct& st);
        void generateEnum(scc::Formatter& fmt, const scc::Enum& st);
        void generateStruct(scc::Formatter& fmt, const scc::Struct& st);
        bool is(Kind kind) { return (kind & mKind) == kind; }
        Kind mKind;
    };
}

#endif //SUILBASE_META_HPP
