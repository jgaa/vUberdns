
#include "warlib/WarLog.h"
#include "vudnslib/HttpServer.h"

namespace vuberdns::http {

using namespace std;
using namespace war;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>

HttpServer::HttpServer(war::Threadpool &ioThreadpool, const HttpServer::Config config)
    : config_{move(config)}, io_threadpool_{ioThreadpool}
{

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
                               [ep,&http_pipeline,&ios,this]
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
                            &HttpServer::DoSession,
                            beast::tcp_stream(std::move(socket)),
                            doc_root,
                            std::placeholders::_1));
                }

            });
        }; // for resolver endpoint
    } // for config_.endpoints



}

} // ns
