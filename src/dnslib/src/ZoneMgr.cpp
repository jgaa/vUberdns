
#include "vudnslib/DnsConfig.h"
#include "vudnslib/ZoneMgr.h"
#include "ZoneMgrMemory.h"

namespace vuberdns {

ZoneMgr::ptr_t ZoneMgr::Create(const DnsConfig& config) {
    return ZoneMgrMemory::Create(config);
}

} // namespace

