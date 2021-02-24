#pragma once

#include <optional>
#include <warlib/WarThreadpool.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/config.hpp>

namespace vuberdns::http {

/*! Basic embedded HTTP server.
 *
 *  This implementation is based on boost::beast
 */
class HttpServer
{
public:
    struct Config {
        struct Endpoint {
            struct Tls {
                std::string key;
                std::string cert;
            };

            std::string host = "[::]";
            std::string port; // If empty, 80 0r 443 is automatically assigned.
            std::optional<Tls> tls;
        };

        std::vector<Endpoint> endpoints;
    };

    HttpServer(war::Threadpool& ioThreadpool, const Config config);
    void Start();

private:
    void Listen();
    void StartSession(boost::beast::tcp_stream& stream,
                      boost::asio::yield_context yield);

    const Config config_;
    war::Threadpool& io_threadpool_;
};

} // ns
