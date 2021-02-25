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
    struct Request {
        std::string path;
        std::string verb;
        std::string body;
    };

    struct Reply {
        uint16_t httpCode = 200;
        bool close = false;
        std::string body;
    };

    using handle_fn_t = std::function<Reply(const Request& req)>;

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

    HttpServer(war::Threadpool& ioThreadpool, const Config config, handle_fn_t handler);
    void Start();

private:
    void Listen();
    void StartSession(boost::beast::tcp_stream& stream,
                      boost::asio::yield_context yield);

    const Config config_;
    war::Threadpool& io_threadpool_;
    const handle_fn_t handler_;
};

} // ns
