
#include <unistd.h>
#include <csignal>
#include <string>

#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <filesystem>
#include <boost/algorithm/string.hpp>

#include "warlib/WarLog.h"
#include "warlib/error_handling.h"
#include "warlib/boost_ptree_helper.h"

#include "vudnsd.h"
#include "vudnslib/DnsDaemon.h"
#include "vudnslib/DnsConfig.h"
#include "vudnslib/RestHandler.h"
#include "vudnslib/util.h"

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



using namespace std;
using namespace war;
using namespace vuberdns;
using namespace vudnsd;
using namespace std::string_literals;
using namespace std::placeholders;

#include "vudnsd.h"

bool ParseCommandLine(int argc, char *argv[], log::LogEngine& logger, Configuration& conf)
{
    namespace po = boost::program_options;

    po::options_description general("General Options");

    general.add_options()
        ("help,h", "Print help and exit")
        ("console-log,C", po::value<string>()->default_value("")->implicit_value("NOTICE"),
            "Log-level for the console-log")
        ("log-level,L", po::value<string>()->default_value("NOTICE"),
            "Log-level for the log-file")
        ("log-file", po::value<string>()->default_value("")->implicit_value("vudnsd.log"),
            "Name of the log-file")
        ("truncate-log", po::value<bool>()->default_value(true),
            "Truncate the log-file if it already exists")
        ("daemon,D", po::bool_switch(&conf.daemon), "Run as a system daemon")
        ("stats-update-seconds", po::value<int>(&conf.stats_update_seconds)->default_value(
            conf.stats_update_seconds), "Interval between statistics summaries in the log. 0 to disable.")
        ;

    po::options_description performance("Performance Options");
    performance.add_options()
        ("io-threads", po::value<int>(&conf.num_io_threads)->default_value(
            conf.num_io_threads),
            "Number of IO threads. If 0, a reasonable value will be used")
        ("io-queue-size", po::value<int>(&conf.max_io_thread_queue_capacity)->default_value(
            conf.max_io_thread_queue_capacity),
            "Capacity of the IO thread queues (max number of pending tasks per thread)")
        ;

    po::options_description conffiles("Files");
    conffiles.add_options()
        ("dns-zones,z",  po::value<string>(&conf.dns_zone_file)->default_value(
            conf.dns_zone_file),
            "Full path to the dns zone file")
        ("http-config,a",  po::value<string>(&conf.http_config)->default_value(
            conf.http_config),
            "Full path to the HTTP (api server) configuration")
        ;

    po::options_description hosts("DNS Hosts");
    hosts.add_options()
        ("dns-host,H", po::value<decltype(Configuration::hosts)>(&conf.hosts)->multitoken(),
            "Host to receive on (UDP) host-or-ip")
        ("dns-port", po::value<string>(&conf.port)->default_value(conf.port),
            "Port to receive on (UDP) name-or-digit")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(general).add(performance).add(conffiles).add(hosts);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << cmdline_options << endl
            << "Log-levels are:" << endl
            << "   FATAL ERROR WARNING INFO NOTICE DEBUG " << endl
            << "   TRACE1 TRACE2 TRACE3 TRACE4" << endl;

        return false;
    }

    if (!conf.daemon && !vm["console-log"].as<string>().empty()) {
        logger.AddHandler(make_shared<log::LogToStream>(cout, "console",
            log::LogEngine::GetLevelFromName(vm["console-log"].as<string>())));
    }

    if (!vm["log-file"].as<string>().empty()) {
        logger.AddHandler(make_shared<log::LogToFile>(
            vm["log-file"].as<string>(),
            vm["truncate-log"].as<bool>(),
            "file",
            log::LogEngine::GetLevelFromName(vm["log-level"].as<string>())));
    }

    return true;
}


int main(int argc, char *argv[]) {
    try {
        locale loc("");
    } catch (const std::exception& e) {
        std::cout << "Locales in Linux are fundamentally broken. Never worked. Never will. Overriding the current mess with LC_ALL=C" << endl;
        setenv("LC_ALL", "C", 1);
    }

    /* Enable logging.
     */
    log::LogEngine logger;
    Configuration configuration;

    if (!ParseCommandLine(argc, argv, logger, configuration))
        return -1;

    LOG_INFO << "vUberdns " << APP_VERSION << " starting up";

    if (configuration.daemon) {
        LOG_INFO << "Switching to system daemon mode";
        daemon(1, 0);
    }

    try {

        Threadpool thread_pool(configuration.num_io_threads,
                               configuration.max_io_thread_queue_capacity);

        //StatisticsManager::ptr_t stats_manager;

        DnsConfig dns_config;
        dns_config.data_path = configuration.dns_zone_file;
        const auto dns_server = DnsDaemon::Create(thread_pool, dns_config);

//         stats_manager = StatisticsManager::Create(thread_pool,
//                                                   configuration.stats_update_seconds);

        if (configuration.hosts.empty()) {
            LOG_INFO << "No host-names specified. Will listen to all interfaces.";
            configuration.hosts.emplace_back("0.0.0.0");
            configuration.hosts.emplace_back("[::]");
        }

        for(const auto& host : configuration.hosts) {

            string hostname {host};
            string port = configuration.port;

            auto brpos = hostname.find(']');

            if (brpos == std::string::npos) {
                brpos = 0;
            }

            auto pos = hostname.find(':', brpos);
            if (pos != std::string::npos) {
                port = hostname.substr(pos + 1);
                hostname.resize(pos);
            }

            // Strip off ipv6 brackets
            if (hostname.size() > 2) {
                if (hostname.front() == '[') {
                    hostname = hostname.substr(1, hostname.size() -1);
                }
                if (hostname.back() == ']') {
                    hostname.resize(hostname.size() -1);
                }
            }

            if (hostname.empty() || port.empty()) {
                LOG_WARN << "Empty argument to --host or --dns-port argument!";
            } else {
                dns_server->StartReceivingUdpAt(hostname, port);
            }
        }

        RestHandler rest{dns_server->GetZoneMgr()};

        std::unique_ptr<http::HttpServer> http;
        if (!configuration.http_config.empty()) {
            http::HttpServer::Config cfg;
            fileToObject(cfg, configuration.http_config);
            http = make_unique<http::HttpServer>(thread_pool, cfg,
                                                 [&rest](const http::HttpServer::Request& req) -> http::HttpServer::Reply {
                try {
                    auto res = rest.Process(req);
                    return res;
                } catch(const exception& ex) {
                    LOG_ERROR << "Unexpected exception from request processing: " << ex.what();
                }

                return {500, true, "internal error - Unexpected exception from request processing"};
            }
            //                                     bind(&RestHandler::Process, &rest, _1)
            );
            http->Start();
        }

        /* We now put the main-tread to sleep.
         *
         * It will remain sleeping until we receive one of the signals
         * below. Then we simply shut down the the thread-pool and wait for the
         * threads in the thread-pool to finish.
         */
        io_context_t main_thread_service;
        boost::asio::signal_set signals(main_thread_service, SIGINT, SIGTERM,
            SIGQUIT);
        signals.async_wait([&thread_pool](boost::system::error_code /*ec*/,
                                          int signo) {
            LOG_INFO << "Received signal " << signo << ". Shutting down.";
            thread_pool.Close();
        });
        main_thread_service.run();

        // Wait for the show to end
        thread_pool.WaitUntilClosed();

    } catch(const war::ExceptionBase& ex) {
        LOG_ERROR_FN << "Caught exception: " << ex;
        return -1;
    } catch(const boost::exception& ex) {
        LOG_ERROR_FN << "Caught boost exception: " << ex;
        return -1;
    } catch(const std::exception& ex) {
        LOG_ERROR_FN << "Caught standard exception: " << ex;
        return -1;
    } catch(...) {
        LOG_ERROR_FN << "Caught UNKNOWN exception!";
        return -1;
    };

    LOG_INFO << "So Long, and Thanks for All the Fish!";

    return 0;


}
