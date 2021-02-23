
#include <filesystem>

#include <boost/fusion/adapted.hpp>

#include "ZoneMgrJson.h"
#include "restc-cpp/SerializeJson.h"
#include "vudnslib/DnsConfig.h"
#include "warlib/WarLog.h"

BOOST_FUSION_ADAPT_STRUCT(vuberdns::Zone::Ns,
    (std::string, fqdn)
);

BOOST_FUSION_ADAPT_STRUCT(vuberdns::Zone::Mx,
    (std::string, fqdn)
    (uint16_t, priority)
);

BOOST_FUSION_ADAPT_STRUCT(vuberdns::ZoneMgrJson::ZoneData,
    (std::string, label)
    (vuberdns::Zone::soa_t, soa)
    (bool, authrorative)
    (std::string, txt)
    (vuberdns::Zone::a_list_t, a)
    (vuberdns::Zone::aaaa_list_t, aaaa)
    (std::string, cname)
    (vuberdns::Zone::ns_list_t, ns)
    (vuberdns::Zone::mx_list_t, mx)
);

BOOST_FUSION_ADAPT_STRUCT(vuberdns::ZoneMgrJson::ZoneNode,
    (vuberdns::ZoneMgrJson::ZoneData, zone)
    (std::optional<std::vector<vuberdns::ZoneMgrJson::ZoneNode>>, children)
);

namespace vuberdns {

using namespace std;

string ZoneMgrJson::ZoneRef::GetDomainName() const
{
    auto name = label();
    for(auto z = &node_; z; z = z->parent) {
        name += '.';
        name += z->zone.label;
    }

    return name;
}

ostream &ZoneMgrJson::ZoneRef::Print(ostream &out, int level) const
{
    out << std::setw(level * 2) << '{' << std::setw(0) << '\"' << label()
        << '\"' << std::endl;
    if (auto c = node_.children) {
        for(auto& n : *c) {
            ZoneRef zone{n};
            zone.Print(out, level + 1);
        }
    }
    out << std::setw(level * 2) << '}' << std::setw(0) << std::endl;
    return out;
}

Zone::ptr_t ZoneMgrJson::Lookup(const ZoneMgrJson::key_t &key, bool *authorative) const
{

}

void ZoneMgrJson::Load(const std::filesystem::path &path)
{
    auto zones = make_shared<std::vector<ZoneNode>>();
    std::ifstream ifs{path.string()};
    restc_cpp::SerializeFromJson(*zones, ifs);
    zones_ = move(zones);
}

void ZoneMgrJson::Save(const std::filesystem::path &path)
{
    if (!zones_) {
        return;
    }
    std::ofstream out{path.string()};
    restc_cpp::SerializeToJson(*zones_, out);
}

ZoneMgrJson::ptr_t ZoneMgrJson::Create(const DnsConfig &config)
{
    auto mgr = make_shared<ZoneMgrJson>();

    filesystem::path path = config.data_path;

    if (std::filesystem::is_regular_file(path)) {
        mgr->Load(path);
    } else {
        LOG_WARN_FN << "The dns zones file "
            << war::log::Esc(config.data_path)
            << " does not exist!. Cannot load zones";
    }

    return mgr;
}

}
