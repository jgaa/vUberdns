
#include <filesystem>

#include "ZoneMgrJson.h"
#include "vudnslib/DnsConfig.h"
#include "vudnslib/util.h"

#include "warlib/WarLog.h"

namespace vuberdns {

using namespace std;

string ZoneMgrJson::ZoneRef::GetDomainName() const
{
    auto name = label();
    for(auto z = node_.parent; z; z = z->parent) {
        name += '.';
        name += z->zone.label;
    }

    return name;
}

ostream &ZoneMgrJson::ZoneRef::Print(ostream &out, int level) const
{
    out << std::setw(level * 2) << '{' << std::setw(0) << '\"' << label()
        << '\"' << std::endl;
    if (auto c = node_.children) {
        for(auto& n : *c) {
            ZoneRef zone{n};
            zone.Print(out, level + 1);
        }
    }
    out << std::setw(level * 2) << '}' << std::setw(0) << std::endl;
    return out;
}

Zone::ptr_t ZoneMgrJson::Lookup(const ZoneMgrJson::key_t &key, bool *authorative) const
{
    if (!zones_) {
        return {};
    }

    auto zones = zones_.get();
    ZoneNode *current = {};
    for(auto host = key.rbegin(); host != key.rend(); ++host) {
        current = {};
        for(auto& z: *zones) {
            if (z.zone.label == *host) {
                current = &z;
                if (current->children && !current->children->empty()) {
                    zones = &current->children.value();
                }
            }
        }

        if (!current) {
            break;
        }
    }

    if (current) {
        if (authorative) {
            *authorative = current->zone.authorative;
        }
        return make_shared<ZoneRef>(const_cast<ZoneMgrJson*>(this)->shared_from_this(), *current);
    }

    return {};
}

void ZoneMgrJson::Load(const std::filesystem::path &path)
{
    auto zones = make_shared<std::vector<ZoneNode>>();
    fileToObject(*zones, path);
    for(auto& z: *zones) {
        z.Init({});
    }
    zones_ = move(zones);
    storage_path_ = path;
}

void ZoneMgrJson::Save(zones_container_t& storage, const std::filesystem::path &path)
{
    if (path.empty()) {
        throw runtime_error("I don't know the path to the json storage file. Load() before you try to Save() ?");
    }

    if (path.extension() != ".json") {
        throw runtime_error("Can only save to .json backed storage!");
    }

    auto tmpName = path.string() + ".tmp";
    std::ofstream out{tmpName};
    restc_cpp::SerializeToJson(storage, out);
    filesystem::rename(tmpName, path);
}

ZoneMgrJson::ptr_t ZoneMgrJson::Create(const DnsConfig &config)
{
    auto mgr = make_shared<ZoneMgrJson>();

    filesystem::path path = config.data_path;

    if (std::filesystem::is_regular_file(path)) {
        mgr->Load(path);
    } else {
        LOG_WARN_FN << "The dns zones file "
            << war::log::Esc(config.data_path)
            << " does not exist!. Cannot load zones";
    }

    return mgr;
}

Zone::ptr_t vuberdns::ZoneMgrJson::CreateZone(const string_view &domain, const Zone &zone)
{
    if (auto [z, _] = LookupName(domain); z) {
        throw AlreadyExistsException("Domain "s + string(domain) + " already exists");
    }

    if (!Zone::Validate(domain, zone)) {
        throw ValidateException("Domain "s + string(domain) + " fails validation");
    }

    // Create a working copy of the domains
    auto zones = make_shared<zones_container_t>(*zones_);

    // We need to adjust the parent pointers, or they will point to the old instances.
    for(auto& zi: *zones) {
        zi.Init({});
    }

    // Insert the zone
    auto hosts = toKey(domain);
    reverse(hosts.begin(), hosts.end());
    auto current_level = zones.get();
    ZoneNode *current = {};
    bool added = false;

    // Walk the hostnames of the domain, creating "zones" as needed
    for(auto& host: hosts) {
        added = false;
        bool found = false;
        if (current_level) {
            for(auto& z : *current_level) {
                if (host == z.zone.label) {
                    found = true;
                    current = &z;
                    if (current->children) {
                        current_level = &current->children.value();
                    } else {
                        current_level = {};
                    }
                }
            }
        }
        if (found) {
            // We have the hostname at this level already. Go to the next level.
            continue;
        }

        if (!current_level) {
            assert(current);
            if (!current->children) {
                current->children.emplace();
            }
            current_level = &current->children.value();
        }

        // Create node.
        auto parent = current;
        current = &current_level->emplace_back();
        assert(current);
        current->zone.label = host.to_string();
        current->zone.authorative = false;
        current->parent = parent;
        current_level = {};
        added = true;
    }

    if (!added) {
        throw AlreadyExistsException("Domain "s + string(domain) + " already exists. I did not know that...");
    }

    assert(current);
    current->zone.assign(zone);
    IncrementSerial(*current);
    Save(*zones, storage_path_);

    // Commit
    zones_ = move(zones);

    return make_shared<ZoneRef>(shared_from_this(), *current);
}

void vuberdns::ZoneMgrJson::Update(const string_view &domain, const Zone &zone)
{
}

void vuberdns::ZoneMgrJson::Delete(const string_view &domain)
{
}

ZoneMgrJson::ZoneData &ZoneMgrJson::ZoneData::assign(const Zone &zone)
{
    label = zone.label();
    soa = zone.soa();
    authorative = zone.authorative();
    txt = zone.txt();
    a = zone.a();
    aaaa = zone.aaaa();
    cname = zone.cname();
    ns = zone.ns();
    mx = zone.mx();

    return *this;
}

void ZoneMgrJson::ForEachZone_(ZoneMgrJson::zones_container_t &zones, const ZoneMgr::zone_fn_t &fn)
{
    for(auto& zone : zones) {
        {
            ZoneRef z{shared_from_this(), zone};
            fn(z);
        }

        if (zone.children) {
            ForEachZone_(*zone.children, fn);
        }
    }
}

void ZoneMgrJson::IncrementSerial(ZoneMgrJson::ZoneNode &node)
{
    auto n = &node;
    for(; n; n = n->parent) {
        if (n->zone.soa && n->zone.authorative) {
            break;
        }
    }

    if (!n) {
        throw ValidateException{"I am not authorative for that domain"};
    }

    ++n->zone.soa->serial;
}



void ZoneMgrJson::ForEachZone(const zone_fn_t& fn)
{
    if (zones_) {
        ForEachZone_(*zones_, fn);
    }
}

void ZoneMgrJson::ZoneNode::Init(ZoneMgrJson::ZoneNode *parent)
{
    this->parent = parent;
    if (children) {
        for(auto &c: *children) {
            c.Init(this);
        }
    }
}

} // ns

