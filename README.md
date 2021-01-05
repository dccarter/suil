# suil
A lightweight mordern `C++` micro framework with lots of goodies packaged in various libraries

```c++
#include <suil/server/endpoint.hpp>
#include <suil/base/logging.hpp>

#include <cstddef>

namespace hs = suil::http::server;

int main(int argc, char *argv[])
{
    // Log a message to console using system log tag SYSTEM
    sinfo("This is an example suil http application");
    
    // Server, optional middlewares can be provided
    using HttpServer = hs::Endpoint<>;
    // Create  socket configuration that binds to port 8000
    net::ServerConfig sock = {
        .socketConfig = net::TcpSocketConfig{
            .bindAddr = { .port = 8000 }
        }
    };
    // Create an http server instance
    HttpServer ep(opt(serverConfig, std::move(sock)));
    
    // attach a route the server
    Route(ep, "/hello/{string}")
    (GET_"method", OPTIONS_"method")        // Optional http method
    .attrs(opt(ReplyType, "text/plain"))    // Optional route attribute
    ([&](String name) {
        sdebug("Handling request {name = " PRIs "}", _PRIs(name));
        return suil::catstr("Hello '", name, "' from server");
    });
    
    return ep.start();
}
```

### Benchmarking (in progress)
This will be done right once the docs site is done. But for an overview,
testing on my laptop yields the following results
##### Environment
* Ubuntu 20.04.1 LTS (application running on docker container)
* Lenovo T460s Intel(R) Core(TM) i7-6600U CPU @ 2.60GHz
  - 2 cores, 2 threads
  - 12Gb RAM
* [wrk2](https://github.com/giltene/wrk2) master running on same machine

#### Application output
```shell
curl -i http://localhost:8000/hello ; echo
HTTP/1.1 200 OK
Connection: Keep-Alive
KeepAlive: 300000
Strict-Transport-Security: max-age 3600; includeSubdomains
Content-Type: application/json
Server: Suil-Http-Server
Date: Tue, 05 Jan 2021 07:17:10 GMT
Content-Length: 11

Hello World
```

#### Benchmark results
```bash
wrk -R1M  -c127 http://localhost:8000/hello
Running 10s test @ http://localhost:8000/hello
  2 threads and 127 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     4.27s     2.71s    9.47s    56.86%
    Req/Sec       -nan      -nan   0.00      0.00%
  444196 requests in 10.00s, 102.52MB read
Requests/sec:  44420.97
Transfer/sec:     10.25MB
```

### Supported Features
#### `suil/base` Library
This library provides utility functions and paves the road for developing other libraries. All the other librares
depend on this library.

#### `suil/network` Library
The networking library provides TCP socket API's, ZMQ API abstraction and a simple SMTP client and server development
framework.

#### `suil/database` Library
Provides database API's. Currently only redis and PostgreSQL supported. Plan to add support for mongo and SQLite.

#### `suil/rpc` Library
A framework for developing RPC API's. Supports JSON RPC and `suil` (or `wire`) RPC. RPC is generated from an `scc` file which is basically
a limited C++ header file with service defination.

#### `suil/http` Library
This library implements HTTP/1.1 protocol both server and client. See library README for more details

#### Build Requirements or Development Dependencies on Ubuntu
When building or developing an application that uses this project on ubuntu, the
following libraries should be installed.

* SQLite
  `sudo apt-get install sqlite3 libsqlite3-dev`
* PostgresSQL
  `sudo apt-get install libpq-dev postgresql postgresql-server-dev-all`
* UUID
  `sudo apt-get install uuid-dev`
* OpenSSL
  `sudo apt-get install openssl libssl-dev`
  
###### Credits 
* Integrates [libmill's](https://github.com/sustrik/libmill) coroutines which is used as the base library for
  asynchronous calls
* Integrates [IOD](https://github.com/matt-42/iod) framework which is a great tool when it comes to meta programming
* [Crow](https://github.com/ipkn/crow) for inspiration and HTTP routing and middleware code logic
* And all the other snippets adopted from various open source projects. A link to such is provided where the snippet
is used
