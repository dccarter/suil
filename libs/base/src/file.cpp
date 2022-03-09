//
// Created by Mpho Mbotho on 2020-10-15.
//

#include "suil/base/file.hpp"
#include "suil/base/datetime.hpp"

#include <suil/async/fdops.hpp>

#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>

#include <climits>

namespace {

    bool _mkdir(const char *dir, mode_t m) {
        bool rc = true;
        if (::mkdir(dir, m) != 0)
            if (errno != EEXIST)
                rc = false;
        return rc;
    }

    bool _mkpath(char *p, mode_t m) {
        char *base = p, *dir = p;
        bool rc;

        while (*dir == '/') dir++;

        while ((dir = strchr(dir,'/')) != nullptr) {
            *dir = '\0';
            rc = _mkdir(base, m);
            *dir = '/';
            if (!rc)
                break;

            while (*dir == '/') dir++;
        }

        return _mkdir(base, m);
    }

    void _forall(
            const char *base,
            suil::String& path,
            std::function<bool(const suil::String&, bool)> h,
            bool recursive,
            bool pod)
    {
        suil::String cdir(suil::catstr(base, "/", path));
        DIR *d = opendir(cdir.data());

        if (d == nullptr) {
            /* openining directory failed */
            throw suil::fs::FileIOException("opendir('", path, "') failed: ", errno_s);
        }

        struct dirent *tmp;

        while ((tmp = readdir(d)) != NULL)
        {
            /* ignore parent and current directory */
            if (suil::strmatchany(tmp->d_name, ".", ".."))
                continue;


            suil::String ipath = path?
                    suil::catstr(path, "/", tmp->d_name) :
                    suil::String(tmp->d_name);
            bool is_dir{tmp->d_type == DT_DIR};

            if (!pod) {
                if (!h(ipath, is_dir)) {
                    /* delegate cancelled travesal*/
                    break;
                }

                if (recursive && is_dir) {
                    /* recursively iterate current directory*/
                    _forall(base, ipath, h, recursive, pod);
                }
            }
            else {
                if (recursive && is_dir) {
                    /* recursively iterate current directory*/
                    _forall(base, ipath, h, recursive, pod);
                }

                if (!h(ipath, is_dir)) {
                    /* delegate cancelled travesal*/
                    break;
                }
            }
        }

        closedir(d);
    }

    inline void _remove(const char *path) {
        if (::remove(path) != 0) {
            throw suil::fs::FileIOException("remove '", path, "' failed: ", errno_s);
        }
    }

    inline void _rmdir(const char *path) {
        suil::fs::forall(path,
          [&](const suil::String& name, bool d) -> bool {
              suil::String tmp = suil::catstr(path, "/", name);
              _remove(tmp.data());
              return true;
          }, true, true);
    }
}

namespace suil {

    File::File(int fd, bool own)
        : _fd(fd), _own{own}
    {}

    File::File(const String& name, int flags, mode_t mode)
        : File()
    {

        if (!open(name, flags, mode)) {
            throw fs::FileIOException("opening file '", name(), "' failed: ", errno_s);
        }
    }

    bool File::open(const String& path, int flags, mode_t mode)
    {
        if (_fd != INVALID_FD) {
            errno = EINPROGRESS;
            return false;
        }

        Ego._fd = ::open(path(), flags, mode);
        if (Ego._fd != INVALID_FD) {
            if (!nonblocking(_fd, true)) {
                auto err = errno;
                ::close(_fd);
                _fd = INVALID_FD;
                errno = err;
                return false;
            }
            _own = true;
            return true;
        }

        return false;
    }

    void File::close() {
        if (_fd != INVALID_FD and _own) {
            ::close(_fd);
        }
        _own = false;
        _fd = INVALID_FD;
    }

    off_t File::seek(off_t off) {
        if (_fd == INVALID_FD) {
            // invalid call context
            throw fs::FileIOException("File::seek - unsupported call context");
        }
        return lseek(_fd, off, SEEK_SET);
    }

    bool File::eof() {
        if (_fd == INVALID_FD) {
            // invalid call context
            throw fs::FileIOException("File::eof - unsupported call context");
        }

        auto current = lseek(_fd, 0, SEEK_CUR);
        if (current == -1) {
            return current;
        }

        auto eof = lseek(_fd, 0, SEEK_END);
        if (eof == -1) {
            return eof;
        }

        auto res = lseek(_fd, current, SEEK_SET);
        if (res == -1) {
            return res;
        }
        return (current == eof);

    }

    off_t File::tell() {
        if (_fd == INVALID_FD) {
            // invalid call context
            throw fs::FileIOException("File::tell - unsupported call context");
        }
        return lseek(_fd, 0, SEEK_CUR);
    }

    task<bool> File::read(
            void *buf,
            size_t &len,
            milliseconds timeout)
    {
        bool status{true};
        if (_fd == INVALID_FD) {
            // invalid call context
            throw fs::FileIOException("File::read - unsupported call context");
        }

        auto ret = co_await fdops::read(_fd, {(char *)buf, len}, timeout);
        if (ret < 0) {
            strace("%p: File read error: %s", _fd, errno_s);
            status = false;
        }
        len = ret;
        co_return status;
    }

    task<Status<String>> File::readLine(milliseconds timeout)
    {
        Buffer ob{256};
        while (true) {
            ob.reserve(1);
            auto ret  = co_await fdops::read(_fd, {&ob.data()[ob.size()], 1}, timeout);
            if (errno) {
                co_return Status<String>{errno};
            }
            if (ob.data()[ob.size()] == '\n') {
                break;
            }
            ob.seek(1);
        }

        co_return Ok(String{ob});
    }


    task<size_t> File::write(
            const void *buf,
            size_t len,
            milliseconds timeout)
    {
        if (_fd == INVALID_FD) {
            // invalid call context
            throw fs::FileIOException("File::write - unsupported call context");
        }

        auto ret = co_await fdops::write(_fd, {(const char*)buf, len}, timeout);
        if (errno) {
            strace("%p: File write error: %s", _fd, errno_s);
        }

        co_return size_t(ret);
    }

    File::~File() {
        Ego.close();
    }

    FileLogger::FileLogger(const String& dir, const String& prefix)
    {
        open(dir, prefix);
    }

    void FileLogger::open(const String& dir, const String& prefix) {
        if (!fs::exists(dir())) {
            sdebug("creating file logger directory: %s", dir.c_str());
            fs::mkdir(dir.c_str(), true, 0777);
        }
        String tmp{suil::catstr(dir, "/", prefix, "-", Datetime()("%Y%m%d_%H%M%S"), ".log")};
        _fd = ::open(tmp(), O_WRONLY|O_APPEND|O_CREAT, 0666);
        if (_fd == INVALID_FD) {
            throw fs::FileIOException("creating log file: '", tmp, "' failed: ", errno_s);
        }
    }

    void FileLogger::write(const char *data, size_t sz, Level l, const char *tag)
    {
        if (_fd == INVALID_FD) {
            return;
        }

        ::write(_fd, data, sz);
    }


    void fs::mkdir(const char *path, bool recursive, mode_t mode) {

        bool status;
        String tmp = String{path}.dup();
        if (!tmp) {
            /* creating directory failed */
            throw FileIOException("mkdir '", path, "' failed: ",  errno_s);
        }

        if (recursive)
            status = _mkpath(tmp.data(), mode);
        else
            status = _mkdir(tmp.data(), mode);

        if (!status) {
            /* creating directory failed */
            throw FileIOException("mkdir '", path, "' failed: ",  errno_s);
        }
    }

    void fs::mkdir(const char *base, const std::vector<const char*>& paths, bool recursive, mode_t mode) {
        for (auto& p : paths) {
            if (p[0] == '/') {
                mkdir(p, recursive, mode);
            }
            else {
                String tmp = catstr(base, "/", p);
                mkdir(tmp.data(), recursive, mode);
            }
        }
    }

    void fs::mkdir(const std::vector<const char*>& paths, bool recursive, mode_t mode) {
        char base[PATH_MAX];
        getcwd(base, PATH_MAX);
        mkdir(base, paths, recursive, mode);
    }

    String fs::currDir() {
        char buf[PATH_MAX];
        if (getcwd(buf, PATH_MAX) == nullptr)
            throw FileIOException("getcwd failed: ", errno_s);
        return String{buf}.dup();
    }

    String fs::parent(const String& path, bool dup) {
        auto pos = path.rfind('/');
        if (pos == String::npos) {
            return {};
        }
        auto parent = path.substr(0, pos);
        return dup? parent.dup(): parent;
    }

    String fs::filename(const String& path, bool dup)
    {
        auto pos = path.rfind('/');
        if (pos == String::npos) {
            return dup? path.dup(): path.peek();
        }
        auto fname = path.substr(pos+1);
        return dup? fname.dup() : fname;
    }

    String fs::extension(const String& path, bool dup)
    {
        auto pos = path.rfind('.');
        if (pos == String::npos) {
            return {};
        }
        auto ext = path.substr(pos+1);
        return dup? ext.dup() : ext;
    }

    bool fs::isDirEmpty(const char *path) {
        DIR *dir = opendir(path);
        if (dir == nullptr)
            return false;
        struct dirent *d;
        size_t n{0};
        while ((d = readdir(dir))!= nullptr)
            if (++n > 2) break;
        closedir(dir);

        return (n <= 2);
    }

    void fs::remove(const char *path, bool recursive, bool contents) {
        bool is_dir{isdir(path)};
        if (recursive && is_dir) {
            _rmdir(path);
        }

        if (!is_dir || !contents)
            _remove(path);
    }

    void fs::remove(const char *base, const std::vector<const char*>& paths, bool recursive) {
        for (auto& p : paths) {
            String tmp = p[0] == '/'? suil::catstr(base, p) : suil::catstr(base, "/", p);
            remove(tmp.data(), recursive);
        }
    }

    void fs::remove(const std::vector<const char*>& paths, bool recursive) {
        char base[PATH_MAX];
        getcwd(base, PATH_MAX);
        remove(base, paths, recursive);
    }

    String fs::readall(const char *path, bool async) {
        Buffer b(512);
        fs::readall(b, path, async);
        return String(b);
    }

    void fs::readall(Buffer& out, const char *path, bool async) {
        /* read file contents into a buffer */
        if (!exists(path)) {
            throw fs::FileIOException("file '", path, "' does not exist");
        }

        struct stat st;
        if (::stat(path, &st)) {
            /* getting file information failed */
            throw FileIOException("stat('", path, "') failed: ", errno_s);
        }

        if (st.st_size > 1048576) {
            /* size too large to be read by this API */
            throw FileIOException("file '", path, "' to large (",
                                    st.st_size, " bytes) to be read by fs::read_all");
        }

        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            /* opening file failed */
            throw FileIOException("opening file '", path, "' failed: ", errno_s);
        }

        out.reserve((uint32_t) st.st_size+4);
        char *data = out.data();
        ssize_t nread = 0, rc = 0;
        do {
            rc = ::read(fd, &data[nread], (size_t)(st.st_size - nread));
            if (rc < 0) {
                /* reading file failed */
                throw FileIOException("reading '", path, "' failed: ", errno_s);
            }

            nread += rc;
        } while (nread < st.st_size);
        out.seek(nread);
    }

    AsyncVoid fs::append(const char *path, const void *data, size_t sz, bool async) {
        File f(path, O_WRONLY|O_CREAT|O_APPEND, 0666);
        co_await f.write(data, sz, 1500ms);
        f.close();
    }

    task<bool> fs::read(const char *path, void *data, size_t sz, bool async) {
        if (!fs::exists(path)) {
            co_return false;
        }

        File f(path, O_RDONLY|O_CREAT, 0666);
        co_return co_await f.read(data, sz, 1500ms);
    }

    String fs::realpath(const char *path) {
        char base[PATH_MAX];
        if (::realpath(path, base) == nullptr) {
            if (errno != EACCES && errno != ENOENT)
                return {};
        }

        return std::move(String{base}.dup());
    }

    size_t fs::size(const char *path) {
        struct stat st{};
        if (stat(path, &st) == 0) {
            return (size_t) st.st_size;
        }
        throw FileIOException("file '", path, "' does not exist");
    }

    void fs::touch(const char *path, mode_t mode) {
        if (::open(path, O_CREAT|O_TRUNC|O_WRONLY, mode) < 0) {
            throw FileIOException("touching file '", path, "' failed: ", errno_s);
        }
    }

    void fs::forall(
            const char *path,
            std::function<bool(const String&, bool)> h,
            bool recursive, bool pod)
    {
        String tmp{""};
        _forall(path, tmp, h, recursive, pod);
    }

    std::vector<String> fs::ls(const char *path, bool recursive) {
        std::vector<String> all;
        fs::forall(path, [&all](const String& d, bool dir) {
            all.emplace_back(d.dup());
            return true;
        }, recursive);

        return std::move(all);
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

namespace sfs = suil::fs;

TEST_CASE("suil::filesystem", "[utils][fs]") {
    // creating a single directory
    REQUIRE_NOTHROW(sfs::mkdir("test"));

    SECTION("basic file operations" "[fs][file_ops]") {
        // directory should have been created
        REQUIRE(sfs::exists("test"));
        // shouldn't files that don't exist
        REQUIRE_FALSE(sfs::exists("test1"));
        // Create another directory
        REQUIRE_NOTHROW(sfs::touch("test/file.txt"));
        // File should have been created
        REQUIRE(sfs::exists("test/file.txt"));
        // Size should be 0
        REQUIRE(sfs::size("test/file.txt") == 0);
        // Delete file, should not exist there after
        REQUIRE_NOTHROW(sfs::remove("test/file.txt"));
        // shouldn't files that don't exist
        REQUIRE_FALSE(sfs::exists("test/file.txt"));
    }

    SECTION("basic directory operations", "[fs][dir_ops]") {
        // Using created test directory
        REQUIRE(sfs::isdir("test"));
        // Create directory within test
        REQUIRE_NOTHROW(sfs::mkdir("test/test1"));
        // Should exist
        REQUIRE(sfs::exists("test/test1"));
        // Can be deleted if empty
        REQUIRE_NOTHROW(sfs::remove("test/test1"));
        // Should not exist
        REQUIRE_FALSE(sfs::exists("test/test1"));
        // Directory with parents requires recursive flag
        REQUIRE_THROWS(sfs::mkdir("test/test1/child1"));
        REQUIRE_NOTHROW(sfs::mkdir("test/test1/child1", true));
        // Should exist
        REQUIRE(sfs::exists("test/test1/child1"));
        // Deleting non-empty directory also requires the recursive flag
        REQUIRE_THROWS(sfs::remove("test/test1"));
        REQUIRE_NOTHROW(sfs::remove("test/test1", true));
        // Should not exist
        REQUIRE_FALSE(sfs::exists("test/test1"));
        // Multiple directories can be created at once
        REQUIRE_NOTHROW(sfs::mkdir("test", {"test1", "test2"}));
        // Test should have 2 directories
        REQUIRE(sfs::ls("test").size() == 2);
        // Remove all directories
        REQUIRE_NOTHROW(sfs::remove("test", {"test1", "test2"}));
        REQUIRE(sfs::ls("test").empty());
        // Recursive again
        REQUIRE_NOTHROW(sfs::mkdir("test",
                                         {"test1/child1/gchild1",
                                          "test1/child1/gchild2"
                                          "test1/child2/gchild1/ggchild/ggchild1",}, true));
        // list recursively
        REQUIRE(sfs::ls("test", true).size() == 8);
        // Remove recursively
        REQUIRE_NOTHROW(sfs::remove("test", true, true));
    }

    SECTION("Basic file IO", "[fs][file_io]") {
        const char *fname = "test/file.txt";
        // Reading non-existent file
        suil::String data;
        REQUIRE_THROWS((data = sfs::readall(fname)));
        // Create empty file
        sfs::touch(fname, 0777);
        data = sfs::readall(fname);
        REQUIRE(data.empty());
        // Write data to file
        data = "The quick brown fox";
        sfs::append(fname, data);
        REQUIRE(sfs::size("test/file.txt") == data.size());
        auto out = sfs::readall(fname);
        REQUIRE(data == out);
        // Append to file
        data = suil::catstr(data, "\n jumped over the lazy dog");
        sfs::append(fname, "\n jumped over the lazy dog");
        REQUIRE(sfs::size(fname) == data.size());
        out = sfs::readall(fname);
        REQUIRE(data == out);
        // Clear the file
        sfs::clear(fname);
        REQUIRE(sfs::size(fname) == 0);
    }

    // Cleanup
    REQUIRE_NOTHROW(sfs::remove("test", true));
}
#endif