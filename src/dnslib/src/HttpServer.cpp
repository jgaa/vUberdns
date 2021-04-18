
#include <boost/beast/ssl.hpp>

#include "warlib/WarLog.h"
#include "vudnslib/HttpServer.h"
#include "vudnslib/util.h"
#include "boost/beast/http/string_body.hpp"

namespace vuberdns::http {

using namespace std;
using namespace war;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace ssl = boost::asio::ssl;
namespace net = boost::asio;            // from <boost/asio.hpp>
using namespace std::placeholders;
using namespace std::string_literals;


namespace {

struct LogRequest {
    boost::asio::ip::tcp::endpoint local, remote;
    beast::string_view type;
    beast::string_view location;
    string_view user;
    int replyValue = 0;
    beast::string_view replyText;
    bool done = false;

    void set(const http::response<http::string_body>& res) {
        replyValue = res.result_int();
        replyText = res.reason();
        flush();
    }

    void flush() {
        if (!done) {
            LOG_INFO << remote << " --> " << local << " [" << user << "] " << type << ' ' << log::Esc(location.data()) << ' ' << replyValue << ' ' << replyText;
        }
        done = true;
    }

    ~LogRequest() {
        flush();
    }
};

template <typename streamT, bool isTls>
void DoSession_(streamT& stream,
               const HttpServer::handle_fn_t& handler,
               HttpServer& instance,
               boost::asio::yield_context& yield)
{
    assert(handler);

    LOG_TRACE1 << "Processing session: " << beast::get_lowest_layer(stream).socket().remote_endpoint()
               << " --> " << beast::get_lowest_layer(stream).socket().local_endpoint();

    bool close = false;
    beast::error_code ec;
    beast::flat_buffer buffer{1024 * 64};

    if constexpr(isTls) {
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(5));
        stream.async_handshake(ssl::stream_base::server, yield[ec]);
        if(ec) {
            LOG_ERROR << "TLS handshake failed: " << ec.message();
            return;
        }
    }

    while(!close) {
        LogRequest lr;
        lr.remote =  beast::get_lowest_layer(stream).socket().remote_endpoint();
        lr.local = beast::get_lowest_layer(stream).socket().local_endpoint();

        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        http::request<http::string_body> req;
        http::async_read(stream, buffer, req, yield[ec]);
        if(ec == http::error::end_of_stream)
            break;
        if(ec) {
            LOG_ERROR << "read failed: " << ec.message();
            return;
        }

        if (!req.keep_alive()) {
            close = true;
        }

        lr.location = req.base().target();
        lr.type = req.base().method_string();

        bool authorized = false;
        if (auto it = req.base().find(http::field::authorization) ; it != req.base().end()) {
            auto [a, u] = instance.Authenticate({it->value().data(), it->value().size()});
            lr.user = u;
            authorized = a;
        }

        if (!authorized) {
            LOG_TRACE1_FN << "Request was unauthorized!";

            http::response<http::string_body> res;
            res.body() = "Access denied";
            res.result(403);
            res.base().set(http::field::server, "vUberdns "s + VUDNS_VERSION);
            res.base().set(http::field::content_type, "application/json; charset=utf-8");
            res.base().set(http::field::connection, close ? "close" : "keep-alive");
            res.prepare_payload();
            lr.set(res);

            http::async_write(stream, res, yield[ec]);
            if(ec) {
                LOG_ERROR << "write failed: " << ec.message();
                break;
            }
        }

        if (!req.body().empty()) {
            if (auto it = req.base().find(http::field::content_type) ; it != req.base().end()) {
                // TODO: Check that the type is json
                LOG_TRACE1 << "Request has content type: " << it->value();
            }
        }

        // TODO: Check that the client accepts our json reply
        HttpServer::Request request {
                req.base().target().to_string(),
                req.base().method_string().to_string(),
                req.body()
        };

        const auto reply = handler(request);
        if (reply.close) {
            close = true;
        }

        http::response<http::string_body> res;
        res.body() = reply.body;
        res.result(reply.httpCode);
        res.base().set(http::field::server, "vUberdns "s + VUDNS_VERSION);
        res.base().set(http::field::content_type, "application/json; charset=utf-8");
        res.base().set(http::field::connection, close ? "close" : "keep-alive");
        res.prepare_payload();

        lr.set(res);
        http::async_write(stream, res, yield[ec]);
        if(ec) {
            LOG_WARN << "write failed: " << ec.message();
            return;
        }
    }

    if constexpr(isTls) {
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

        // Perform the SSL shutdown
        stream.async_shutdown(yield[ec]);
        if (ec) {
            LOG_TRACE1 << "TLS shutdown failed: " << ec.message();
        }
    } else {
        // Send a TCP shutdown
        stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    }
}

template <typename streamT, bool isTls>
void DoSession(streamT& stream,
               const HttpServer::handle_fn_t& handler,
               HttpServer& instance,
               shared_ptr<ssl::context> sslctx,
               boost::asio::yield_context yield) {

    try {
        DoSession_<streamT, isTls>(stream, handler, instance, yield);
    } WAR_CATCH_ALL_E;
}

} // ns

HttpServer::HttpServer(war::Threadpool &ioThreadpool,
                       const HttpServer::Config config,
                       const handle_fn_t& handler)
    : config_{move(config)}, io_threadpool_{ioThreadpool}, handler_{move(handler)}
{
    assert(handler_);
}

void HttpServer::Start()
{
    Listen();
}

std::pair<bool, std::string_view /* user name */> HttpServer::Authenticate(const string_view &authHeader)
{
    auto pos = authHeader.find(' ');
    if (pos == string_view::npos) {
        return {false, {}};
    }
    auto token = authHeader.substr(pos +1);

    for(const auto& user : config_.users) {
        auto concat = user.name + ":" + user.passwd;
        auto auth = Base64Encode(concat);

        if (token == auth) {
            return {true, user.name};
        }
    }

    return {false, {}};
}

void HttpServer::Listen()
{
    auto& pipeline = io_threadpool_.GetPipeline(0);
    auto& ios = pipeline.GetIoService();

    for(const auto& cep : config_.endpoints) {
        tcp::resolver resolver(ios);

        auto port = cep.port;
        if (port.empty()) {
            if (cep.tls)
                port = "https";
            else
                port = "http";
        }

        LOG_DEBUG_FN << "Preparing to listen to: " <<
            log::Esc(cep.host) << " on " << (cep.tls ? "HTTPS" : "HTTP") << " port " << log::Esc(port);

        auto endpoint = resolver.resolve({cep.host, port});
        tcp::resolver::iterator end;
        for(; endpoint != end; ++endpoint) {
            tcp::endpoint ep = endpoint->endpoint();
            LOG_NOTICE << "Adding " << (cep.tls ? "HTTPS" : "HTTP") << " endpoint: " << ep;

            auto& http_pipeline = io_threadpool_.GetAnyPipeline();

            boost::asio::spawn(http_pipeline.GetIoService(),
                               [this, ep, &ios, cep]
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

                    if (cep.tls) {
                        auto ctx = make_shared<ssl::context>(ssl::context::tls_server);
                        ctx->use_certificate_chain_file(cep.tls->cert);
                        ctx->use_private_key_file(cep.tls->key, ssl::context::pem);

                        boost::asio::spawn(
                                        acceptor.get_executor(),
                                        std::bind(
                                            &DoSession<beast::ssl_stream<beast::tcp_stream>, true>,
                                            beast::ssl_stream<beast::tcp_stream>(std::move(socket), *ctx),
                                            handler_, *this, ctx, _1));

                    } else {
                        boost::asio::spawn(
                                        acceptor.get_executor(),
                                        std::bind(
                                            &DoSession<beast::tcp_stream, false>,
                                            beast::tcp_stream(std::move(socket)),
                                            handler_, *this, nullptr, _1));
                    }
                }

            });
        }; // for resolver endpoint
    } // for config_.endpoints
}

} // ns
