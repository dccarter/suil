# suil
A lightweight mordern `C++` micro framework with lots of goodies packaged in various libraries

```c++
#include <suil/hpp/server/endpoint.hpp>
#include <suil/base/logging.hpp>

#include <cstddef>

namespace hs = suil::http::server;

int main(int argc, char *argv[])
{
    // Log a message to console using system log tag SYSTEM
    sinfo("This is an example suil http application");
    
    // Server with an optional middleware, as many as needed
    using HttpServer = hs::Endpoint<HelloMiddleware>;
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

###### Credits 
* Integrates [libmill's](https://github.com/sustrik/libmill) coroutines which is used as the base library for
  asynchronous calls
* Integrates [IOD](https://github.com/matt-42/iod) framework which is a great tool when it comes to meta programming
* [Crow](https://github.com/ipkn/crow) for inspiration and HTTP routing and middleware code logic
* And all the other snippets adopted from various open source projects. A link to such is provided where the snippet
is used

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
A framework for developing RPC API's. Supports JSON RPC and `suil` (or `wire`) RPC and uses C++ format to generate RPC code
```c++
// demo.hpp (Service implementation)
namespace demo {
    class Adder {
    public:
        int add(int a, int b) {
            return a + b;
        }
        
        void debug(const String& str) {
            sdebug("Adder::debug(...) remote debug " PRIs, _PRIs(str));
        }
    };
}


// demo.scc (Transpiled by SCC to a C++ and creates JSON RPC client/server pair)
#pragma native[cpp]
#include "demo.hpp"
#pragma endnative

#pragma load rpc

namespace demo {
    // generator a JSON RPC client/server
    class [[gen::rpc(json)]] Adder {
    public:
        int add(int a, int b);
        void debug(const suil::String& str);
    };
}

// client.cpp
#include "demo.hpp"
#include "demo.scc.hpp" // generate from service file demo.scc

int main(int argc, char *argv[])
{
    suil::net::TcpSocketConfig rpcConfig{
        .bindAddr = {
            .port = 5000
        }
    };
    
    // Create a client
    demo::AdderJrpcClient client(opt(tcpConfig, std::move(rpcConfig)));
    // Connect client to server
    client.connect();
    
    // Invoke remote methods
    client.debug("Hello RPC Server");
    auto result =  client.add(10, 100);
    std::cout << "remote 10 + 100 = " << result << std::endl;

    return EXIT_SUCCESS;
}

// Server.cpp
#include "demo.hpp"
#include "demo.scc.hpp" // generate from service file demo.scc

int main(int argc, char *argv[])
{
    suil::net::TcpSocketConfig rpcConfig{
        .bindAddr = {
            .port = 5000
        }
    };
    
    // Create a JSON RPC server context
    auto jctx = std::make_shared<demo::AdderJrpcContext>(new demo::Adder{});
    // Create a server
    suil::rpc::jrpc::Server server(jctx, opt(tcpConfig, jrpcConfig))
    
    return server.start();
}
```

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
