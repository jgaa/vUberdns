#pragma once

#include "vudnslib/HttpServer.h"

namespace vuberdns {

class ZoneMgr;

class RestHandler
{
public:
    RestHandler(ZoneMgr& zoneMgr);

    http::HttpServer::Reply Process(const http::HttpServer::Request& req);

private:
    http::HttpServer::Reply GetZone(const std::string_view& path);

    ZoneMgr& zone_mgr_;

};

}
