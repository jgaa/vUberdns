#pragma once

#include "vudnslib/Zone.h"
#include "vudnslib/AnswerBase.h"

namespace vuberdns {

class RdataSoa : public AnswerBase
{
public:
    RdataSoa(const std::string& domain, const Zone::ptr_t& zone, existing_labels_t& existingLabels)
    : AnswerBase(domain, TYPE_SOA, 0, existingLabels ),
    zone_{zone}
    {
    }

    RdataSoa(uint16_t namePtr, const Zone::ptr_t& zone, existing_labels_t& existingLabels)
    : AnswerBase(namePtr, TYPE_SOA, 0, existingLabels ),
    zone_{zone}
    {
    }

    void Write(buffer_t& buffer) override {
        WriteHeader(buffer);
        const size_t orig_size = buffer.size();

        auto soa = zone_->GetSoa();

        // Write MNAME
        if (soa.mname) {
            WriteDomainName(buffer, soa.mname->fqdn, zone_);
        }

        // Write RNAME
        WriteDomainName(buffer, soa.GetRname(), zone_);

        // Write numerical fields
        const std::size_t num_segment_lenght = 4 * 5;
        buffer.resize(buffer.size() + num_segment_lenght);
        char *last_segment = &buffer[buffer.size() - num_segment_lenght];
        uint32_t *p = reinterpret_cast<uint32_t *>(last_segment);

        *p++ = htonl(soa.serial);
        *p++ = htonl(soa.refresh);
        *p++ = htonl(soa.retry);
        *p++ = htonl(soa.expire);
        *p++ = htonl(soa.minimum);

        rdlength_ = buffer.size() - orig_size;
        size_ += rdlength_;
        WriteRdlength(buffer);
    }

private:
    // NB!
    const Zone::ptr_t& zone_;
};

}
