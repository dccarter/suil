//
// Created by Mpho Mbotho on 2020-10-07.
//

#ifndef SUIL_BASE_DATA_HPP
#define SUIL_BASE_DATA_HPP

#include "suil/base/utils.hpp"

#include <cstring>

namespace suil {

    /**
     * A data class is used to represent a binary buffer
     */
    class Data {
    public:

        /**
         * Creates an empty useless Data instance
         */
        Data();

        /**
         * Creates a new Data instance and allocates a byte buffer with
         * the given \param size
         *
         * @param size the size of the buffer
         */
        Data(size_t size);

        /**
         * Creates a new Data instance which uses the given data.
         *
         * @param data the buffer to be used by the instance
         * @param size the size of the buffer
         * @param own true is the instance assumes ownership of the data, meaning
         * it is responsible for deallocating the memory
         */
        Data(void *data, size_t size, bool own = true);

        /**
         * Creates a new Data instance which uses the given data.
         *
         * @param data the buffer to be used by the instance
         * @param size the size of the buffer
         * @param own true is the instance assumes ownership of the data, meaning
         * it is responsible for deallocating the memory
         */
        Data(const void *data, size_t size, bool own = true);

        Data(const Data& d) noexcept;
        Data& operator=(const Data& d) noexcept;

        Data(Data&& d) noexcept;
        Data& operator=(Data&& d) noexcept;

        /**
         * Return a reference to a data instance. The returned reference
         * is weak, i.e does not own the underlying and shouldn't outlive
         * the instance
         *
         * @return a reference to this data
         */
        inline Data peek() const {
            Data d{Ego.m_data, Ego.m_size, false};
            return std::move(d);
        }

        /**
         * @return true if the size of the Data instance
         */
        inline bool empty() const {
            return Ego.m_size == 0;
        }

        /**
         * @return the size of the Data instance
         */
        inline size_t size() const {
            return Ego.m_size;
        }

        /**
         * @return a raw pointer to the underlying data instance
         */
        inline uint8_t* data() {
            return Ego.m_data;
        }

        /**
         * @return a raw const pointer to the underlying data instance
         */
        inline const uint8_t* data() const {
            return Ego.m_data;
        }

        /**
         * @return a raw pointer to the underlying data instance
         */
        inline const uint8_t* cdata() const {
            return Ego.m_cdata;
        }

        /**
         * Equality operator
         * @param other the Data instance to compare against
         * @return true if the two instances have the
         * same size and contents of the underlying buffers is similar
         */
        inline bool operator==(const Data& other) const {
            if (Ego.m_size != other.m_size)
                   return false;
            return (Ego.m_size == 0) || memcmp(Ego.m_data, other.m_data, Ego.m_size) == 0;
        }

        /**
         * In-equality operator
         * @param other the Data instance to compare against
         * @return true if the two instances have the
         * different sizes and contents of the underlying buffer's is
         * different
         */
        inline bool operator!=(const Data& other) {
            if (Ego.m_size != other.m_size)
                   return true;
            return (Ego.m_size != 0) && memcmp(Ego.m_data, other.m_data, Ego.m_size) != 0;
        }

        /**
         * @return true if the instance owns the underlying buffer, false otherwise
         */
        inline bool owns() const { return m_own; }

        /**
         * Creates and returns a new Data instance whose data buffer is a copy
         * of the current instance. The returned instance will own it's underlying
         * buffer
         * @return a Data instance which is a copy of the calling instance
         */
        Data copy() const;

        /**
         * Releases ownership of the underlying buffer
         * @return
         */
        uint8* release();

        /**
         * Releases ownership of the underlying buffer
         * @return
         */
        Data release(size_t resize);

        /**
         * Clears the Data instance by deallocating memory if the instance
         * owns the underlying buffer and setting the size to 0
         */
        void clear();

        ~Data() {
            Ego.clear();
        }

    private suil_ut:
        union {
            uint8_t       *m_data{nullptr};
            const uint8_t *m_cdata;
        };
        bool     m_own{false};
        uint32_t m_size{0};
    } __attribute__((aligned(1)));

    /**
     * Unpacks the given string formatted buffer to a bytes data
     * instance
     * @param data the buffer representing string formatted data
     * @param size the size of the buffer
     * @param b64 true if the data is in base64 format false if hex
     * @return bytes data representing the unpacked data
     */
    Data bytes(const uint8_t *data, size_t size, bool b64 = true);

    /**
     * Unpacks the given string formatted buffer to is bytes equivalent
     * buffer
     *
     * @tparam D the type holding the formatted data (must implement the data() and size()
     *  method)
     * @param data base64/hex formatted data instance
     * @param b64 true if the data is in base64 format false if hex
     * @return bytes data representing the unpacked data
     */
    template <DataBuf D>
    Data bytes(const D& data, bool b64 = true) {
        return bytes(static_cast<const uint8_t*>(data.data()), data.size(), b64);
    }
}
#endif //SUIL_BASE_DATA_HPP
