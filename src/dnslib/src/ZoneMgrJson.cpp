
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
    for(auto z = &node_; z; z = z->parent) {
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
    zones_ = move(zones);
    storage_path_ = path;
}

void ZoneMgrJson::Save(zones_container_t& storage, const std::filesystem::path &path)
{
    if (path.empty()) {
        throw runtime_error("I don't know the path to the json storage file. Load() before you try to Save() ?");
    }

    if (path.extension() != "json") {
        throw runtime_error("Can only save to .json backed storage!");
    }

    auto tmpName = path.string() + ".tmp";
    std::ofstream out{path.string()};
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

bool ZoneMgrJson::Validate(const string &domain, const Zone &zone)
{
    if (!zone.cname().empty()) {
        if (!zone.a()->empty() || !zone.aaaa()->empty()) {
            LOG_WARN << "Validate: Zone " << domain << " Must use either cname or a/aaaa";
            return false; // Must be either cname or IP
        }
    }

    auto k = toKey(domain);
    if (!k.empty()) {
        if (k.back() != zone.label()) {
            LOG_WARN << "Validate: Zone " << domain << " Expected label to be " << k.back();
            return false;
        }
    }

    return true;
}

Zone::ptr_t vuberdns::ZoneMgrJson::CreateZone(const string &domain, const Zone &zone)
{
    if (auto [z, _] = LookupName(domain); z) {
        throw AlreadyExistsException("Domain "s + domain + " already exists");
    }

    if (!Validate(domain, zone)) {
        throw ValidateException("Domain "s + domain + " fails validation");
    }

    // Create a working copy of the domains
    auto zones = make_shared<zones_container_t>(*zones_);

    // Insert the zone
    auto hosts = toKey(domain);
    reverse(hosts.begin(), hosts.end());
    auto current_level = zones.get();
    ZoneNode *current = {};
    bool added = false;

    // Walk the hostnames of the domain, creating "zones" as needed
    for(auto& host: hosts) {
        added = false;

        if (current_level) {
            if (current) {
                if (!current->children) {
                    current->children.emplace();
                }

                current_level = &current->children.value();
            } else {
                assert(false);
                throw runtime_error("Internal error - No current level or current zone");
            }

         }

        assert(current_level);

        for(auto& z : *current_level) {
            if (host == z.zone.label) {
                current = &z;
                if (current->children) {
                    current_level = &current->children.value();
                } else {
                    current_level = {};
                }
                continue;
            }
        }

        if (current_level->empty()) {
            // Create node.
            auto& node = current_level->emplace_back();
            node.zone.label = host.to_string();
            node.zone.authorative = false;
            node.parent = current;
            current = &node;
            current_level = {};
            added = true;
        }
    }

    if (!added) {
        throw AlreadyExistsException("Domain "s + domain + " already exists. I did not know that...");
    }

    assert(current);
    current->zone.assign(zone);

    Save(*zones, storage_path_);

    // Commit
    zones_ = move(zones);

    return make_shared<ZoneRef>(shared_from_this(), *current);
}

void vuberdns::ZoneMgrJson::Update(const string &domain, const Zone &zone)
{
}

void vuberdns::ZoneMgrJson::Delete(const string &domain)
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

} // ns
