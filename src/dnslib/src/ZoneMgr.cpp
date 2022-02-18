
#include <filesystem>

#include <boost/algorithm/string.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include "vudnslib/DnsConfig.h"
#include "vudnslib/ZoneMgr.h"
#include "vudnslib/util.h"
#include "ZoneMgrMemory.h"
#include "ZoneMgrJson.h"

namespace vuberdns {

using namespace std;
namespace bs = boost::system;

std::tuple<Zone::ptr_t, bool> ZoneMgr::LookupName(const string_view &domain, bool throwOnNotFound )
{
    bool a = false;
    auto z = Lookup(toKey(domain), &a);

    auto dn = z->GetDomainName();

    if (z && !boost::iequals(domain, dn)) {
        if (throwOnNotFound) {
            throw bs::system_error{bs::errc::make_error_code(bs::errc::no_such_file_or_directory)};
        }
        return {};
    }

    return {z, a};
}

ZoneMgr::ptr_t ZoneMgr::Create(const DnsConfig& config) {

    filesystem::path path{config.data_path};

    if (path.extension() == ".json" || path.extension() == ".yaml") {
        return ZoneMgrJson::Create(config);
    }

    return ZoneMgrMemory::Create(config);
}

ZoneMgr::key_t ZoneMgr::toKey(const string_view &domain)
{
    return Split<boost::string_ref>(domain, '.');
}

} // namespace

