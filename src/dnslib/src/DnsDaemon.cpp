

#include "vudnslib/ZoneMgr.h"
#include "vudnslib/DnsConfig.h"

#include "DnsDaemonImpl.h"

using namespace war;
using namespace std;

namespace vuberdns {

DnsDaemon::ptr_t DnsDaemon::Create(Threadpool& ioThreadpool,
                                   const DnsConfig& configuration) {

    return make_shared<DnsDaemonImpl>(ioThreadpool,
        ZoneMgr::Create(configuration)
    );
}


} // namespace
