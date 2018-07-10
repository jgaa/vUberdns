#pragma once

#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include <boost/optional.hpp>

namespace vuberdns {

// Interface for Zone
class Zone {
public:
    using ptr_t = std::shared_ptr<Zone>;

    Zone() = default;
    virtual ~Zone() = default;
    Zone(const Zone&) = delete;
    Zone(Zone&&) = delete;
    void operator = (const Zone&) = delete;
    void operator = (Zone&&) = delete;

    struct Ns {
        Ns(const std::string& fqdnVal, Zone::ptr_t z)
        : fqdn{fqdnVal}, zone{std::move(z)} {}

        std::string fqdn;
        Zone::ptr_t zone;

        std::string GetFdqn() const {
            static const std::string dot(".");
            if (zone)
                return fqdn + dot + zone->GetDomainName();
            return fqdn;
        }
    };

    using ns_t = struct Ns;

    struct Mx : public ns_t {
        Mx(const std::string& fqdnVal, uint16_t pri, Zone::ptr_t z)
        : ns_t{fqdnVal, std::move(z)}, priority(pri) {}

        uint16_t priority = 0;
    };

    using mx_t = struct Mx;

    struct Soa { // defaults from RFC 1035 and usual conventions

        const std::string& GetRname() const noexcept {
            return rname.empty() ? hostmaster_ : rname;
        }

        // FIXME: How do we handle this?
        const Ns *mname = {};
        std::string rname;
        uint32_t serial = 1;
        uint32_t refresh = 7200;
        uint32_t retry = 600;
        uint32_t expire = 3600000;
        uint32_t minimum = 60;

    private:
        static const std::string hostmaster_;
    };

    using soa_t = boost::optional<Soa>;

    using a_list_t = boost::optional<std::vector<boost::asio::ip::address_v4>>;
    using aaaa_list_t = boost::optional<std::vector<boost::asio::ip::address_v6>>;
    using ns_list_t = boost::optional<std::vector<ns_t>>;
    using mx_list_t = boost::optional<std::vector<mx_t>>;

    virtual ptr_t parent() const { return {}; }

    virtual Soa GetSoa() const {

        if (auto s = soa()) {
            return *s;
        }
        if (auto p = parent()) {
            return p->GetSoa();
        }

        return {};
    };

    virtual std::string label() const { return {}; }
    virtual a_list_t a() const { return {}; }
    virtual aaaa_list_t aaaa() const { return {}; }
    virtual ns_list_t ns() const { return {};}
    virtual mx_list_t mx() const { return {}; }
    virtual std::string cname() const { return {}; } //fqdn
    virtual soa_t soa() const {return {}; }
    virtual bool authorative() const { return false; }
    virtual std::string GetDomainName() const { return {}; }
    virtual std::ostream& Print(std::ostream& out, int level = 0) const = 0;
    virtual bool HaveCname() const noexcept { return false; }

private:
};

}

