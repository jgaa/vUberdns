# REST API

## Get a zone

Get a specific zone

`GET /zone/{zone-name}`

The json paylpoad will be similar to the one documented to crfeate a zone. 

## List zones

Zone list format:

```json

{ 
    "error": false,
    "reason": "",
    "zones": [
        {...}
    ]
}
```

Example:
``` 
~$ curl -s http://127.0.0.1:8080/zone/example.com  |  python -m json.tool 
```
```json
{
    "error": false,
    "nodes": {
        "example.com": {
            "authorative": true,
            "label": "example",
            "ns": [
                {
                    "fqdn": "ns1"
                },
                {
                    "fqdn": "ns2"
                }
            ],
            "soa": {
                "expire": 3600000,
                "minimum": 60,
                "refresh": 7200,
                "retry": 600,
                "serial": 4
            }
        },
        "ns1.example.com": {
            "a": [
                "127.0.0.1"
            ],
            "authorative": true,
            "label": "ns1"
        },
        "ns2.example.com": {
            "a": [
                "127.0.0.2"
            ],
            "authorative": true,
            "label": "ns2"
        }
    }
}
```

The `zones` are json as described for *Create zone*.

`GET /zones` - Get the root zones from the server. 

`GET /zones/recurse/{zone-name}` - Get the child zones for a given zone

`GET /zones/authorative` - Get a list of all the zones with Soa record that the server is authorative for.

`GET /zones/soa` - Get a list of zones with a Soa record.

## Create zone

`POST /zone/{zone-name}`

The payload is a standard DNS zone in Json format. All fields are optional. If set, the label must match the last
hostname in the `zone-name` above.

The server will evaluate the valitity of the request; for example, you cannot specify both a/aaaa record(s) and a cname.

The request will fail with `409 Conflict` if the zone already exists. 

```json
{
    "soa": {
        "rname": "hostmaster",
        "serial": 1,
        "refresh": 7200,
        "retry": 600,
        "expire": 3600000,
        "minimum": 60
    },
    
    "label": "zone.name",
    "authrorative": true,
    "a": ["192.168.0.1"],
    "aaaa": ["::ffff:c0a8:1"],
    "txt": "Some Info",
    "cname": "",
    "ns": [
        { "fqdn": "ns1.example.com" },
        { "fqdn": "ns2.example.com" },
    ],
    "mx": [
        { "fqdn": "mail.example.com", "priority": 10}
    ]
}

```

## Create or update a zone.

As POST, but if the zone exists, it will be modified to the new values. 

`PUT /zone/{zone-name}`

## Change single fields in a zone

If you change from an IP number to a cname, you just have to specify the new value, and
the server will remove the other.

This is where you for example specify a txt record for certbot to verify that you control the zone.

`PATCH /zone/{zone-name}`


## Delete a zone

`DELETE /zone/{zone-name}`

This deletes a zone and all it's subdomains. 

If you delete and then re-create a zone, the zones serial numers default value will be 1. 
This may confuse dns servers and dns cahches, remembering the old zone and it's serial number. 

