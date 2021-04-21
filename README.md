# `SCF` 
Suil C++ Framework - a mordern `C++` micro framework packaged in various libraries that can be used to develop micro service components.

[SCF Documentation](https://suilteam.com)

## Hello World
```c++
#include "suil/http/server/endpoint.hpp"

namespace hs   = suil::http::server;
namespace net  = suil::net;

int main(int argc, char *argv[])
{
    using Server = hs::Endpoint<>;
    // Create an http endpoint that uses default configurations
    Server ep("/");

    // GET http://localhost/hello
    Route(ep, "/hello")
    ("GET"_method, "OPTIONS"_method)
    (Desc{"Route returns hello world"})
    .attrs(opt(ReplyType, "text/plain"))
    ([&]() {
        // body will be cleared after response
        return "Hello World!";
    });

    // GET http://localhost/echo
    Route(ep, "/echo")
    ("GET"_method, "OPTIONS"_method)
    (Desc{"Route returns whatever was given in the request body"})
    .attrs(opt(ReplyType, "text/plain"))
    ([&](const hs::Request& req, hs::Response& res) {
        // Chunked response buffer to support zero copy
        res.chunk({const_cast<char *>(req.body().data()), req.body().size(), 0});
    });

    return ep.start();
}

```

# `SCF` Features
Everything in `suil` is developed with coroutines in mind (thanks to Libmill). Imagine go but in C++
| Features | What's up |
|:-----------|:-------------|
| **[Suil Code Compiler](/documentation/scc)** | `scc` is a plugin base tool that can transform a **defined** subset of `C++` header file content to a syntax tree that can be crunched by plugins (`generators`) to generate code. |
| **[Logging](/documentation/libs/base/logging)** | Customizable `printf` style logging |
| **[Mustache](/documentation/libs/base/mustache)** | `SCF` includes a cached mustache template parser and renderer |
| **[JSON](/documentation/libs/base/json)** | Includes a static JSON decoder/encoder from IOD and dynamic parser/encoder |
| **[Cryptography wrappers](/documentation/libs/base/crypto)** | Wrappers around OpenSSL API's - mostly the the kind you will find in blockchain implementations |
| **[Binary Serialization protocol](/documentation/libs/base/wire)** | A custom binary serialization protocol (currently work in progress and open for major improvements) which is dubbed `suil` |
| **[PostgreSQL database](/documentation/libs/database/postgres)** | A very easy to use  PostgreSQL client. This client is based on coroutines and provides a nice API. Supports cached connections and statements
| **[Redis Database Client](/documentation/libs/database/redis)** | A hand written Redis client based on the spec. Provides cached connections and implements most features of Redis. It is easy to add features to this client to keep up with redis |
| **[Sockets Library](/documentation/libs/network/socket)** | Libmill provide some API's that abstract Unix socket API's. `SCF` Sockets Library provide easy to use `C++` abstractions of top of libmill abstractions |
| **[ZMQ Abstraction](/documentation/libs/network/zmq)** | A very beautiful and easy to use ZMQ API abstraction on to of the `C` ZMQ library |
| **[SMTP Client and Server](/documentation/libs/network/zmq)** | A fully fledged SMTP client which can be used to send emails to SMTP servers. The server implementation is minimal and can be used to prototype an SMTP servers for testing environments. An `SmtpOutbox` is provided to avoid having to wait for mail server to respond when sending SMTP emails |
| **[RPC Framework](/documentation/libs/scf)** | `SCF` provides A gRPC like easy to use framework but very light weight based on **[`Suil Code Compiler`](/documentation/scc)** and corresponding `rpc` generator written for `scc`. |
| **[JSON RPC](/documentation/libs/rpc/json)** | Clean and easy to use JSON RPC framework and is compliant with JSON RPC 2.0 spec. |
| **[Suil RPC](/documentation/libs/rpc/suil)** | An RPC which custom `suil` binary serialization protocol (binary) for encoding data that goes on the wire |
| **[`scc` RPC generator](/documentation/libs/rpc/generator)** | An `scc` generator which generates RPC services from `.scc` code files. i.e this generator transforms a `C++` class definition (with some syntactic sugar) into either `SRPC`/`JRPC` client API and server API' |
| **[HTTP/1 Web API](/documentation/libs/http/server)** | The backbone of this project started it all. This API tries to simplify developing HTTP web applications and features with `C++`; See the HTTP features below |
| **[HTTP/1 Client](/documentation/libs/http/client)** | A clean session based HTTP/1 client |
| **[WebSocket Server](/documentation/libs/http/websocket)** | Provides API's that can be used to develop robust websocket servers |
| **[HyperLedger Sawtooth SDK](/documentation/libs/sawtooth)** | A fully fledged and easy to use HyperLedger sawtooth client |
| **[Mongo DB CLient](/documentation/libs/database/mongo)** | Coming soon...  building from spec... |

## `SCF` HTTP/1 Web API Features
- An easy to use [parameter based static and dynamic routing](/documentation/libs/http/server/routing) API
- Middleware based endpoints
- [Application Initialization Middleware](/documentation/libs/http/server/app-init) which can be used to provide an initialization route for the API while blocking all the other routes
- [System Attributes Middleware](/documentation/libs/http/server/sysattrs) can be added to the endpoint to allow adding features that toggle options on the endpoint (e.g enabling/disabling authentication on specific route)
- One-line method call for [Route Administration and API documentation](/documentation/libs/http/server/route-admin), this expose's routes that can be used to administer the endpoint, including enabling/disabling routes and querying route metadata (Admin Only)
- [JSON Web Token](/documentation/libs/http/jwt) API's which can be used to create JWT tokens and verify them
- [JWT Authorization Middleware](/documentation/libs/http/server/jwtauth) is available for JWT authentication. When added to an endpoint, it will validate all routes marked with `Authorize` attribute and reject all unauthorized requests
- [JWT Session Middleware](/documentation/libs/http/server/jwtsession) builds on top of JWT Authorization  and Redis Middlewares by implementing session based JWT (i.e the refresh/accept token ideology)
- [Redis](/documentation/libs/http/server/redis-mw) and [PostgreSQL](/documentation/libs/http/server/postgres-mw) middlewares which when attached to an endpoint, can be used as database connection providers for the endpoint
- [File Server](libraries/http/server/fs) which can be used to serve the contents of any directory. This is still missing some features but thrives to be **very** fast and reliable.
  * Supports cached files, i.e files are cached in an `L.R.U` cache
  * Range based requests
  * Cache headers
  * Easy configuration API. e.g `fs.mime("js", opt(allowCache, true))` will allow the server to cache javascript files
  * _There is still some of work to here but current implementation serves basic needs_
  
## Benchmarking (in progress)
This will be done right once the docs site is done. But for an overview,
testing on my laptop yields the following results
##### Environment
* OSX BigSur LTS (application running on docker container)
* 2.8 GHz Quad-Core Intel Core i7
  - 4 cores, 8 threads
  - 12Gb RAM
* Application configured with 6 threads
* [wrk2](https://github.com/giltene/wrk2) master running on same machine

### Application output
```shell
curl -i http://localhost:8000/hello ; echo
HTTP/1.1 200 OK
Connection: Keep-Alive
KeepAlive: 300000
Strict-Transport-Security: max-age 3600; includeSubdomains
Content-Type: application/json
Server: Suil-Http-Server
Date: Wed, 21 Apr 2021 01:12:38 GMT
Content-Length: 11

Hello World
```

### Benchmark results
```bash
wrk -t3 -c1000 -R1M http://0.0.0.0:8000/hello
Running 10s test @ http://0.0.0.0:8000/hello
  3 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.32s     1.96s    7.35s    58.71%
    Req/Sec       -nan      -nan   0.00      0.00%
  2003273 requests in 10.00s, 473.80MB read
Requests/sec: 200347.19
Transfer/sec:     47.38MB
```

## Giving credits where they are due
The following list of open source libraries are used in `SCF` is third party dependencies

| Library | How it is used |
|:--------|:---------------|
|**[Libmill](http://libmill.org)** | Libmill is the core of this project. It provides the the coroutine concurrency used across the the project. `SCF` uses a modified version of this library [Suil/Libmill](https://gitlab.com/sw-devel/thirdparty/libmill). The main modification introduced in the aforementioned repo is multi threading and API's that are commonly used in multi thread applications |
| **[IOD](https://github.com/matt-42/iod)** | IOD is a library that enhances `C++` meta programing. `suil` uses this library intensively more specifically around it's `json` encoding/decoding features and it's symbol types. An extensible code generator ([`scc` or Suil Code Compiler](https://gitlab.com/sw-devel/tools/scc)), was created to support generating IOD symbols and `suil` meta types. |
| **[Crow](https://github.com/ipkn/crow)** | Crow provides the routing and middleware dispatch magic that is used in the HTTP server code. See [suil/http/server/crow.hpp](https://github.com/dccarter/suil/blob/main/libs/http/include/suil/http/server/crow.hpp), [suil/http/server/rch.hpp](https://github.com/dccarter/suil/blob/main/libs/http/include/suil/http/server/rch.hpp) which includes code copied from crow and modified accordingly |
| **[Catch](https://github.com/catchorg/Catch2)** | Catch is used across the project for writing unit tests |

Any other code that might have been used, either modified or copied as is, the original licence text kept at the top of the file or comment with a link to the source of the snippet is included
