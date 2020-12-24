//
// Created by Mpho Mbotho on 2020-10-07.
//

#include "suil/base/data.hpp"

#include "suil/base/base64.hpp"
#include "suil/base/string.hpp"
#include "suil/base/buffer.hpp"

namespace suil {

    Data::Data()
            : Data((void *)nullptr, 0, false)
    {}

    Data::Data(size_t size)
            : Data()
    {
        if (size) {
            m_data = new uint8_t[size]();
            if (m_data != nullptr) {
                m_size = static_cast<uint32_t>(size);
                m_own = true;
            }
        }
    }

    Data::Data(void *data, size_t size, bool own)
            : m_data((uint8_t*) data),
              m_size((uint32_t)(size)),
              m_own(own)
    {}

    Data::Data(const void *data, size_t size, bool own)
            : m_cdata((uint8_t*) data),
              m_size((uint32_t)(size)),
              m_own(own)
    {}

    Data::Data(const Data& d) noexcept
            : Data()
    {
        if (d.m_size) {
            Ego.m_data = new uint8_t[d.m_size];
            Ego.m_size = d.m_size;
            memcpy(Ego.m_data, d.m_data, d.m_size);
            Ego.m_own  = true;
        }
    }

    Data& Data::operator=(const Data& d) noexcept {
        Ego.clear();
        if (d.m_size) {
            Ego.m_data = new uint8_t[d.m_size];
            Ego.m_size = d.m_size;
            memcpy(Ego.m_data, d.m_data, d.m_size);
            Ego.m_own  = true;
        }

        return Ego;
    }

    Data::Data(Data&& d) noexcept
        : Data(d.m_data, d.m_size, d.m_own)
    {
        d.m_data = nullptr;
        d.m_size = 0;
        d.m_own  = 0;
    }

    Data& Data::operator=(Data&& d) noexcept {
        Ego.clear();
        Ego.m_data = d.m_data;
        Ego.m_size = d.m_size;
        Ego.m_own  = d.m_own;
        d.m_data = nullptr;
        d.m_size = 0;
        d.m_own  = false;
        return Ego;
    }

    Data Data::copy() const {
        Data tmp{};
        if (Ego.m_size) {
            tmp.m_data = new uint8_t[Ego.m_size];
            tmp.m_size = Ego.m_size;
            memcpy(tmp.m_data, Ego.m_data, Ego.m_size);
            tmp.m_own  = true;
        }
        return std::move(tmp);
    }

    void Data::clear() {
        if (Ego.m_own && Ego.m_data) {
            delete [] Ego.m_data;
        }
        Ego.m_data = nullptr;
        Ego.m_size = 0;
        Ego.m_own  = 0;
    }

    Data bytes(const uint8_t *data, size_t size, bool b64)
    {
        if (!b64) {
            auto outSize{(size / 2) + 3};
            auto out = new uint8_t[outSize];
            bytes(String{(const char *) data, size, false}, out, outSize);
            return Data{out, size / 2, true};
        }
        else {
            Buffer ob;
            Base64::decode(ob, data, size);
            size = ob.size();
            return Data{ob.release(), size, true};
        }
    }
}

#ifdef SUIL_UNITTEST
#include "catch2/catch.hpp"

namespace sb = suil;

SCENARIO("Using suil::Data") {
    WHEN("Constructing a new Data instance") {
        sb::Data d1;
        REQUIRE(d1.m_data == nullptr);
        REQUIRE(d1.m_size == 0);

        sb::Data d2(64);
        REQUIRE(d2.m_data != nullptr);
        REQUIRE(d2.m_size == 64);
        REQUIRE(d2.m_own);

        uint8_t nums[4] = {1, 2, 3, 4};
        sb::Data d3(nums, 4, false);
        REQUIRE(d3.m_data == nums);
        REQUIRE(d3.m_size == 4);
        REQUIRE_FALSE(d3.m_own);

        auto* nums2 = new uint8_t[4]{1, 2, 3, 4};
        sb::Data d4(nums2, 4);
        REQUIRE(d4.m_data == nums2);
        REQUIRE(d4.m_own);
    }

    WHEN("Assigning/Copying/Moving Data objects") {
        auto* nums = new uint8_t[4]{1, 2, 3, 4};
        sb::Data d1(nums, 4, true);
        // Copy assignment
        sb::Data d2 = d1;
        REQUIRE_FALSE(d1.m_data == d2.m_data);
        REQUIRE(d2.m_own);
        REQUIRE(d2.m_size == 4);
        // Move assignment
        sb::Data d3 = std::move(d1);
        REQUIRE(d3.m_data == nums);
        REQUIRE(d3.m_own);
        REQUIRE(d3.m_size == 4);
        // Move construction
        auto d4 = ([nums](sb::Data&& d) -> sb::Data {
            REQUIRE(d.m_data == nums);
            REQUIRE(d.m_size == 4);
            REQUIRE(d.m_own);
            return d;
        })(std::move(d3));
        REQUIRE_FALSE(d4.m_data == nums);
        REQUIRE(d4.m_data[0] == 1);
        REQUIRE(d4.m_data[1] == 2);
        REQUIRE(d4.m_data[2] == 3);
        REQUIRE(d4.m_data[3] == 4);
    }

    WHEN("Using Data other API's") {
        sb::Data d1;
        REQUIRE(d1.empty());
        REQUIRE(d1.size() == 0);
        REQUIRE(d1.data() == nullptr);
        REQUIRE(d1.cdata() == nullptr);
        REQUIRE_FALSE(d1.owns());

        sb::Data d2((void*)(&d1), sizeof(d1), false);
        REQUIRE_FALSE(d2.empty());
        REQUIRE(d2.size() == sizeof(d1));
        REQUIRE(d2.data() == (void *)&d1);
        REQUIRE(d2.cdata() == (void *)&d1);
        REQUIRE_FALSE(d2.owns());

        auto d3 = d2.peek();
        REQUIRE_FALSE(d3.empty());
        REQUIRE(d3.size() == sizeof(d1));
        REQUIRE(d3.data() == (void *)&d1);
        REQUIRE(d3.cdata() == (void *)&d1);
        REQUIRE_FALSE(d2.owns());
        REQUIRE((d3 == d2));

        auto d4 = d3.copy();
        REQUIRE_FALSE(d4.empty());
        REQUIRE_FALSE(d4.data() == nullptr);
        REQUIRE(d4.size() == sizeof(d1));
        REQUIRE(d4.owns());
        REQUIRE((d4 == d3));

        auto d5 = d4.peek();
        REQUIRE_FALSE(d5.owns());
        REQUIRE((d5 == d2));

        d5.clear();
        REQUIRE(d5.empty());
        d4.clear();
        REQUIRE(d5.empty());
        REQUIRE_FALSE(d5.owns());
    }
}
#endif