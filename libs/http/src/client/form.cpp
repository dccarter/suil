//
// Created by Mpho Mbotho on 2020-12-24.
//

#include "suil/http/client/form.hpp"
#include "suil/http/common.hpp"

namespace suil::http::cli {

    Form::File::File(String path, String ctype)
        : path{std::move(path)},
          contentType{std::move(ctype)}
    {}

    Form::Form(Form&& other) noexcept
        : _boundary{std::move(other._boundary)},
          _encoding{std::exchange(other._encoding, 0)},
          _files{std::move(other._files)},
          _uploads{std::move(other._uploads)},
          _data{std::move(other._data)}
    {}

    Form& Form::operator=(Form&& o) noexcept
    {
        if (&o == this) {
            return Ego;
        }

        _boundary = std::move(o._boundary);
        _encoding = std::exchange(o._encoding, 0);
        _files = std::move(o._files);
        _uploads  = std::move(o._uploads);
        _data = std::move(o._data);
        return Ego;
    }

    Form::~Form()
    {
        clear();
    }

    std::size_t Form::encode(Buffer& dst) const
    {
        if (_files.empty() and _data.empty()) {
            // nothing to encode
            return 0;
        }

        if (_encoding == UrlEncoded) {
            return encodeUrlEncoded(dst);
        }

        return encodeMultipart(dst);
    }

    std::size_t Form::encodeUrlEncoded(Buffer& dst) const
    {
        for (auto it = _data.begin(); it != _data.end(); it++) {
            if (it != _data.begin()) {
                dst << "&";
            }
            dst << it->first << '=' << it->second;
        }

        return dst.size();
    }

    std::size_t Form::encodeMultipart(Buffer& dst) const
    {
#undef  CRLF
#define CRLF "\r\n"
        for (auto& data: _data) {
            dst << "--" << _boundary << CRLF;
            dst << "Content-Disposition: form-data; name=\""
                << data.first << "\"" << CRLF;
            dst << CRLF << data.second << CRLF;
        }

        if (_files.empty()) {
            dst << "--" << _boundary << "--";
            return dst.size();
        }

        size_t contentLength{dst.size()};
        for (auto& ff: _files) {
            Buffer tmp(127);
            tmp << "--" << _boundary << CRLF;
            tmp << "Content-Disposition: form-data; name=\""
                << ff.name << "\"; filename=\"" << fs::filename(ff.path, false)
                << "\"" << CRLF;
            if (ff.contentType) {
                /* append content type header */
                tmp << "Content-Type: " << ff.contentType << CRLF;
            }
            tmp << CRLF;
            contentLength += tmp.size() + ff.size;

            Upload upload(&ff, String(tmp, true));
            _uploads.push_back(std::move(upload));
            /* \r\n will added at the end of each upload */
            contentLength += sizeofcstr(CRLF);
        }

        /* --boundary--  will be appended by Request::submit */
        contentLength += sizeofcstr("----") + _boundary.size();
        return contentLength;
    }

    Form::Upload::Upload(File const* file, String head)
        : _file{file},
          _head{std::move(head)}
    {}

    Form::Upload::Upload(Upload&& o) noexcept
        : _file{std::exchange(o._file, nullptr)},
          _head(std::move(o._head)),
          _fd{std::exchange(o._fd, -1)}
    {}

    Form::Upload::~Upload()
    {
        _file = nullptr;
        Ego.close();
    }

    Form::Upload& Form::Upload::operator=(Upload&& o) noexcept
    {
        if (&o == this) {
            return Ego;
        }
        _file = std::exchange(o._file, nullptr);
        _head = std::move(o._head);
        _fd = std::exchange(o._fd, -1);

        return Ego;
    }

    int Form::Upload::open()
    {
        if (_fd > 0) {
            return _fd;
        }

        _fd = ::open(_file->path(), O_RDONLY);
        if (_fd < 0) {
            throw HttpException("opening upload '", _file->path, "' failed: ", errno_s);
        }

        return _fd;
    }

    void Form::Upload::close()
    {
        if (_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    void Form::clear()
    {
        _uploads.clear();
        _files.clear();
        _data.clear();
    }
}