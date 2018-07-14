#pragma once

#include "vudnslib/AnswerBase.h"

namespace vuberdns {

class RdataNs : public AnswerBase
{
public:
    RdataNs(const std::string& domain, const std::string& fdqn, existing_labels_t& existingLabels)
    : AnswerBase(domain, TYPE_NS, 0, existingLabels ),
    fdqn_(fdqn)
    {
    }

    void Write(buffer_t& buffer) override {
        WriteHeader(buffer);

        // Write the nameserver name
        rdlength_ = WriteDomainName(buffer, fdqn_, nullptr);
        size_ += rdlength_;
        WriteRdlength(buffer);
    }
private:
    const std::string fdqn_;
};

}
