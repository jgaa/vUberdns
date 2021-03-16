#pragma once

#include <memory>

#include <numeric>
#include <arpa/inet.h>
#include <boost/concept_check.hpp>
#include <boost/regex.hpp>
#include <boost/utility/string_ref.hpp>

#include "warlib/error_handling.h"

namespace vuberdns {

class LabelHeader
{
public:
    struct LabelHeaderException : public war::ExceptionBase {};
    struct NoLabelsException : public LabelHeaderException {};
    struct IllegalLabelException : public LabelHeaderException {};
    struct IllegalPointereException: public LabelHeaderException {};

    using labels_t = std::vector<boost::string_ref>;

    LabelHeader(const char *buffer, // Buffer for this segment
        std::size_t size, // Buffer-size for this segment (and may be more)
        labels_t& knownLabels, const char *messageBuffer)
    : message_buffer_{messageBuffer}
    {
        if (size < 1) {
            WAR_THROW_T(LabelHeaderException, "Label-length underflow");
        }

        const char *end = buffer + size;
        const char *p = buffer;
        bool is_traversing_pointer = false;
        while(true) {
            // *p is the length-field in the next label.
            if (*p == 0) {
                ++size_;
                if (!is_traversing_pointer) {
                    ++buffer_size_;
                }
                ValidateSize();

                // Root label. We are basically done
                if (names_.empty()) {
                    WAR_THROW_T(NoLabelsException, "No labels found in header");
                }
                break;
            }
            const size_t len = static_cast<uint8_t>(*p);
            if (len < 64) {
                // Normal label with the name as bytes

                const char *label_end = ++p + len;
                if (label_end > end) {
                    WAR_THROW_T(LabelHeaderException,
                        "Buffer underflow in label (potential hostile request)");
                }

                size_ += len;
                ValidateSize();

                if (!is_traversing_pointer) {
                    buffer_size_ += len + 1;
                }

                boost::string_ref label{p, len};

                if (!is_traversing_pointer) {
                    // If we are traversing a pointer, the label is already known.
                    knownLabels.push_back(label);
                }
                names_.push_back(label);

                // Validate the name
                static const boost::regex pat{ R"([a-z0-9\.\-_]+)",
                    boost::regex::icase | boost::regex::optimize};
                if (!boost::regex_match(p, p + len, pat)) {
                    WAR_THROW_T(IllegalLabelException, "Invalid char in label");
                }

                WAR_ASSERT(p < label_end);
                p = label_end; // Points to the first byte in the next label
            } else if ((len & 0xC0) == 0xC0) {
                // Compressed label 2 byte header
                if ((p + 1) >= end) {
                    WAR_THROW_T(LabelHeaderException,
                       "Buffer underflow in label (potential hostile request");
                }

                size_ += 2;
                ValidateSize();

                if (!is_traversing_pointer) {
                    buffer_size_ += 2;
                }

                const size_t pointer = *reinterpret_cast<const uint16_t *>(p) & ~0xC000;
                names_.push_back(FindPointer(pointer, knownLabels));

                // Point to the item after the resolved pointer.
                is_traversing_pointer = true;
                p = names_.back().end();

            } else {
                WAR_THROW_T(LabelHeaderException, "Invalid label length!");
            }
        }
    }

    size_t GetSize() const { return buffer_size_; }

    /*! Return the calculated size of the labels. Max 255.

        TODO: Check if this includes the size header of each label.
     */
    size_t GetLabelSize() const { return size_; }

    /*! Return a normal domain name as a string, starting with the first label */
    std::string GetDomainName() const {
        static const std::string empty;
        static const std::string dot{"."};
        std::ostringstream out;
        int cnt = 0;
        for(auto & label : names_) {
            out << (++cnt == 1 ? empty : dot) << label;
        }

        return out.str();
    }

    const labels_t& GetLabels() const { return names_; }

private:
    boost::string_ref FindPointer(uint16_t pointer, const labels_t& labels)
    {
        // The pointer is an offset into the message buffer
        const char *key = message_buffer_ + pointer;
        for(const auto& v : labels) {
            if (v.begin() == key) {
                return v;
            }
        }

        WAR_THROW_T(IllegalPointereException,
            "Invalid label pointer (potential hostile request)!");
    }

    void ValidateSize() {
        if (size_ > 255) {
            WAR_THROW_T(LabelHeaderException, "The labels exeeds the 255 bytes limit");
        }
    }

    labels_t names_;
    std::size_t size_ = 0;
    std::size_t buffer_size_ = 0; // Bytes used by the labes in /this/ buffer
    const char *message_buffer_;
};


} // namespace
