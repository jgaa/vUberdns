#pragma once

#include <optional>
#include <memory>

#include <filesystem>
#include <boost/property_tree/ptree_fwd.hpp>

#include "vudnslib/ZoneMgr.h"

namespace vuberdns {

class ZoneMgrJson : public ZoneMgr {
public:
    using ptr_t = std::shared_ptr<ZoneMgrJson>;
    using key_t = const std::vector<boost::string_ref>;

    struct ZoneData {
        std::string label;
        Zone::soa_t soa;
        bool authrorative = false;;
        std::string txt;
        Zone::a_list_t a;
        Zone::aaaa_list_t aaaa;
        std::string cname;
        Zone::ns_list_t ns;
        Zone::mx_list_t mx;
    };

    struct ZoneNode {
        ZoneNode *parent;
        ZoneData zone;
        std::optional<std::vector<ZoneNode>> children;
    };

    using zones_t = std::shared_ptr<std::vector<ZoneNode>>;

    class ZoneRef : public Zone {
    public:
        ZoneRef(ptr_t& mgr, ZoneNode& node)
            : mgr_{mgr}, node_{node} {}

        ZoneRef(ZoneNode& node)
            : node_{node} {}

        mutable ptr_t mgr_;
        ZoneNode& node_;

        std::string label() const override { return node_.zone.label; };
        a_list_t a() const override { return node_.zone.a; }
        aaaa_list_t aaaa() const override { return node_.zone.aaaa; }
        ns_list_t ns() const override { return node_.zone.ns; }
        mx_list_t mx() const override { return node_.zone.mx; }
        std::string cname() const override { return node_.zone.cname; }
        std::string txt() const override { return {node_.zone.txt}; }
        soa_t soa() const override { return node_.zone.soa; }
        bool authorative() const override { return node_.zone.authrorative; }
        Zone::ptr_t parent() const override {
            assert(mgr_);
            if (node_.parent) {
                return std::make_shared<ZoneRef>(mgr_, *node_.parent);
            }
            return {};
        };
        std::string GetDomainName() const override;
        bool HaveCname() const noexcept override { return !node_.zone.cname.empty(); }
        bool HaveTxt() const noexcept override { return !node_.zone.txt.empty();}
        std::ostream& Print(std::ostream& out, int level = 0) const override;
    };

    Zone::ptr_t Lookup(const key_t& key, bool *authorative) const override;

    void Load(const std::filesystem::path& path);
    void Save(const std::filesystem::path& path);

    // Instantiates the manager and loads data
    static ptr_t Create(const DnsConfig& config);

private:
    using it_t = key_t::const_reverse_iterator;

//    ZoneMemoryImpl::ptr_t Lookup(it_t label, it_t end,
//                                 const ZoneMemoryImpl::zones_t& zones,
//                                 bool *authorative) const;

   zones_t zones_;
};

} // namespace
