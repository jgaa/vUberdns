#pragma once

#include <string_view>

#include "vudnslib/ZoneMgr.h"
#include "vudnslib/HttpServer.h"

namespace vuberdns {

class RestHandler
{
public:
    enum class Mode {
        POST,
        PUT,
        PATCH
    };

    RestHandler(ZoneMgr& zoneMgr);

    http::HttpServer::Reply Process(const http::HttpServer::Request& req);

private:
    http::HttpServer::Reply GetZone(const std::string_view& path, bool recurse = false);
    http::HttpServer::Reply ProcessGet(const http::HttpServer::Request& req);
    http::HttpServer::Reply ProcessSet(const http::HttpServer::Request& req, const Mode mode);
    http::HttpServer::Reply ProcessDelete(const http::HttpServer::Request& req);

    Zone::ptr_t Lookup(const std::string_view &path);

    ZoneMgr& zone_mgr_;

};

}
