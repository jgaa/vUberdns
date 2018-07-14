#pragma once

#include "vudnslib/AnswerBase.h"

namespace vuberdns {

class RdataCname : public AnswerBase
{
public:
    RdataCname(uint16_t namePtr, const std::string& cname, existing_labels_t& existingLabels)
    : AnswerBase(namePtr, TYPE_CNAME, 0, existingLabels ),
    cname_(cname)
    {
    }

    void Write(buffer_t& buffer) override {
        WriteHeader(buffer);

        // Write CNAME
        rdlength_ = WriteDomainName(buffer, cname_, nullptr);
        size_ += rdlength_;
        WriteRdlength(buffer);
    }
private:
    const std::string& cname_;
};


}
