//
// Created by Mpho Mbotho on 2020-10-15.
//

#ifndef SUIL_BASE_FILE_HPP
#define SUIL_BASE_FILE_HPP

#include "suil/base/buffer.hpp"
#include "suil/base/exception.hpp"
#include "suil/base/logging.hpp"
#include "suil/base/string.hpp"
#include "suil/base/utils.hpp"

#include <libmill/libmill.h>

#include <fcntl.h>

namespace suil {

    /**
    * File provides asynchronous operations for writing
            * and reading from files. It's an abstraction on top of libmill's
    * file API's
    */
    class File {
    public:
        File() = default;
        /**
         * Creates a new file taking a lib mill file object
         * @param mf a lib mill file object
         */
        File(mfile mf);

        /**
         * creates a new file which operates on the given file path
         * see syscall open
         *
         * @param path the path of the file to work with
         * @param flags file open flags
         * @param mode file open mode
         */
        File(const String& path, int flags, mode_t mode);
        /**
         * creates a new file which operates on the given file descriptor
         * @param fd the file descriptor to work with
         * @param own if true, the created \class File object will take
         * ownership of the given descriptor and takes care of cleaning it
         * up
         */
        explicit File(int fd, bool own = false);

        File(File&) = delete;
        File&operator=(File&) = delete;
        /**
         * move constructor
         * @param f
         */
        File(File&& f)
                : fd(f.fd)
        {
            f.fd = nullptr;
        }

        /**
         * move assignment
         * @param f
         * @return
         */
        File& operator=(File&& f) {
            fd = f.fd;
            f.fd = nullptr;
            return Ego;
        }

        /**
         * Opens a new file which operates on the given file path
         * see syscall open
         *
         * @param path the path of the file to work with
         * @param flags file open flags
         * @param mode file open mode
         */
        bool open(const String& path, int flags, mode_t mode);

        /**
         * writes the given data to the file
         *
         * @param data pointer to the data to write
         * @param size the size to write to file from data buffer
         * @param dd the deadline time for the write
         * @return number of bytes written to file
         */
        virtual size_t write(
                const void* data,
                size_t size,
                const Deadline& dd = Deadline::infinite());

        /**
         * read data from file
         * @param data the buffer to hold the read data.
         * @param size the size of the given output buffer \param data. after reading,
         * this reference variable will have the number of bytes read into buffer
         * @param dd the deadline time for the read
         * @return true for a successful read, false otherwise
         */
        virtual bool read(
                void *data,
                size_t& size,
                const Deadline& dd = Deadline::infinite());

        /**
         * Reads data from a file until the end of line character is seen
         * @param dd deadline for the read
         * @return result from readline, error code will be 0 for success and
         * the result will be set read input
         */
        virtual Status<String> readLine(const Deadline& dd = Deadline::infinite());

        /**
         * file seek to given offset
         * @param off the offset to seek to
         * @return the new position in file
         */
        virtual off_t  seek(off_t off);
        /**
         * get the offset on the file
         * @return the current offset on the file
         */
        virtual off_t  tell();
        /**
         * check if the offset is at the end of file
         * @return true if offset at end of file
         */
        virtual bool   eof();
        /**
         * flush the data to disk
         * @param dd timestamp to wait for until flush is completed
         */
        virtual void  flush(const Deadline& dd = Deadline::infinite());
        /**
         * closes the file descriptor if it's being owned and it's valid
         */
        virtual void   close();
        /**
         * checks if the given file descriptor is valid
         * @return true if the descriptor is valid, false otherwise
         */
        virtual bool   valid() {
            return fd != nullptr;
        }

        /**
         * get the file descriptor associated with the given file
         * @return
         */
        virtual int raw() {
            return mfget(fd);
        }

        /**
         * eq equality operator, check if two \class File objects
         * point to the same file descriptor
         * @param other
         * @return
         */
        bool operator==(const File& other) {
            return (this == &other)  ||
                   ( fd == other.fd);
        }

        /**
         * neq equality operator, check if two \class File objects
         * point to different file descriptor
         * @param other
         * @return
         */
        bool operator!=(const File& other) {
            return !(*this == other);
        }

        /**
         * stream operator - write a stringview into the file
         * @param sv the string view to write
         * @return
         */
        File& operator<<(strview& sv);

        /**
         * stream operator - write a \class String into the file
         * @param str the \class String to write
         * @return
         */
        File& operator<<(const String& str);

        /**
         * stream operator - write a \class OBuffer into the file
         * @param b the buffer to write to file
         * @return
         */
        File& operator<<(const Buffer& b);

        /**
         * stream operator - write c-style string into file
         * @param str the string to write
         * @return
         */
        File& operator<<(const char* str);

        /**
         * stream operator - write c-style string into file
         * @param str the string to write
         * @return
         */
        inline File& operator<<(const std::string& str) {
            String tmp(str);
            return (Ego << tmp);
        }

        virtual ~File();

    protected:
        mfile  fd{nullptr};
    };

    class FileLogger final : public LogWriter {
    public:
        FileLogger(const String& dir, const String& prefix);
        FileLogger() = default;

        void write(const char *, size_t, Level l, const char* tag) override;

        inline void close() {
            dst.close();
        }

        void open(const String& str, const String& prefix);

        inline ~FileLogger() {
            close();
        }

    private:
        File dst{nullptr};
    };

    namespace fs {

        DECLARE_EXCEPTION(FileIOException);

        String realpath(const char *path);

        size_t size(const char *path);

        void touch(const char *path, mode_t mode=0777);

        inline bool  exists(const char *path) {
            return access(path, F_OK) != -1;
        }

        inline bool isdir(const char *path) {
            struct stat st{};
            return (stat(path, &st) == 0) && (S_ISDIR(st.st_mode));
        }

        String currDir();

        bool isDirEmpty(const char *dir);

        String parent(const String& path, bool dup = true);
        String filename(const String& path, bool dup = true);
        String extension(const String& path, bool dup = true);

        void mkdir(const char *path, bool recursive = false, mode_t mode = 0777);

        void mkdir(const char *base, const std::vector<const char*>& paths, bool recursive = false, mode_t mode = 0777);

        void mkdir(const std::vector<const char*>& paths, bool recursive = false, mode_t mode = 0777);

        void remove(const char *path, bool recursive = false, bool contents = false);

        void remove(const char *base, const std::vector<const char*>& paths, bool recursive = false);

        void remove(const std::vector<const char*>& paths, bool recursive = false);

        void forall(const char *path, std::function<bool(const String&, bool)> h, bool recursive = false, bool pod = false);

        std::vector<String> ls(const char *path, bool recursive = false);

        void readall(Buffer& out, const char *path, bool async = false);

        String readall(const char* path, bool async = false);

        void append(const char *path, const void *data, size_t sz, bool async = true);

        template <DataBuf T>
        inline void append(const char *path, const T& b, bool async = true) {
            append(path, b.data(), b.size(), async);
        }

        inline void append(const char *path, const char* s, bool async = true) {
            append(path, String{s}, async);
        }

        inline void clear(const char *path) {
            if ((::truncate(path, 0) < 0) && errno != EEXIST) {
                throw FileIOException("clearing file '", path, "' failed: ", errno_s);
            }
        }

        template <typename T>
        requires (!std::is_pointer_v<T> && !DataBuf<T>)
        inline void append(const char* path, T d, bool async = true) {
            Buffer b(15);
            b << d;
            append(path, b, async);
        }

        bool read(const char *path, void *data, size_t sz, bool async = true);
    }
}
#endif //SUIL_BASE_FILE_HPP
