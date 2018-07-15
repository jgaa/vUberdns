#pragma once

#include "vudnslib/AnswerBase.h"

namespace vuberdns {

class RdataTxt : public AnswerBase
{
public:
    RdataTxt(uint16_t namePtr, const std::string& txt, existing_labels_t& existingLabels)
    : AnswerBase(namePtr, TYPE_TXT, 0, existingLabels ),
    txt_(txt)
    {
    }

    void Write(buffer_t& buffer) override {
        WriteHeader(buffer);

        // Write Txt
        rdlength_ = WriteData(buffer, txt_);
        size_ += rdlength_;
        WriteRdlength(buffer);
    }
private:
    const std::string txt_;
};


}
