
#include "vudnslib/Zone.h"
#include <boost/fusion/adapted.hpp>
#include "vudnslib/RestHandler.h"
#include "vudnslib/util.h"
#include "ZoneMgrJson.h"
#include "warlib/WarLog.h"
#include "restc-cpp/SerializeJson.h"


struct RestReply {
    std::optional<bool> error = false;
    std::optional<std::string> reason;
    using rr_zones_t = std::map<std::string, vuberdns::ZoneMgrJson::ZoneData>;
    rr_zones_t nodes;
    void add(const vuberdns::Zone& zone) {
        nodes[zone.GetDomainName()] = zone;
    }
};

BOOST_FUSION_ADAPT_STRUCT(RestReply,
    (std::optional<bool>, error)
    (std::optional<std::string>, reason)
    (RestReply::rr_zones_t, nodes)
);

namespace vuberdns {

using namespace std;
using namespace war;
using namespace std::string_literals;
using namespace std::placeholders;

namespace {
    http::HttpServer::Reply make_reply(const RestReply& data,
                                       uint16_t httpCode = 200) {
        if (data.reason) {
            LOG_DEBUG << "Returning " << httpCode << ' ' << *data.reason;
        }
        return {httpCode, data.reason.has_value(), toJson(data)};
    }

    http::HttpServer::Reply make_reply(const Zone& zone,
                                       uint16_t httpCode = 200) {

        RestReply r;
        r.add(zone);
        return make_reply(r, httpCode);
    }

    http::HttpServer::Reply make_reply(const std::string& reason,
                                       uint16_t httpCode = 404) {

        return make_reply({true, reason, {}}, httpCode);
    }

    void Merge(ZoneMgrJson::ZoneData& zone, const Zone& existingZone) {
        if (!zone.soa) {
            zone.soa = existingZone.soa();
        }

        if (!zone.a && !zone.aaaa && !zone.cname) {
            zone.a = existingZone.a();
            zone.aaaa = existingZone.aaaa();
            zone.cname = existingZone.cname();
        } else {
            if (!zone.cname || zone.cname->empty()) {
                if (!zone.a) {
                    zone.a = existingZone.a();
                }
                if (!zone.aaaa) {
                    zone.aaaa = existingZone.aaaa();
                }
            }
        }

        if (!zone.txt) {
            zone.txt = existingZone.txt();
        }

        if (!zone.ns) {
            zone.ns = existingZone.ns();
        }

        if (!zone.mx) {
            zone.mx = existingZone.mx();
        }
    }
}

RestHandler::RestHandler(ZoneMgr &zoneMgr)
    : zone_mgr_{zoneMgr}
{
}

http::HttpServer::Reply RestHandler::Process(const http::HttpServer::Request &req)
{
    if (req.verb == "GET") {
        return ProcessGet(req);
    }

    if (req.verb == "POST") {
        return ProcessSet(req, Mode::POST);
    }

    if (req.verb == "PUT") {
        return ProcessSet(req, Mode::PUT);
    }

    if (req.verb == "PATCH") {
        return ProcessSet(req, Mode::PATCH);
    }

    if (req.verb == "DELETE") {
        return ProcessDelete(req);
    }

    return make_reply("Not implemented");
}

http::HttpServer::Reply RestHandler::GetZone(const string_view &path, bool recurse)
{
    auto key = Split<boost::string_ref>(path, '.');

    if (auto zone = zone_mgr_.Lookup(key)) {
        RestReply r;
        r.add(*zone);

        if (recurse) {
            zone->ForEachChild([&r](const auto& cz) {
               r.add(cz);
            });
        }

        return make_reply(r);
    }

    return make_reply("Zone Not Found");
}

http::HttpServer::Reply RestHandler::ProcessGet(const http::HttpServer::Request &req)
{
    auto paths = Split(req.path);
    if (paths.empty()) {
        return make_reply("Unknown Resource");
    }

    if (paths.at(0) == "zone") {
        return GetZone(paths.at(1), false);
    }

    if (paths.at(0) == "zones") {
        if (paths.size() == 1) {
            // Get root zones
            RestReply r;
            zone_mgr_.ForEachZone([&](const Zone& zone) {
                if (!zone.parent()) {
                    r.add(zone);
                }
            });
            return make_reply(r);
        }
        if (paths.size() == 2) {
            RestReply r;
            const auto what = paths.at(1);
            if (what == "authorative") {
                zone_mgr_.ForEachZone([&](const Zone& zone) {
                    if (zone.authorative() && zone.soa()) {
                        r.add(zone);
                    }
                });
                return make_reply(r);
            }

            if (what == "soa") {
                zone_mgr_.ForEachZone([&](const Zone& zone) {
                    if (zone.soa()) {
                        r.add(zone);
                    }
                });
                return make_reply(r);
            }
        }
        if (paths.size() == 3) {
            const auto what = paths.at(1);
            if (what == "recurse") {
                return GetZone(paths.at(2), true);
            }
        }
    }

    return make_reply("Not implemented");
}

http::HttpServer::Reply RestHandler::ProcessSet(const http::HttpServer::Request &req,
                                                const Mode mode)
{
    auto paths = Split(req.path);
    if (paths.size() != 2 || paths.at(0) != "zone") {
        return make_reply("Invalid Request");
    }

    const auto zone_name = paths.at(1);

    const auto parts = Split(zone_name, '.');
    if (parts.empty()) {
        return make_reply("No zone name provided", 400);
    }

    const auto name = parts.front();

    ZoneMgrJson::ZoneData zone;
    try {
        restc_cpp::SerializeFromJson(zone, req.body);
    } catch (const std::exception& ex) {
        LOG_WARN << "Failed to serialize json: " << req.body;
        make_reply("Invalid json", 400);
    }

    if (zone.label.empty()) {
        zone.label = name;
    }

    bool create = false;
    switch (mode) {
    case Mode::POST:
        create = true;
        break;
    case Mode::PUT:
        create = get<0>(zone_mgr_.LookupName(zone_name)) == nullptr;
        break;
    case Mode::PATCH: {
        auto [currentZone,_] = zone_mgr_.LookupName(zone_name);
        if (currentZone) {
            Merge(zone, *currentZone);
        } else {
            create = true;
        }
        break;
        }
    }

    // Get a Zone interface to the new zone
    ZoneMgrJson::ZoneNode zn{{}, zone};
    ZoneMgrJson::ZoneRef z{zn};

    try {
        if (create) {
            return make_reply(*zone_mgr_.CreateZone(zone_name, z));
        }
        return make_reply(*zone_mgr_.Update(zone_name, z));
    } catch(const AlreadyExistsException& ex) {
        return make_reply(ex.what(), 409);
    } catch(const NotFoundException& ex) {
        return make_reply(ex.what(), 404);
    } catch (const exception& ex) {
        return make_reply(ex.what(), 400);
    }

    assert(false);
}

http::HttpServer::Reply RestHandler::ProcessDelete(const http::HttpServer::Request &req)
{
    auto paths = Split(req.path);
    if (paths.size() != 2 || paths.at(0) != "zone") {
        return make_reply("Invalid Request");
    }

    const auto zone_name = paths.at(1);

    try {
        zone_mgr_.Delete(zone_name);
        return make_reply("Zone deleted", 200);
    } catch(const AlreadyExistsException& ex) {
        return make_reply(ex.what(), 409);
    } catch(const NotFoundException& ex) {
        return make_reply(ex.what(), 404);
    } catch (const exception& ex) {
        return make_reply(ex.what(), 400);
    }

    assert(false);
}

Zone::ptr_t RestHandler::Lookup(const string_view &path)
{
    auto key = Split<boost::string_ref>(path, '.');
    return zone_mgr_.Lookup(key);
}


} // ns
