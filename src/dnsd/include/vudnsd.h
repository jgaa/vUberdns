#pragma once

#include <vector>
#include <string>
#include <utility>
#include "vudnslib/HttpServer.h"

namespace vudnsd {

struct Configuration
{
    int num_io_threads {2};
    int max_io_thread_queue_capacity {1024 * 64};
    std::string dns_zone_file {"/var/lib/vudns/zones.conf"};
    bool daemon  {false};
    int stats_update_seconds {60};
    std::vector<std::string> hosts;

    vuberdns::http::HttpServer::Config api = {
        {{"127.0.0.1"}}
    };
};

#ifndef APP_VERSION
#   define APP_VERSION VUDNS_VERSION
#endif


} // namespace
