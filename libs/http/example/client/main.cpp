//
// Created by Mpho Mbotho on 2020-12-30.
//

#include "suil/http/client/session.hpp"
#include <suil/base/console.hpp>

namespace http = suil::http;
namespace Pri = suil::Console;

using http::cli::Form;
using http::cli::UpFile;

void showResponse(http::cli::Response& resp, const suil::String& m);

int main(int argc, char *argv[])
{
    auto sess = http::cli::Session::load("https://httpbin.org", 0, {});
    {
        auto h = sess.handle();
        auto resp = http::cli::get(h, "/get");
        showResponse(resp, "/get");

        resp = http::cli::post(h, "/post", [](http::cli::Request& req) -> bool {
            req.args("id", 5, "name", "Carter");
            req << Form(Form::MultipartForm,
                    "username", "mmbotho",
                    "age", 31,
                    "data", UpFile{"CMakeFiles/CMakeDirectoryInformation.cmake"});
            return true;
        });
        showResponse(resp, "/post");
    }
    return EXIT_SUCCESS;
}

void showResponse(http::cli::Response& resp, const suil::String& m)
{
    if (resp) {
        Pri::cprintf(Pri::GREEN, 1, "https://httpbin.org/" PRIs, _PRIs(m));
        Pri::green(" headers\n");
        for (auto& hdr: resp.headers()) {
            Pri::cprintf(Pri::YELLOW, 1, "\t" PRIs " = " PRIs "\n",
                         _PRIs(hdr.first), _PRIs(hdr.second));
        }

        Pri::cprintf(Pri::GREEN, 1, "https://httpbin.org/" PRIs, _PRIs(m));
        Pri::green(" body\n");
        auto body = resp.str();
        printf("\t" PRIs "\n", _PRIs(body));
    }
    else {
        Pri::red("Request failed: %s", http::toString(resp.status())() );
    }
}