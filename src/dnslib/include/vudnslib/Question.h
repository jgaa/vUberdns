#pragma once

#include "vudnslib/LabelHeader.h"

namespace vuberdns {

// See RFC1035 4.1.2. Question section format
class Question : public LabelHeader {
public:
    struct QuestionHeaderException : public war::ExceptionBase {};

    Question(const char *buffer,
        std::size_t size,
        labels_t& knownLabels,
        const char *messageBuffer,
        uint16_t offsetIntoReplyBuffer)
    : LabelHeader(buffer, size, knownLabels, messageBuffer),
        offset_(offsetIntoReplyBuffer)
    {
        const char *end = buffer + size;
        const char *start = buffer + LabelHeader::GetSize();
        const std::uint16_t *p = reinterpret_cast<const std::uint16_t *>(start);

        if ((start + 4) > end) {
            WAR_THROW_T(QuestionHeaderException, "Buffer underflow");
        }

        qtype_ = ntohs(*p++);
        qclass = ntohs(*p++);
    }

    std::size_t GetSize() const {
        return LabelHeader::GetSize() + 4;
    }

    std::uint16_t GetQtype() const { return qtype_; }
    std::uint16_t GetQclass() const { return qclass; }
    std::uint16_t GetOffset() const { return offset_; }


private:
    std::uint16_t qtype_ = 0;
    std::uint16_t qclass = 0;
    std::uint16_t offset_ = 0;
};


} // namespace
