
#include "vudnslib/Zone.h"
#include <boost/fusion/adapted.hpp>
#include "vudnslib/RestHandler.h"
#include "vudnslib/util.h"
#include "ZoneMgrJson.h"


struct RestReply {
    std::optional<bool> error = false;
    std::optional<std::string> reason;
    std::optional<std::vector<vuberdns::ZoneMgrJson::ZoneData>> zones;
};

BOOST_FUSION_ADAPT_STRUCT(RestReply,
    (std::optional<bool>, error)
    (std::optional<std::string>, reason)
    (std::optional<std::vector<vuberdns::ZoneMgrJson::ZoneData>>, zones)
);

namespace vuberdns {

using namespace std;
using namespace war;
using namespace std::string_literals;
using namespace std::placeholders;

namespace {
    http::HttpServer::Reply make_reply(const RestReply& data,
                                       uint16_t httpCode = 200) {
        return {httpCode, data.reason.has_value(), toJson(data)};
    }
}

RestHandler::RestHandler(ZoneMgr &zoneMgr)
    : zone_mgr_{zoneMgr}
{
}

http::HttpServer::Reply RestHandler::Process(const http::HttpServer::Request &req)
{
    auto paths = Split(req.path);
    if (paths.empty()) {
        return make_reply({true, "Unknown Resource", {}}, 404);
    }

    if (req.verb == "GET") {
        if (paths.at(0) == "zone") {
            return GetZone(paths.at(1));
        }

    }
    return make_reply({true, "Not implemented", {}}, 404);
}

http::HttpServer::Reply RestHandler::GetZone(const string_view &path)
{
    auto key = Split<boost::string_ref>(path, '.');

    if (auto zone = zone_mgr_.Lookup(key)) {
        RestReply r;
        r.zones.emplace();
        auto& z = r.zones->emplace_back();
        z.assign(*zone);
        return make_reply(r);
    }

    return make_reply({true, "Zone Not Found", {}}, 404);
}

} // ns
