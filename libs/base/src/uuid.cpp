//
// Created by Mpho Mbotho on 2020-10-08.
//

#include "suil/base/uuid.hpp"

namespace suil {

    unsigned char* uuid(uuid_t id) {
        if (uuid_generate_time_safe(id)) {
            return nullptr;
        }
        return id;
    }

    String uuidstr(uuid_t id) {
        static uuid_t UUID;
        if (id == nullptr) {
            id = uuid(UUID);
        }
        size_t olen{(size_t)(20+(sizeof(uuid_t)))};
        char out[olen];
        int i{0}, rc{0};
        for(i; i<4; i++) {
            out[rc++] = i2c((uint8_t) (0x0F&(UUID[i]>>4)));
            out[rc++] = i2c((uint8_t) (0x0F& UUID[i]));
        }
        out[rc++] = '-';

        for (i; i < 10; i++) {
            out[rc++] = i2c((uint8_t) (0x0F&(UUID[i]>>4)));
            out[rc++] = i2c((uint8_t) (0x0F& UUID[i]));
            if (i&0x1) {
                out[rc++] = '-';
            }
        }

        for(i; i<sizeof(uuid_t); i++) {
            out[rc++] = i2c((uint8_t) (0x0F&(UUID[i]>>4)));
            out[rc++] = i2c((uint8_t) (0x0F& UUID[i]));
        }

        return String{out, (size_t) rc, false}.dup();
    }
}