#include <boost/property_tree/info_parser.hpp>
#include <boost/algorithm/string.hpp>

#include "warlib/WarLog.h"
#include "ZoneMgrMemory.h"

using namespace war;
using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;


namespace vuberdns {

std::string ZoneMemoryImpl::GetDomainName() const {
    std::string name = label();
    for(auto z = this->parent(); z; z = z->parent()) {
        name += '.';
        name += z->label();
    }

    return name;
}

bool ZoneMemoryImpl::HaveCname() const noexcept {
    return !cname_.empty();
}

ZoneMemoryImpl::ptr_t
ZoneMemoryImpl::Create(const std::string& name,
                       const boost::property_tree::ptree& pt,
                       ptr_t parent, ZoneMgrMemory& mgr) {

    auto zone = make_shared<ZoneMemoryImpl>();
    zone->parent_ = parent;
    zone->label_ = name;
    zone->authrorative_ = pt.get<bool>("authorative", false);

    if (auto soa = pt.get_child_optional("soa")) {
        zone->soa_ = soa_t::value_type {};
        zone->soa_->rname = soa->get<string>("rname", {});
        zone->soa_->serial = soa->get<uint32_t>("serial", zone->soa_->serial);
        zone->soa_->refresh = soa->get<uint32_t>("refresh", zone->soa_->refresh);
        zone->soa_->retry = soa->get<uint32_t>("retry", zone->soa_->retry);
        zone->soa_->expire = soa->get<uint32_t>("expire", zone->soa_->expire);
        zone->soa_->minimum = soa->get<uint32_t>("minimum", zone->soa_->minimum);

        LOG_INFO << "Loading zone: " << zone->GetDomainName();
    }

    if (auto ns = pt.get_child_optional("ns")) {
        zone->ns_ = ns_list_t::value_type {};
        for(auto it = ns->begin() ; it != ns->end(); ++it) {
            zone->ns_->emplace_back(Zone::ns_t{it->first, zone});
        }

        zone->soa_->mname = &zone->ns_->front();
    }

    if (auto mx = pt.get_child_optional("mx")) {
        zone->mx_ = mx_list_t::value_type {};
        for(auto it = mx->begin() ; it != mx->end(); ++it) {
            zone->mx_->emplace_back(Zone::mx_t{it->first,
                it->second.get_value<uint16_t>(), zone});
        }
    }

    if (auto cname = pt.get_optional<string>("cname")) {
        zone->cname_ = *cname;
    }

    if (auto a = pt.get_child_optional("a")) {
        zone->a_ = a_list_t::value_type{};
        for(auto it = a->begin() ; it != a->end(); ++it) {
            zone->a_->push_back(boost::asio::ip::address_v4::from_string(it->first));
        }
    }

    zone->children_ = mgr.AddZones(pt, zone);

    return zone;
}

std::ostream& ZoneMemoryImpl::Print(std::ostream& out, int level) const  {
    out << std::setw(level * 2) << '{' << std::setw(0) << '\"' << label()
        << '\"'<< std::endl;
    if (auto c = children()) {
        for(const auto zone : *c) {
            zone->Print(out, level + 1);
        }
    }
    out << std::setw(level * 2) << '}' << std::setw(0) << std::endl;
    return out;
}

}
