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
    using key_t = const std::vector<boost::string_ref>;

    /*! Lookup a zone recursively, starting from the back.
     *
     * \note An empty label in the zone-tree matches all names
     *      at that level. If there is an ambiguity, with
     *      a matching name /and/ an empty label,
     *      the full match is used.
     */
    virtual Zone::ptr_t Lookup(const key_t& key,
                               bool *authorative = nullptr) const = 0;

    static ptr_t Create(const DnsConfig& config);
};

} // namespace

