
#include <boost/property_tree/info_parser.hpp>
#include <boost/algorithm/string.hpp>

#include "warlib/WarLog.h"
#include "ZoneMgrMemory.h"

#include "vudnslib/DnsConfig.h"

using namespace war;
using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;


namespace vuberdns {

Zone::ptr_t ZoneMgrMemory::Lookup(const key_t& key, bool *authorative) const {
     return Lookup(key.rbegin(), key.rend(), zones_, authorative);
}

ZoneMemoryImpl::ptr_t
ZoneMgrMemory::Lookup(it_t label, it_t end,
                      const ZoneMemoryImpl::zones_t& zones,
                      bool *authorative) const {

    if(label == end) {
        LOG_TRACE4_FN << "Returning null. Called with end";
        return nullptr;
    }

    LOG_TRACE4_FN << "Searching for " << war::log::Esc(*label);

    ZoneMemoryImpl::ptr_t wildchard;
    ZoneMemoryImpl::ptr_t match;

    for(const auto & zone : zones) {
        if (zone->label().empty()) {
            wildchard = zone;
            LOG_TRACE4_FN << "Matched wildchard zone" << war::log::Esc(wildchard->label());
        }
        if (boost::iequals(*label, zone->label())) {
            match = zone;
            LOG_TRACE4_FN << "Matched [full] zone" << war::log::Esc(match->label());
            break;
        }
    }

    auto rval = match ? move(match) : move(wildchard);

    if (authorative && rval) {
        *authorative = rval->authorative();
    }

    // If we found something, and have more labels to search, recurse further
    if (rval && (++label != end)) {
        if (auto children = rval->children()) {
            return Lookup(label, end, *children, authorative);
        }
    }

    LOG_TRACE4_FN << "Returning " << (rval ? war::log::Esc(rval->label()) : std::string("[null]"));
    return rval; // Return whatever we found, if anything
}

void ZoneMgrMemory::Load(const std::filesystem::path& path) {
    boost::property_tree::ptree dns_pt;
    boost::property_tree::read_info(path.string(), dns_pt);
    zones_ = AddZones(dns_pt);
}

ZoneMemoryImpl::zones_t
ZoneMgrMemory::AddZones(const boost::property_tree::ptree& pt,
                        ZoneMemoryImpl::ptr_t parent) {
    ZoneMemoryImpl::zones_t zones;

    for(auto it = pt.begin(); it != pt.end(); ++it) {
        std::string name = it->first;
        if (!name.empty() && (name[0] == '@')) {
            name.erase(name.begin());
            // We have a zone.
            zones.push_back(ZoneMemoryImpl::Create(name, it->second, parent,
                                                   *this));
        }
    }

    return zones;
}

ZoneMgrMemory::ptr_t ZoneMgrMemory::Create(const DnsConfig& config) {
    auto mgr = make_shared<ZoneMgrMemory>();

    std::filesystem::path path = config.data_path;

    if (std::filesystem::is_regular_file(path)) {
        mgr->Load(path);
    } else {
        LOG_WARN_FN << "The dns zones file "
            << war::log::Esc(config.data_path)
            << " does not exist!. Cannot load zones";
    }

    return mgr;
}


} // namespace
