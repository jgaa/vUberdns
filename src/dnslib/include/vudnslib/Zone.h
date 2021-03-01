#pragma once

#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include <optional>

namespace vuberdns {

class NotFoundException : public std::runtime_error
{
public:
    NotFoundException(const std::string& why)
        : runtime_error(why) {}
};

class AlreadyExistsException : public std::runtime_error
{
public:
    AlreadyExistsException(const std::string& why)
        : runtime_error(why) {}
};

class ValidateException : public std::runtime_error
{
public:
    ValidateException(const std::string& why)
        : runtime_error(why) {}
};

class NotImplementedException : public std::runtime_error
{
public:
    NotImplementedException()
        : runtime_error("Not implemented") {}
};

// Interface for Zone
class Zone {
public:
    using ptr_t = std::shared_ptr<Zone>;

    // Boost asio ipv4/6 can't initialize directly from strings, so we either
    // have to write code to do it after we deserialize json, or add the magic below
    // to make the deserializer do it directly.
    template <typename T>
    class ipT : public T {
    public:
        using base_t = T;
        ipT() = default;
        ipT(const base_t& v) : base_t{v} {}
        ipT(base_t&& v) : base_t{std::move(v)} {}
        ipT(const std::string& ip)
            : base_t(from_string(ip))
        {}

        ipT& operator = (const base_t& v) {
            base_t::operator = (v);
            return *this;
        }

        ipT& operator = (const base_t&& v) {
            base_t::operator = (std::move(v));
            return *this;
        }

        ipT& operator = (const std::string& ip) {
            base_t::operator = (from_string(ip));
            return *this;
        }

        static base_t from_string(const std::string& ip) {
            if constexpr (std::is_same_v<base_t, boost::asio::ip::address_v4>) {
                return boost::asio::ip::make_address_v4(ip);
            }
            if constexpr (std::is_same_v<base_t, boost::asio::ip::address_v6>) {
                return boost::asio::ip::make_address_v6(ip);
            }
        }
    };

    using ipv4_t = ipT<boost::asio::ip::address_v4>;
    using ipv6_t = ipT<boost::asio::ip::address_v6>;

    Zone() = default;
    virtual ~Zone() = default;
    Zone(const Zone&) = delete;
    Zone(Zone&&) = delete;
    void operator = (const Zone&) = delete;
    void operator = (Zone&&) = delete;

    struct Ns {
        Ns() = default;
        Ns(const std::string& fqdnVal)
        : fqdn{fqdnVal} {}

        std::string fqdn;

        std::string GetFdqn(const std::string& domainName) const {
            static const std::string dot(".");
            if (!domainName.empty())
                return fqdn + dot + domainName;
            return fqdn;
        }
    };

    using ns_t = struct Ns;

    struct Mx : public ns_t {
        Mx() = default;
        Mx(const std::string& fqdnVal, uint16_t pri)
        : ns_t{fqdnVal}, priority(pri) {}

        unsigned int priority = 0;
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

    using soa_t = std::optional<Soa>;

    using a_list_t = std::optional<std::vector<ipv4_t>>;
    using aaaa_list_t = std::optional<std::vector<ipv6_t>>;
    using ns_list_t = std::optional<std::vector<ns_t>>;
    using mx_list_t = std::optional<std::vector<mx_t>>;

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
    virtual std::string txt() const { return {}; }
    virtual soa_t soa() const {return {}; }
    virtual bool authorative() const { return false; }
    virtual std::string GetDomainName() const { return {}; }
    virtual std::ostream& Print(std::ostream& out, int level = 0) const = 0;
    virtual bool HaveCname() const noexcept { return false; }
    virtual bool HaveTxt() const noexcept { return false; }

private:
};

}

// Needed to serialize zones to json
namespace std {
inline std::string to_string(const vuberdns::Zone::ipv4_t& v) {
    return v.to_string();
}

inline std::string to_string(const vuberdns::Zone::ipv6_t& v) {
    return v.to_string();
}
}

