#pragma once

#include <vector>
#include <string>
#include <utility>

namespace vudnsd {

struct Configuration
{
    int num_io_threads {2};
    int max_io_thread_queue_capacity {1024 * 64};
    std::string dns_zone_file {"/var/lib/vudns/zones.conf"};
    bool daemon  {false};
    int stats_update_seconds {60};
    std::vector<std::string> hosts;
};

#ifndef APP_VERSION
#   define APP_VERSION "undefined"
#endif


} // namespace
