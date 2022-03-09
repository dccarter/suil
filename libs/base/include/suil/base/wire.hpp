//
// Created by Mpho Mbotho on 2020-10-15.
//

#ifndef SUIL_BASE_WIRE_HPP
#define SUIL_BASE_WIRE_HPP

#include "suil/base/varint.hpp"
#include "suil/base/blob.hpp"
#include "suil/base/symbols.hpp"
#include "suil/base/sio.hpp"

#include <suil/utils/exception.hpp>

#include <iod/symbol.hh>

namespace suil {

    struct Buffer;
    struct String;

    class Wire {
    public:
        inline bool push(const uint8_t e[], size_t es) {
            return forward(e, es) == es;
        }

        inline void pull(uint8_t e[], size_t es) {
            reverse(e, es);
        }

        Wire& operator<<(const VarInt& vi) {
            uint8_t sz{vi.length()};
            push(&sz, 1);
            std::integral auto tmp = vi.read<uint64_t>();
            push((uint8_t *)&tmp, sz);
            return Ego;
        }

        static size_t maxByteSize(const VarInt& vi) {
            return 1 + vi.length();
        }

        Wire& operator>>(VarInt& vi) {
            uint8_t sz{0};
            pull(&sz, 1);
            // actual value
            uint64_t tmp{0};
            Ego.pull((uint8_t *)&tmp, sz);
            vi.write(tmp);
            return Ego;
        }

        Wire& operator<<(const Data& d) {
            VarInt sz(d.size());
            Ego << sz;
            push(d.cdata(), d.size());
            return Ego;
        }

        static size_t maxByteSize(const Data& d);

        Wire& operator>>(Data& d) {
            VarInt tmp(0);
            Ego >> tmp;
            std::integral auto sz = tmp.read<uint64_t>();
            if (sz) {
                // get access to serialization buffer
                if (!Ego.copyOut)
                    d = Data(Ego.reverse(sz), sz, false);
                else
                    d = Data(Ego.reverse(sz), sz, false).copy();
            }
            return Ego;
        }

        template <size_t N>
        Wire& operator<<(const Blob<N>& b) {
            VarInt sz(b.size());
            Ego << sz;
            push(&b.cbin(), b.size());
            return Ego;
        }

        template <size_t N>
        static size_t maxByteSize(const Blob<N>& b) {
            return Wire::maxByteSize(VarInt{b.size()}) =+ b.size();
        }

        template <size_t N>
        Wire& operator>>(Blob<N>& b) {
            VarInt tmp(0);
            Ego >> tmp;
            std::integral auto sz = tmp.read<uint64_t>();
            if (sz) {
                // get access to serialization buffer
                b.copy(Ego.reverse(sz), sz);
            }
            return Ego;
        }

        template <typename T>
            requires std::is_arithmetic_v<T>
        Wire& operator<<(const T& val) {
            uint64_t tmp{0};
            memcpy(&tmp, &val, sizeof(T));
            tmp = htole64((uint64_t) tmp);
            forward((uint8_t *) &tmp, sizeof(val));
            return Ego;
        }

        template <typename T>
        requires std::is_arithmetic_v<T>
        inline Wire& operator>>(T& val) {
            uint64_t tmp{0};
            reverse((uint8_t *) &tmp, sizeof(val));
            tmp = le64toh(tmp);
            memcpy(&val, &tmp, sizeof(T));
            return Ego;
        };

        template <typename T>
            requires (std::is_enum_v<T> and std::is_integral_v<std::underlying_type_t<T>>)
        Wire& operator<<(const T& val) {
            return (Ego << std::underlying_type_t<T>(val));
        }

        template <typename T>
            requires (std::is_enum_v<T> and std::is_integral_v<std::underlying_type_t<T>>)
        Wire& operator>>(T& val) {
            std::underlying_type_t<T> tmp;
            Ego >> tmp;
            val = (T) tmp;
            return Ego;
        }

        template <typename T>
            requires std::is_arithmetic_v<T>
        static size_t maxByteSize(const T val) {
            return sizeof(val);
        }

        template <typename T>
            requires (std::is_base_of_v<iod::MetaType, T> || std::is_base_of_v<iod::UnionType, T>)
        static size_t maxByteSize(const T& val) {
            return val.maxByteSize();
        }

        template <typename T>
            requires (std::is_base_of_v<iod::MetaType, T> || std::is_base_of_v<iod::UnionType, T>)
        Wire& operator>>(T& val) {
            val = T::fromWire(Ego);
            return Ego;
        }

        template <typename T>
            requires (std::is_base_of_v<iod::MetaType, T> || std::is_base_of_v<iod::UnionType, T>)
        Wire& operator<<(const T& val) {
            val.toWire(Ego);
            return Ego;
        }

        template <typename T>
            requires (std::is_enum_v<T> and std::is_integral_v<std::underlying_type_t<T>>)
        static size_t maxByteSize(const T& val) {
            return maxByteSize(std::underlying_type_t<T>(val));
        }

        template <typename T>
        static size_t maxByteSize(const iod::Nullable<T>& val) {
            if (val.has_value()) {
                return 1 + Wire::maxByteSize(val.value());
            }
            return 1;
        }

        template <typename T>
        Wire& operator<<(const iod::Nullable<T>& val) {
            Ego << val.has_value();
            if (val.has_value()) {
                return (Ego << val.value());
            }
            return Ego;
        }

        template <typename T>
        inline Wire& operator>>(iod::Nullable<T>& val) {
            bool isSet{false};
            Ego >> isSet;
            if (isSet) {
                T tmp;
                Ego >> tmp;
                val = std::move(tmp);
            } else {
                val = {};
            }
            return Ego;
        };

        template <typename... T>
        Wire& operator<<(const iod::sio<T...>& o) {
            iod::foreach2(o) |
            [&](auto &m) {
                if (!m.attributes().has(var(unwire)) || Ego.always) {
                    /* use given metadata to to set options */
                    Ego << m.symbol().member_access(o);
                }
            };
            return Ego;
        }

        template <typename... T>
        static size_t maxByteSize(const iod::sio<T...>& o) {
            size_t totalBytes{0};
            iod::foreach2(o) |
            [&](auto &m) {
                /* use given metadata to to set options */
                totalBytes += Wire::maxByteSize(m.symbol().member_access(o));
            };
            return totalBytes;
        }

        template <typename... T>
        Wire& operator>>(iod::sio<T...>& o) {
            iod::foreach2(o) |
            [&](auto &m) {
                if (!m.attributes().has(var(unwire)) || Ego.always) {
                    /* use given metadata to to set options */
                    Ego >> m.symbol().member_access(o);
                }
            };
            return Ego;
        }

        template <typename T>
        Wire& operator<<(const std::vector<T>& v) {
            VarInt sz{v.size()};
            Ego << sz;
            for (auto& e: v) {
                Ego << e;
            }
            return Ego;
        }

        template <typename T>
        static size_t maxByteSize(const std::vector<T>& v) {
            VarInt sz{v.size()};
            size_t totalBytes = Wire::maxByteSize(sz);
            for (auto& e: v) {
                totalBytes += Wire::maxByteSize(e);
            }
            return totalBytes;
        }

        template <typename T>
        Wire& operator>>(std::vector<T>& v) {
            VarInt sz{0};
            Ego >> sz;
            uint64_t entries{sz.read<uint64_t>()};
            for (int i = 0; i < entries; i++) {
                T entry{};
                Ego >> entry;
                v.emplace_back(std::move(entry));
            }

            return Ego;
        }

        template <typename... T>
        Wire& operator<<(const std::vector<iod::sio<T...>>& v) {
            VarInt sz{v.size()};
            Ego << sz;
            for (const iod::sio<T...>& e: v) {
                Ego << e;
            }

            return Ego;
        }

        template <typename... T>
        static size_t maxByteSize(const std::vector<iod::sio<T...>>& v) {
            VarInt sz{v.size()};
            size_t totalBytes = Wire::maxByteSize(sz);
            for (const iod::sio<T...>& e: v) {
                totalBytes += Wire::maxByteSize(e);
            }
            return totalBytes;
        }

        template <typename... T>
        Wire& operator>>(std::vector<iod::sio<T...>>& v) {
            VarInt sz{0};
            Ego >> sz;
            uint64_t entries{sz.read<uint64_t>()};
            for (int i = 0; i < entries; i++) {
                iod::sio<T...> entry;
                Ego >> entry;
                v.emplace_back(std::move(entry));
            }

            return Ego;

        }

        inline Wire& operator<<(const char* str) {
            return (Ego << Data(str, strlen(str), false));
        }

        static size_t maxByteSize(const char* str) {
            return Wire::maxByteSize(Data{str, strlen(str), false});
        }

        inline Wire& operator<<(const std::string& str) {
            return (Ego << Data(str.data(), str.size(), false));
        }

        static size_t maxByteSize(const std::string& str) {
            return Wire::maxByteSize(Data{str.data(), str.size(), false});
        }

        inline Wire& operator>>(std::string& str) {
            // automatically decode reverse
            Data rv;
            Ego >> rv;
            if (!rv.empty()) {
                // this will copy out the string
                str = std::string((char *)rv.data(), rv.size());
            }

            return Ego;
        }

        inline Wire& operator()(bool al) {
            Ego.always = al;
            return Ego;
        }

        virtual bool move(size_t size) = 0;

        virtual Data raw() const = 0;

        bool isFilterOn() const { return !always; }

        inline void setCopyOut(bool en) { Ego.copyOut = en; }

    protected suil_ut:
        virtual size_t   forward(const uint8_t e[], size_t es) = 0;
        virtual void     reverse(uint8_t e[], size_t es) = 0;
        virtual const uint8_t *reverse(size_t es) = 0;
        bool           always{false};
        bool           copyOut{false};
    };

    DECLARE_EXCEPTION(BufferOutOfMemory);

    class Breadboard : public Wire {
    public:
        Breadboard(uint8_t* buf, size_t sz)
            : sink(buf),
              M(sz)
        {}

        size_t forward(const uint8_t e[], size_t es) override {
            if ((M-T) < es) {
                throw BufferOutOfMemory("Breadboard buffer out of memory, requested: ", es,
                                        " available: ", (M-T));
            }
            memcpy(&sink[T], e, es);
            T += es;
            return es;
        }

        void reverse(uint8_t e[], size_t es) override {
            if (es <= (T-H)) {
                memcpy(e, &sink[H], es);
                H += es;
            }
            else
                throw  BufferOutOfMemory("Breadboard out of read buffer bytes, requested: ", es,
                                         " available: ", (T-H));
        }

        const uint8_t* reverse(size_t es) override {
            if (es <= (T-H)) {
                size_t tmp{H};
                H += es;
                return &sink[tmp];
            }
            else
                throw  BufferOutOfMemory("Breadboard out of read buffer bytes, requested: ", es,
                                         " available: ", (T-H));
        }

        virtual uint8_t *rd() {
            return &sink[H];
        }

        bool move(size_t size) override {
            if (Ego.size() >= size) {
                Ego.H += size;
                return true;
            }
            return false;
        }

        inline void reset() {
            H = T = 0;
        }

        ~Breadboard() {
            reset();
        }

        inline size_t size() const {
            return T - H;
        }

        Data raw() const {
            return Data{&sink[H], size(), false};
        };

        void toHexStr(Buffer& ob) const;

        bool fromHexStr(String& str);

    protected suil_ut:
        friend
        Wire&operator<<(Wire& w, const Breadboard& bb) {
            if (&w == &bb) {
                // cannot serialize to self
                throw InvalidArguments("serialization loop detected, cannot serialize to self");
            }
            return (w << bb.raw());
        }

        uint8_t     *sink;
        size_t       H{0};
        size_t       T{0};
        size_t       M{0};
    };

    class HeapBoard : public Breadboard {
    public:
        HeapBoard(size_t size);

        HeapBoard(const uint8_t *buf = nullptr, size_t size = 0, bool own = false);

        explicit HeapBoard(const Data& data)
            : HeapBoard(data.cdata(), data.size())
        {}

        HeapBoard(const HeapBoard& hb)
                : HeapBoard()
        {
            if (hb.data != nullptr)
                Ego.copyfrom(hb.data, hb.M);
        }

        HeapBoard&operator=(const HeapBoard& hb) {
            if (hb.data != nullptr)
                Ego.copyfrom(hb.data, hb.M);
            return Ego;
        }

        HeapBoard(HeapBoard&& hb);

        template <typename T>
        static Data serialize(const T& in, size_t off = 0) {
            HeapBoard hb(Wire::maxByteSize(in) + std::min(off, size_t(16)));
            for (int i = 0; i < off; i++)
                hb << '\0';
            hb << in;
            return hb.release();
        }

        template <typename T, typename D>
        static void parse(T& dest, const D& data) {
            suil::HeapBoard bb(data.data(), data.size());
            bb.setCopyOut(true);
            bb >> dest;
        }

        HeapBoard& operator=(HeapBoard&& hb);

        void copyfrom(const uint8_t* data, size_t sz);

        bool seal();

        inline void clear() {
            if (Ego.own && Ego.data) {
                free(Ego.data);
                Ego.H = Ego.M = Ego.T;
                Ego.own = false;
            }
            Ego.sink = Ego.data = nullptr;
        }

        virtual ~HeapBoard() {
            clear();
        }

        Data release();

    private suil_ut:
        union {
            uint8_t *data{nullptr};
            const uint8_t* _cdata;
        };
        bool      own{false};
    };

    template <size_t N=8192>
    class StackBoard: public Breadboard {
        static_assert(((N<=8192) && ((N&(N-1))==0)), "Requested Breadboard size greater than 8912");
    public:
        StackBoard()
            : Breadboard(data, N)
        {}

    private suil_ut:
        uint8_t   data[N];
    };

    template <MetaWithSchema Mt>
    inline void metaFromWire(Mt& o, suil::Wire& w) {
        iod::foreach(Mt::Meta) |
        [&](auto &m) {
            if (!m.attributes().has(var(unwire)) || w.isFilterOn()) {
                /* use given metadata to to set options */
                w >> m.symbol().member_access(o);
            }
        };
    }

    template <MetaWithSchema Mt>
    inline size_t metaMaxByteSize(Mt& o) {
        size_t totalBytes{0};
        iod::foreach(Mt::Meta) |
        [&](auto &m) {
            /* use given metadata to to set options */
            totalBytes += Wire::maxByteSize(m.symbol().member_access(o));
        };
        return totalBytes;
    }

    template <MetaWithSchema Mt>
    inline void metaToWire(const Mt& o, suil::Wire& w) {
        iod::foreach(Mt::Meta) |
        [&](auto &m) {
            if (!m.attributes().has(var(unwire)) || w.isFilterOn()) {
                /* use given metadata to to set options */
                w << m.symbol().member_access(o);
            }
        };
    }
}
#endif //SUIL_BASE_WIRE_HPP
