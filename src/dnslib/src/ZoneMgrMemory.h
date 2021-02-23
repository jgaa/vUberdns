#pragma once

#include <boost/optional.hpp>
#include <filesystem>
#include <boost/property_tree/ptree_fwd.hpp>

#include "vudnslib/ZoneMgr.h"
#include "ZoneMemoryImpl.h"

namespace vuberdns {

class ZoneMgrMemory;


class ZoneMgrMemory : public ZoneMgr {
public:
    using ptr_t = std::shared_ptr<ZoneMgrMemory>;
    using key_t = const std::vector<boost::string_ref>;

    Zone::ptr_t Lookup(const key_t& key, bool *authorative) const override;

    void Load(const std::filesystem::path& path);

    ZoneMemoryImpl::zones_t AddZones(const boost::property_tree::ptree& pt,
                                     ZoneMemoryImpl::ptr_t parent = {});

    // Instantiates the manager and loads data
    static ptr_t Create(const DnsConfig& config);

private:
    using it_t = key_t::const_reverse_iterator;

    ZoneMemoryImpl::ptr_t Lookup(it_t label, it_t end,
                                 const ZoneMemoryImpl::zones_t& zones,
                                 bool *authorative) const;

    ZoneMemoryImpl::zones_t zones_;
};

} // namespace

