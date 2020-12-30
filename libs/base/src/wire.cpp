//
// Created by Mpho Mbotho on 2020-10-15.
//

#include "suil/base/wire.hpp"
#include "suil/base/string.hpp"
#include "suil/base/buffer.hpp"

namespace suil {

    size_t Wire::maxByteSize(const suil::Data &d) {
        VarInt sz(d.size());
        return Wire::maxByteSize(sz) + d.size();
    }

    void Breadboard::toHexStr(Buffer &ob) const {
        auto tmp = Ego.raw();
        ob.reserve((tmp.size()*2)+2);
        ssize_t sz = suil::hexstr(tmp.cdata(), tmp.size(), (char *)&ob[ob.size()], ob.capacity());
        ob.seek(sz);
    }

    bool Breadboard::fromHexStr(suil::String &str) {
        if ((str.size() >> 1) > (M-T))
            return false;
        suil::bytes(str, &sink[T], (M-T));
        T += (str.size() >> 1);
        return true;
    }

    HeapBoard::HeapBoard(size_t size)
            : data((uint8_t *)malloc(size)),
              Breadboard(data, size),
              own{true}
    {
        Ego.sink = Ego.data;
        Ego.M    = size;
    }

    HeapBoard::HeapBoard(const uint8_t *buf, size_t size, bool own)
            : _cdata(buf),
              own{own},
              Breadboard(data, size)
    {
        // push some data into it;
        Ego.sink = data;
        Ego.M    = size;
        Ego.T    = size;
        Ego.H    = 0;
    }

    HeapBoard::HeapBoard(suil::HeapBoard &&hb)
            : HeapBoard(hb.data, hb.M)
    {
        Ego.own = hb.own;
        Ego.H = hb.H;
        Ego.T = hb.T;
        hb.own  = false;
        hb.data = hb.sink = nullptr;
        hb.M = hb.H = hb.T = 0;
    }

    HeapBoard& HeapBoard::operator=(suil::HeapBoard &&hb) {
        Ego.sink = Ego.data = hb.data;
        Ego.own = hb.own;
        Ego.H = hb.H;
        Ego.T = hb.T;
        Ego.M = hb.M;
        hb.own  = false;
        hb.data = hb.sink = nullptr;
        hb.M = hb.H = hb.T = 0;
        return  Ego;
    }

    void HeapBoard::copyfrom(const uint8_t *data, size_t sz) {
        if (data && sz>0) {
            clear();
            Ego.data = (uint8_t *)malloc(sz);
            Ego.sink = Ego.data;
            Ego.M    = sz;
            Ego.own  = true;
            Ego.H    = 0;
            Ego.T    = sz;
            memcpy(Ego.data, data, sz);
        }
    }

    bool HeapBoard::seal() {
        size_t tmp{size()};
        if (Ego.own && tmp) {
            /* 65 K maximum size
             * if (size()) < 75% */
            if (tmp < (M-(M>>1))) {
                void* td = malloc(tmp);
                size_t tm{M}, tt{T}, th{H};
                memcpy(td, &Ego.data[H], tmp);
                clear();
                Ego.sink = Ego.data = (uint8_t *)td;
                Ego.own = true;
                Ego.M   = tm;
                Ego.T   = tt;
                Ego.H   = th;

                return true;
            }
        }
        return false;
    }

    Data HeapBoard::release() {
        if (Ego.size()) {
            // seal buffer
            Ego.seal();
            Data tmp{Ego.data, Ego.size(), Ego.own};
            Ego.own = false;
            Ego.clear();
            return std::move(tmp);
        }

        return Data{};
    }

    Wire& operator>>(suil::Wire &w, suil::String &s)  {
        Data tmp{};
        w >> tmp;
        if (!tmp.empty()) {
            // string is not empty
            s = String((const char*)tmp.data(), tmp.size(), false).dup();
        }
        return w;
    }

    Wire& operator<<(suil::Wire &w, const suil::String &s) {
        return (w << Data(s.m_cstr, s.size(), false));
    }

    size_t String::maxByteSize() const {
        return Wire::maxByteSize(Data{m_cstr, Ego.size(), false});
    }

    void String::clear()
    {
        if (Ego.m_own and Ego.m_str) {
            free(Ego.m_str);
        }
        Ego.m_str = nullptr;
        Ego.m_own = false;
        Ego.m_len = 0;
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

#include "../test/test.hpp"

namespace suil {

    typedef decltype(iod::D(
            test:: prop(a, int),
            test:: prop(b, int)
    )) A;

    struct Mt : iod::MetaType {
        typedef decltype(iod::D(
                test:: prop(a, int),
                test:: prop(b, String)
        )) Schema;
        static Schema Meta;

        int a;
        String b;

        static Mt fromWire(suil::Wire &w) {
            Mt out{};
            iod::foreach(Mt::Meta) |
            [&](auto &m) {
                if (!m.attributes().has(var(unwire)) || w.isFilterOn()) {
                    /* use given metadata to to set options */
                    w >> m.symbol().member_access(out);
                }
            };
            return std::move(out);
        }

        void toWire(suil::Wire &w) const {
            iod::foreach(Mt::Meta) |
            [&](auto &m) {
                if (!m.attributes().has(var(unwire)) || w.isFilterOn()) {
                    /* use given metadata to to set options */
                    w << m.symbol().member_access(Ego);
                }
            };
        }
    };
}

#endif