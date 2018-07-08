#pragma once

#include <cstdint>

namespace vuberdns {

struct Statistics
{
    using cnt_t = std::uint64_t;

    Statistics() = default;
    Statistics(const Statistics& v) = default;
    Statistics& operator = (const Statistics& v) = default;

    cnt_t udp_connections {};
    cnt_t failed_udp_connections {};
    cnt_t ok_requests {};
    cnt_t failed_requests {};
    cnt_t bad_requests {};
    cnt_t timeout_dropped_connections {};
    cnt_t overflow_dropped_connections {};
    cnt_t bytes_received {};
    cnt_t bytes_sent {};
    cnt_t unknown_names {};

    Statistics& operator += (const Statistics& v) {
        udp_connections += v.udp_connections;
        failed_udp_connections += v.failed_udp_connections;
        ok_requests += v.ok_requests;
        failed_requests += v.failed_requests;
        bad_requests += v.bad_requests;
        timeout_dropped_connections += timeout_dropped_connections;
        overflow_dropped_connections += v.overflow_dropped_connections;
        bytes_received += v.bytes_received;
        bytes_sent += v.bytes_sent;
        unknown_names += v.unknown_names;

        return *this;
    }

    void Reset() {
        udp_connections = 0;
        failed_udp_connections = 0;
        ok_requests = 0;
        failed_requests = 0;
        bad_requests = 0;
        timeout_dropped_connections = 0;
        overflow_dropped_connections = 0;
        bytes_received = 0;
        bytes_sent = 0;
        unknown_names = 0;
    }
};


} // namespace
