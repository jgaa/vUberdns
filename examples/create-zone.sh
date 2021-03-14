#!/bin/sh

curl -d '{"ns": [{"fqdn": "ns1"},{"fqdn": "ns2"}],"soa": {}}' -H 'Content-Type: application/json' http://127.0.0.1:8080/zone/example.com

curl -d '{"a": ["127.0.0.1"]}' -H 'Content-Type: application/json' http://127.0.0.1:8080/zone/ns1.example.com

curl -d '{"a": ["127.0.0.2"]}' -H 'Content-Type: application/json' http://127.0.0.1:8080/zone/ns2.example.com

curl -X PUT -d '{"a": ["127.0.0.1"]}' -H 'Content-Type: application/json' http://127.0.0.1:8080/zone/blog.example.com
