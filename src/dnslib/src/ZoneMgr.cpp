
#include <filesystem>
#include "vudnslib/DnsConfig.h"
#include "vudnslib/ZoneMgr.h"
#include "ZoneMgrMemory.h"
#include "ZoneMgrJson.h"

namespace vuberdns {

using namespace std;

ZoneMgr::ptr_t ZoneMgr::Create(const DnsConfig& config) {

    filesystem::path path{config.data_path};

    if (path.extension() == ".json") {
        return ZoneMgrJson::Create(config);
    }

    return ZoneMgrMemory::Create(config);
}

} // namespace

