# vUberdns

**Simple, ultrafast DNS server**.

This DNS server is designed to serve a limited number of zones, and to do that as fast as possible while consuming few resources. First of all, - all information is kept in memory. So when a request comes in, the server can answer immediately - without looking up any database tables or querying any external entities.

It does not need to write to disk. Any number of instances can be deployed using shared, read-only storage volumes with the zone information.

## Motivation

I needed a simple DNS server for some private projects, so I decided to make one. The code is currently a re-factored version of the embedded DNS server in the [vUbercool](https://github.com/jgaa/vUbercool) project. DNS have over the years proven to be a security nightmare, with lots and lots of exploits found in popular servers and client libraries. To keep things safe and sound, I feel better running a server that I myself have designed with security in mind, and with a limited number of features.

## Current Status

I just started. It can load a zone file (in a simple text file format) into memory and serve the most basic requests.

I plan to add more features rapidly over the next weeks, and make a beta release when I feel it is useful and ready to be tested in the wild.

## Operating Systems

It's designed for *nix. So far I have only tested it with Debian Linux.

## How to test it?

Build it with cmake, and start it in a console:

```sh
~/src/vUberdns$ ./build/src/dnsd/vudnsd -H "localhost:5353" --io-threads 2 -z conf/exampe_zone.txt -C

```

This will start it, and listen for requests on localhost, port 5353

Then, use for example *dig* to query it from another console:

```sh
~$ dig @127.0.0.1 -p 5353 onebillionsites.com

; <<>> DiG 9.10.3-P4-Debian <<>> @127.0.0.1 -p 5353 onebillionsites.com
; (1 server found)
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 63363
;; flags: qr rd ad; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0
;; WARNING: recursion requested but not available

;; QUESTION SECTION:
;onebillionsites.com.           IN      A

;; ANSWER SECTION:
onebillionsites.com.    300     IN      A       127.0.0.1

;; Query time: 0 msec
;; SERVER: 127.0.0.1#5353(127.0.0.1)
;; WHEN: Sun Jul 08 17:49:15 EEST 2018
;; MSG SIZE  rcvd: 53
```

```sh
~$ dig @127.0.0.1 -p 5353 onebillionsites.com soa

; <<>> DiG 9.10.3-P4-Debian <<>> @127.0.0.1 -p 5353 onebillionsites.com soa
; (1 server found)
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 60944
;; flags: qr rd ad; QUERY: 1, ANSWER: 1, AUTHORITY: 2, ADDITIONAL: 2
;; WARNING: recursion requested but not available

;; QUESTION SECTION:
;onebillionsites.com.           IN      SOA

;; ANSWER SECTION:
onebillionsites.com.    300     IN      SOA     ns1.onebillionsites.com. hostmaster.onebillionsites.com. 1 7200 600 3600000 60

;; AUTHORITY SECTION:
onebillionsites.com.    300     IN      NS      ns1.onebillionsites.com.
onebillionsites.com.    300     IN      NS      ns2.onebillionsites.com.

;; ADDITIONAL SECTION:
ns1.onebillionsites.com. 300    IN      A       127.0.0.1
ns2.onebillionsites.com. 300    IN      A       127.0.0.2

;; Query time: 0 msec
;; SERVER: 127.0.0.1#5353(127.0.0.1)
;; WHEN: Sun Jul 08 17:55:59 EEST 2018
;; MSG SIZE  rcvd: 152
```

## Running on port 53

If you want the server to use port 53 under Linux, you have several options. You can run it as root (bad, bad idea), or you can give the application permission to use privileged ports. One way to do that is to run the command below.

```sh
$ sudo setcap CAP_NET_BIND_SERVICE=+eip  ./build/src/dnsd/vudnsd
```

## Prebuilt Docker container

```sh
$ docker run --name vudnsd -p 53:53/udp -d --rm jgaafromnorth/vuberdns
```

