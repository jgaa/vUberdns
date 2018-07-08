#pragma once

#include <memory>

#include <warlib/asio.h>
#include <warlib/WarThreadpool.h>

namespace vuberdns {

class DnsConfig;

class DnsDaemon
{
public:
    using ptr_t = std::shared_ptr<DnsDaemon>;

    DnsDaemon() = default;
    virtual ~DnsDaemon() = default;
    DnsDaemon(const DnsDaemon&) = delete;
    DnsDaemon& operator = (const DnsDaemon&) = delete;

    /*! Start to process incoming UDP queries at this address */
    virtual void StartReceivingUdpAt(std::string host, std::string port) = 0;

    /*! Fabric */
    static ptr_t Create(war::Threadpool& ioThreadpool,
                        const DnsConfig& configuration);
};

} // namespace
