#pragma once

#include "vudnslib/AnswerBase.h"

namespace vuberdns {

class RdataA : public AnswerBase
{
public:
    RdataA(uint16_t namePtr,
           const boost::asio::ip::address_v4& ip, existing_labels_t& existingLabels)
    : AnswerBase(namePtr, TYPE_A, 4 /* IPv4 length */, existingLabels),
      ip_(ip)
    {
    }

    RdataA(const std::string& domain,
           const boost::asio::ip::address_v4& ip, existing_labels_t& existingLabels)
    : AnswerBase(domain, TYPE_A, 4 /* IPv4 length */, existingLabels),
    ip_(ip)
    {
    }

    void Write(buffer_t& buffer) override {
        WriteHeader(buffer);
        size_ += 4; //Finalized: GetSize() will now work

        // Copy IP address in network byte order
        const auto address = ip_.to_bytes();
        std::copy(address.begin(), address.end(), std::back_inserter(buffer));
    }
private:
    const boost::asio::ip::address_v4 ip_;
};


}
