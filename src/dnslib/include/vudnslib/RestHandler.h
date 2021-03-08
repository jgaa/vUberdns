#pragma once

#include <string_view>

#include "vudnslib/ZoneMgr.h"
#include "vudnslib/HttpServer.h"

namespace vuberdns {

class RestHandler
{
public:
    RestHandler(ZoneMgr& zoneMgr);

    http::HttpServer::Reply Process(const http::HttpServer::Request& req);

private:
    http::HttpServer::Reply GetZone(const std::string_view& path, bool recurse = false);
    http::HttpServer::Reply ProcessGet(const http::HttpServer::Request& req);
    http::HttpServer::Reply ProcessPost(const http::HttpServer::Request& req);

    Zone::ptr_t Lookup(const std::string_view &path);

    ZoneMgr& zone_mgr_;

};

}
