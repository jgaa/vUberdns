#pragma once

#include "vudnslib/AnswerBase.h"

namespace vuberdns {

class RdataAaaa : public AnswerBase
{
public:
    RdataAaaa(uint16_t namePtr,
           const boost::asio::ip::address_v6& ip, existing_labels_t& existingLabels)
    : AnswerBase(namePtr, TYPE_AAAA, 16 /* IPv6 length */, existingLabels),
      ip_(ip)
    {
    }

    RdataAaaa(const std::string& domain,
           const boost::asio::ip::address_v6& ip, existing_labels_t& existingLabels)
    : AnswerBase(domain, TYPE_AAAA, 16 /* IPv6 length */, existingLabels),
    ip_(ip)
    {
    }

    void Write(buffer_t& buffer) override {
        WriteHeader(buffer);
        size_ += 16; //Finalized: GetSize() will now work

        // Copy IP address in network byte order
        const auto address = ip_.to_bytes();
        std::copy(address.begin(), address.end(), std::back_inserter(buffer));
    }
private:
    const boost::asio::ip::address_v6 ip_;
};


}

