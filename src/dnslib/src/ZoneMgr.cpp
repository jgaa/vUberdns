
#include <filesystem>

#include <boost/algorithm/string.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include "vudnslib/DnsConfig.h"
#include "vudnslib/ZoneMgr.h"
#include "ZoneMgrMemory.h"
#include "ZoneMgrJson.h"

namespace vuberdns {

using namespace std;
namespace bs = boost::system;

std::tuple<Zone::ptr_t, bool> ZoneMgr::LookupName(const string &domain)
{
    bool a = false;
    auto z = Lookup(toKey(domain), &a);

    if (z && !boost::iequals(domain, z->GetDomainName())) {
        throw bs::system_error{bs::errc::make_error_code(bs::errc::no_such_file_or_directory)};
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

ZoneMgr::key_t ZoneMgr::toKey(const string &domain)
{
    key_t key;

    std::string_view w = domain;
    do {
        string_view k;
        if (const auto pos = w.find('.') ; pos != string::npos) {
            k = w.substr(0, pos -1);
            w = w.substr(pos +1);
        } else {
            k = w;
            w = {};
        }

        if (!k.empty()) {
            key.push_back(boost::string_ref{k.data(), k.size()});
        }
    } while (false);

    return key;
}

} // namespace

