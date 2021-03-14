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
        ZoneData() = default;
        ZoneData(const Zone& z) {
            assign(z);
        }

        std::string label;
        Zone::soa_t soa;
        bool authorative = true;
        std::optional<std::string> txt;
        Zone::a_list_t a;
        Zone::aaaa_list_t aaaa;
        std::optional<std::string> cname;
        Zone::ns_list_t ns;
        Zone::mx_list_t mx;

        ZoneData& assign(const Zone& zone);
    };

    struct ZoneNode {
        ZoneNode *parent = {};
        ZoneData zone;
        std::optional<std::vector<ZoneNode>> children;

        void Init(ZoneNode *parent);
    };

    using zones_container_t = std::vector<ZoneNode>;
    using zones_t = std::shared_ptr<zones_container_t>;

    class ZoneRef : public Zone {
    public:
        ZoneRef(const zones_t& root, const ZoneNode& node)
            : root_{root}, node_{node} {}

        ZoneRef(const ZoneNode& node)
            : node_{node} {}

        mutable zones_t root_;
        const ZoneNode& node_;

        std::string label() const override { return node_.zone.label; };
        a_list_t a() const override { return node_.zone.a; }
        aaaa_list_t aaaa() const override { return node_.zone.aaaa; }
        ns_list_t ns() const override { return node_.zone.ns; }
        mx_list_t mx() const override { return node_.zone.mx; }
        std::string cname() const override { return node_.zone.cname ? *node_.zone.cname : std::string{};}
        std::string txt() const override { return node_.zone.txt ? *node_.zone.txt : std::string{}; }
        soa_t soa() const override { return node_.zone.soa; }
        bool authorative() const override { return node_.zone.authorative; }
        Zone::ptr_t parent() const override {
            assert(root_);
            if (node_.parent) {
                return std::make_shared<ZoneRef>(root_, *node_.parent);
            }
            return {};
        };
        std::string GetDomainName() const override;
        bool HaveCname() const noexcept override { return node_.zone.cname && !node_.zone.cname->empty(); }
        bool HaveTxt() const noexcept override { return node_.zone.txt && !node_.zone.txt->empty();}
        std::ostream& Print(std::ostream& out, int level = 0) const override;

        void ForEachChild(const std::function<void(Zone& zone)>& fn) override {
            if (node_.children) {
                // FIXME: Make const implementation
                ForEachZone_(root_, *const_cast<ZoneNode&>(node_).children, fn);
            }
        }
    };

    Zone::ptr_t Lookup(const key_t& key, bool *authorative) const override;

    void Load(const std::filesystem::path& path);
    void Save(zones_container_t& storage, const std::filesystem::path& path);

    // Instantiates the manager and loads data
    static ptr_t Create(const DnsConfig& config);

private:
    static void ForEachZone_(const zones_t& root, zones_container_t& zones, const zone_fn_t& fn);
    void IncrementSerial(ZoneNode& node);

   zones_t zones_;
   std::filesystem::path storage_path_;

   // ZoneMgr interface
public:
   Zone::ptr_t CreateZone(const std::string_view &domain, const Zone &zone) override;
   Zone::ptr_t Update(const std::string_view &domain, const Zone &zone) override;
   static ZoneNode *Search(zones_t& zones, const key_t& key);
   zones_t CopyZones();
   // Replace the current zones with the new one
   void Commit(zones_t& zones);

   void Delete(const std::string_view &domain) override;
   void ForEachZone(const zone_fn_t& fn) override;
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
    (std::optional<std::string>, txt)
    (vuberdns::Zone::a_list_t, a)
    (vuberdns::Zone::aaaa_list_t, aaaa)
    (std::optional<std::string>, cname)
    (vuberdns::Zone::ns_list_t, ns)
    (vuberdns::Zone::mx_list_t, mx)
);

BOOST_FUSION_ADAPT_STRUCT(vuberdns::ZoneMgrJson::ZoneNode,
    (vuberdns::ZoneMgrJson::ZoneData, zone)
    (std::optional<std::vector<vuberdns::ZoneMgrJson::ZoneNode>>, children)
);
