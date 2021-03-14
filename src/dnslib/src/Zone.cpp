
#include "vudnslib/Zone.h"

#include "warlib/WarLog.h"
#include "vudnslib/ZoneMgr.h"
#include "vudnslib/util.h"

namespace vuberdns {
const std::string Zone::Soa::hostmaster_ = { "hostnamster" };

bool Zone::Validate(const std::string_view &domain, const Zone &zone)
{
    if (!zone.cname().empty()) {
        if ((zone.a() && !zone.a()->empty()) || (zone.aaaa() && !zone.aaaa()->empty())) {
            LOG_WARN << "Validate: Zone " << domain << " Must use either cname or a/aaaa";
            return false; // Must be either cname or IP
        }
    }

    const auto k = Split(domain, '.');
    if (!k.empty()) {
        if (k.front() != zone.label()) {
            LOG_WARN << "Validate: Zone " << domain << " Expected label to be " << k.front();
            return false;
        }
    }

    return true;
}

}
