#pragma once

#include "vudnslib/AnswerBase.h"

namespace vuberdns {

class RdataMx : public AnswerBase
{
public:
    RdataMx(std::uint16_t namePtr, const std::string& fqdn, std::uint16_t pri,
            existing_labels_t& existingLabels)
    : AnswerBase(namePtr, TYPE_MX, 0, existingLabels ),
    fqdn_(fqdn), pri_{pri}
    {
    }

    void Write(buffer_t& buffer) override {
        WriteHeader(buffer);

        // Write priority
        buffer.resize(buffer.size() + 2);
        *reinterpret_cast<uint16_t *>(&buffer[buffer.size() -2]) = htons(pri_);
        // Write the naame of the mail-server

        rdlength_ = 2 + WriteDomainName(buffer, fqdn_, nullptr);
        size_ += rdlength_;
        WriteRdlength(buffer);
    }
private:
    const std::string fqdn_;
    const std::uint16_t pri_;

};

}
