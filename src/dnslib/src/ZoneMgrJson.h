#pragma once

#include <optional>
#include <memory>

#include <filesystem>
#include <boost/fusion/adapted.hpp>

#include "vudnslib/ZoneMgr.h"

namespace vuberdns {

class ZoneMgrJson : public ZoneMgr, public std::enable_shared_from_this<ZoneMgrJson> {
public:
    using ptr_t = std::shared_ptr<ZoneMgrJson>;
    using key_t = const std::vector<boost::string_ref>;

    struct ZoneData {
        std::string label;
        Zone::soa_t soa;
        bool authorative = false;;
        std::string txt;
        Zone::a_list_t a;
        Zone::aaaa_list_t aaaa;
        std::string cname;
        Zone::ns_list_t ns;
        Zone::mx_list_t mx;

        ZoneData& assign(const Zone& zone);
    };

    struct ZoneNode {
        ZoneNode *parent = {};
        ZoneData zone;
        std::optional<std::vector<ZoneNode>> children;
    };

    using zones_container_t = std::vector<ZoneNode>;
    using zones_t = std::shared_ptr<zones_container_t>;

    class ZoneRef : public Zone {
    public:
        ZoneRef(const ZoneMgrJson::ptr_t& mgr, const ZoneNode& node)
            : mgr_{mgr}, node_{node} {}

        ZoneRef(const ZoneNode& node)
            : node_{node} {}

        mutable ZoneMgrJson::ptr_t mgr_;
        const ZoneNode& node_;

        std::string label() const override { return node_.zone.label; };
        a_list_t a() const override { return node_.zone.a; }
        aaaa_list_t aaaa() const override { return node_.zone.aaaa; }
        ns_list_t ns() const override { return node_.zone.ns; }
        mx_list_t mx() const override { return node_.zone.mx; }
        std::string cname() const override { return node_.zone.cname; }
        std::string txt() const override { return {node_.zone.txt}; }
        soa_t soa() const override { return node_.zone.soa; }
        bool authorative() const override { return node_.zone.authorative; }
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
    void Save(zones_container_t& storage, const std::filesystem::path& path);

    // Instantiates the manager and loads data
    static ptr_t Create(const DnsConfig& config);

private:
    static bool Validate(const std::string& domain, const Zone& zone);

   zones_t zones_;
   std::filesystem::path storage_path_;

   // ZoneMgr interface
public:
   Zone::ptr_t CreateZone(const std::string &domain, const Zone &zone) override;
   void Update(const std::string &domain, const Zone &zone) override;
   void Delete(const std::string &domain) override;
};
} // namespace

BOOST_FUSION_ADAPT_STRUCT(vuberdns::Zone::Ns,
    (std::string, fqdn)
);

BOOST_FUSION_ADAPT_STRUCT(vuberdns::Zone::Mx,
    (std::string, fqdn)
    (unsigned int, priority)
);

BOOST_FUSION_ADAPT_STRUCT(vuberdns::Zone::Soa,
    (std::string, rname)
    (uint32_t, serial)
    (uint32_t, refresh)
    (uint32_t, retry)
    (uint32_t, expire)
    (uint32_t, minimum)
);

BOOST_FUSION_ADAPT_STRUCT(vuberdns::ZoneMgrJson::ZoneData,
    (std::string, label)
    (vuberdns::Zone::soa_t, soa)
    (bool, authorative)
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
