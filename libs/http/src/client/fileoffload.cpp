//
// Created by Mpho Mbotho on 2020-12-29.
//

#include "suil/http/client/fileoffload.hpp"

namespace suil::http::cli {

    FileOffload::FileOffload(String path, int64 timeout)
        : File(nullptr),
          _timeout{timeout}
    {
        _handler = [path = std::move(path), this](const char *data, size_t len) {
            if (data == nullptr and len > 0) {
                // initial condition
                if (!Ego.open(path, O_WRONLY|O_CREAT, 0644)) {
                    throw HttpException("opening offload file '", path, "' failed: ", errno_s);
                }
            }
            if (len == 0) {
                // terminal condition
                flush(_timeout);
            }
            else if (data != nullptr) {
                auto written = Ego.write(data, len, _timeout);
                if (written != len) {
                    // failed to write data to file
                    throw HttpException("Offloading data to file '", path, "' failed: ", errno_s);
                }
            }
            return true;
        };
    }

    bool FileOffload::operator()(const char* data, size_t len)
    {
        return _handler(data, len);
    }

}