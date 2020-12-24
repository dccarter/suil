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
