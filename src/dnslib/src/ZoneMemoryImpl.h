#pragma once

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

#include "vudnslib/ZoneMgr.h"

namespace vuberdns {

class ZoneMgrMemory;

class ZoneMemoryImpl : public Zone {
public:
    using ptr_t = std::shared_ptr<ZoneMemoryImpl>;
    using zones_t = std::vector<ZoneMemoryImpl::ptr_t>;
    using zones_list_t = boost::optional<zones_t>;

    ZoneMemoryImpl() = default;

    std::string label() const override { return label_; };
    a_list_t a() const override { return a_; }
    ns_list_t ns() const override { return ns_; }
    mx_list_t mx() const override { return mx_; }
    std::string cname() const override { return cname_; }
    soa_t soa() const override { return soa_; }
    bool authorative() const override { return authrorative_; }
    Zone::ptr_t parent() const override { return parent_; }

    std::string GetDomainName() const override;

    bool HaveCname() const noexcept override;

    zones_list_t children() const  { return children_; }

    std::ostream& Print(std::ostream& out, int level = 0) const override;

    static ptr_t Create(const std::string& name,
                        const boost::property_tree::ptree& pt,
                        ptr_t parent,
                        ZoneMgrMemory& mgr);
private:
    ptr_t parent_;
    std::string label_;
    soa_t soa_;
    bool authrorative_;
    std::string fdqn_;
    a_list_t a_;
    std::string cname_;
    ns_list_t ns_;
    mx_list_t mx_;
    zones_t children_;
};

} // namespace

