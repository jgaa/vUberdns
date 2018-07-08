#pragma once

#include <memory>

#include <numeric>
#include <arpa/inet.h>
#include <boost/concept_check.hpp>
#include <boost/regex.hpp>
#include <boost/utility/string_ref.hpp>

#include "warlib/error_handling.h"

namespace vuberdns {

// See RFC1035 4.1.1 Header section format
class MessageHeader
{
public:
    struct MessageHeaderException : public war::ExceptionBase {};
    enum class Rcode : uint8_t {
        OK = 0,
        FORMAT_ERROR = 1,
        SERVER_FAILURE = 2,
        NAME_ERROR = 3,
        NOT_IMPLEMENTED = 4,
        REFUSED = 5
    };

    MessageHeader(const char *buffer, std::size_t size)
    {
        if (size < GetSize()) {
            WAR_THROW_T(MessageHeaderException, "Header-length underflow");
        }

        const uint16_t *p = reinterpret_cast<const uint16_t *>(buffer);

        id_ = ntohs(*p++);
        flags_ = ntohs(*p++);
        qdcount_ = ntohs(*p++);
        ancount_ = ntohs(*p++);
        nscount_ = ntohs(*p++);
        arcount_ = ntohs(*p++);
    }

    MessageHeader(const MessageHeader& v)
    : id_{v.id_}, flags_{v.flags_}, qdcount_{v.qdcount_},
    ancount_{v.ancount_}, nscount_{v.nscount_},
    arcount_{v.arcount_}
    {
    }

    /*! Get the length of the header */
    size_t GetSize() const { return 2 * 6; }

    /*! Get the ID in native byte order */
    uint16_t GetId() const { return id_; }

    bool GetQr() const { return (flags_ & (static_cast<uint16_t>(1) << 15)) != 0; }
    uint8_t GetOpcode() const { return (flags_ >> 11) & 0xF; }
    bool GetAa() const { return (flags_ & (1 << 10)) != 0; }
    bool GetTc() const { return (flags_ & (1 << 9)) != 0; }
    bool GetRd() const { return (flags_ & (1 << 8)) != 0; }
    bool GetRa() const { return (flags_ & (1 << 7)) != 0; }
    uint8_t GetZ() const { return (flags_ >> 4) & 0xF; }
    uint8_t GetRcode() const { return flags_ & 0xF; }
    uint16_t GetQdCount() const { return qdcount_; }
    uint16_t GetAnCount() const { return ancount_; }
    uint16_t GetNsCount() const { return nscount_; }
    uint16_t GetArCount() const { return arcount_; }

    void SetQr(bool qr) {
        flags_ &= ~(1 << 15);
        if (qr)
            flags_ |= static_cast<uint16_t>(1) << 15;
    }

    void SetTc(bool tc) {
        flags_ &= ~(1 << 9);
        if (tc)
            flags_ |= static_cast<uint16_t>(1) << 9;
    }

    void SetRa(bool ra) {
        flags_ &= ~(1 << 7);
        if (ra)
            flags_ |= static_cast<uint16_t>(1) << 7;
    }

    void SetAa(bool aa) {
        flags_ &= ~(1 << 10);
        if (aa)
            flags_ |= static_cast<uint16_t>(1) << 10;
    }

    void SetOpcode(uint8_t opcode) {
        flags_ &= ~(0xF << 11);
        flags_ |= (opcode & 0xF) << 11;
    }

    void SetRcode(Rcode opcode) {
        flags_ &= ~0xF;
        flags_ |= (static_cast<uint16_t>(opcode) & 0xF);
    }

    void SetQdCount(uint16_t val) { qdcount_ = val; }
    void SetAnCount(uint16_t val) { ancount_ = val; }
    void SetNsCount(uint16_t val) { nscount_ = val; }
    void SetArCount(uint16_t val) { arcount_ = val; }

    void ResetAllCounters() {
        qdcount_ = ancount_ = nscount_ = arcount_ = 0;
    }

    void Write(char *buffer) const {
        uint16_t *p = reinterpret_cast<uint16_t *>(buffer);

        *p++ = htons(id_);
        *p++ = htons(flags_);
        *p++ = htons(qdcount_);
        *p++ = htons(ancount_);
        *p++ = htons(nscount_);
        *p++ = htons(arcount_);
    }

private:
    std::uint16_t id_ = 0;
    std::uint16_t flags_ = 0;
    std::uint16_t qdcount_ = 0;
    std::uint16_t ancount_ = 0;
    std::uint16_t nscount_ = 0;
    std::uint16_t arcount_ = 0;
};


} // namespace
