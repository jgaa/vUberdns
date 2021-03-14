#pragma once

#include <memory>
#include <vector>
#include <boost/utility/string_ref.hpp>

#include "vudnslib/Zone.h"

namespace vuberdns {

struct DnsConfig;

class ZoneMgr {
public:
    using ptr_t = std::shared_ptr<ZoneMgr>;
    using key_t = std::vector<boost::string_ref>;
    using zone_fn_t = std::function<void(Zone &zone)>;

    /*! Lookup a zone recursively, starting from the back.
     *
     * \note An empty label in the zone-tree matches all names
     *      at that level. If there is an ambiguity, with
     *      a matching name /and/ an empty label,
     *      the full match is used.
     */
    virtual Zone::ptr_t Lookup(const key_t& key,
                               bool *authorative = nullptr) const = 0;

    /*! Lookup a zone exactely matching the domain
     *
     *  \throws NotFoundException
     */
    virtual std::tuple<Zone::ptr_t /*zone*/, bool /*authorative*/> LookupName(const std::string_view& domain);

    /*! Create a zone.
     *
     *  \param domain Domain name for the zone. For example: `api.example.com`
     *  \param zone Data for the zone.
     *
     *  \throws NotFoundException NotImplementedException
     */
    virtual Zone::ptr_t CreateZone(const std::string_view& domain, const Zone& zone) {
        throw NotImplementedException{};
    }

    /*! Update a zone
     *
     *  Child zones are not touched.
     *  If this is a Soa zone, and soa.serial is not specified, the server will
     *  increment the serial number. Else, the parent zoa will have it's serial
     *  number incremented.
     *
     *  \throws AlreadyExistsException NotImplementedException
     */
    virtual Zone::ptr_t Update(const std::string_view& domain, const Zone& zone) {
        throw NotImplementedException{};
    }

    /*! Delete a zone and all it's subdomains
     *
     *  Parent soa will have its serial number updated.
     *
     *  \throws NotImplementedException
     */
    virtual void Delete(const std::string_view& domain) {
        throw NotImplementedException{};
    }

    /*! Create the zone manager
     *
     *  Loads existing zones.
     */
    static ptr_t Create(const DnsConfig& config);

    /* Returns a list of hostnames from the domain name, in reverse order.
     *
     * \note The list contains memory references into the memory buffer for the
     *       `domain` variable. This needs to exist at least for the same
     *       scope as the returned "key".
     */
    static key_t toKey(const std::string_view& domain);

    virtual void ForEachZone(const zone_fn_t& fn) {
        throw NotImplementedException{};
    }
};

} // namespace

