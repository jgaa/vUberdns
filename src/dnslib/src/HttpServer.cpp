
#include "warlib/WarLog.h"
#include "vudnslib/HttpServer.h"
#include "boost/beast/http/string_body.hpp"

namespace vuberdns::http {

using namespace std;
using namespace war;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using namespace std::placeholders;
using namespace std::string_literals;

HttpServer::HttpServer(war::Threadpool &ioThreadpool,
                       const HttpServer::Config config,
                       handle_fn_t handler)
    : config_{move(config)}, io_threadpool_{ioThreadpool}, handler_{move(handler)}
{
    assert(handler_);
}

void HttpServer::Start()
{
    Listen();
}

void HttpServer::Listen()
{
    auto& pipeline = io_threadpool_.GetPipeline(0);
    auto& ios = pipeline.GetIoService();

    for(const auto& cep : config_.endpoints) {
        LOG_DEBUG_FN << "Preparing to listen to: " <<
        log::Esc(cep.host) << " on HTTP port " << log::Esc(cep.port);

        tcp::resolver resolver(ios);

        auto port = cep.port;
        if (port.empty()) {
            if (cep.tls)
                port = "https";
            else
                port = "http";
        }

        auto endpoint = resolver.resolve({cep.host, port});
        tcp::resolver::iterator end;
        for(; endpoint != end; ++endpoint) {
            tcp::endpoint ep = endpoint->endpoint();
            LOG_NOTICE << "Adding HTTP endpoint: " << ep;

            auto& http_pipeline = io_threadpool_.GetAnyPipeline();

            boost::asio::spawn(http_pipeline.GetIoService(),
                               [this, ep, &ios]
                               (boost::asio::yield_context yield) {
                beast::error_code ec;

                tcp::acceptor acceptor{ios};
                acceptor.open(ep.protocol(), ec);
                if(ec) {
                    LOG_ERROR << "Failed to open endpoint " << ep << ": " << ec;
                    return;
                }

                acceptor.set_option(net::socket_base::reuse_address(true), ec);
                if(ec) {
                    LOG_ERROR << "Failed to set option reuse_address on " << ep << ": " << ec;
                    return;
                }

                // Bind to the server address
                acceptor.bind(ep, ec);
                if(ec) {
                    LOG_ERROR << "Failed to bind to " << ep << ": " << ec;
                    return;
                }

                // Start listening for connections
                acceptor.listen(net::socket_base::max_listen_connections, ec);
                if(ec) {
                    LOG_ERROR << "Failed to listen to on " << ep << ": " << ec;
                    return;
                }

                for(;!ios.stopped();)
                {
                    tcp::socket socket{ios};
                    acceptor.async_accept(socket, yield[ec]);
                    if(ec) {
                        LOG_WARN << "Failed to accept on " << ep << ": " << ec;
                        return;
                    }

                    boost::asio::spawn(
                        acceptor.get_executor(),
                        std::bind(
                            &HttpServer::StartSession,
                            this,
                            beast::tcp_stream(std::move(socket)), _1));
                }

            });
        }; // for resolver endpoint
    } // for config_.endpoints
}

void HttpServer::StartSession(beast::tcp_stream &stream, boost::asio::yield_context yield)
{
    bool close = false;
    beast::error_code ec;
    beast::flat_buffer buffer{1024 * 64};

    while(!close) {

        stream.expires_after(std::chrono::seconds(30));

        // Read a request
        http::request<http::string_body> req;
        http::async_read(stream, buffer, req, yield[ec]);
        if(ec == http::error::end_of_stream)
            break;
        if(ec) {
            LOG_ERROR << "read failed: " << ec;
            return;
        }

        if (!req.keep_alive()) {
            close = true;
        }

        // TODO: Add authentication check

        if (!req.body().empty()) {
            const auto ct =req.at(http::field::content_type);
            // TODO: Check that the type is json
            LOG_TRACE1 << "Request has content type: " << ct;
        }

        // TODO: Check that the client accepts our json reply
        Request request {
                req.base().target().to_string(),
                req.base().method_string().to_string(),
                req.body()
        };

        const auto reply = handler_(request);
        if (reply.close) {
            close = true;
        }

        // Send the response
        //handle_request(*doc_root, std::move(req), lambda);
        http::response<http::string_body> res;
        res.body() = reply.body;
        res.result(reply.httpCode);
        res.base().set(http::field::server, "vUberdns "s + VUDNS_VERSION);
        res.base().set(http::field::content_type, "application/json; charset=utf-8");
        res.base().set(http::field::connection, close ? "close" : "keep-alive");
        res.prepare_payload();

        http::async_write(stream, res, yield[ec]);
        if(ec) {
            LOG_ERROR << "write failed: " << ec;
            return;
        }
    }

    // Send a TCP shutdown
    stream.socket().shutdown(tcp::socket::shutdown_send, ec);
}

} // ns
