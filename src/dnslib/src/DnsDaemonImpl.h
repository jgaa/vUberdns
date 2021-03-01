#pragma once

#include <vector>

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>

#include <warlib/WarLog.h>
#include <warlib/error_handling.h>
#include <warlib/boost_ptree_helper.h>

#include "vudnslib/DnsDaemon.h"
#include "vudnslib/RequestStats.h"
#include "vudnslib/MessageHeader.h"
#include "vudnslib/LabelHeader.h"
#include "vudnslib/Question.h"
#include "vudnslib/AnswerBase.h"
#include "vudnslib/RdataA.h"
#include "vudnslib/RdataAaaa.h"
#include "vudnslib/RdataCname.h"
#include "vudnslib/RdataMx.h"
#include "vudnslib/RdataNs.h"
#include "vudnslib/RdataSoa.h"
#include "vudnslib/RdataTxt.h"
#include "vudnslib/Zone.h"
#include "vudnslib/ZoneMgr.h"

namespace vuberdns {

struct RequestStats;
class MessageHeader;
class Question;
class Zone;

class DnsDaemonImpl : public DnsDaemon
{
public:
    struct InvalidQuery : public war::ExceptionBase {};
    struct Refused : public war::ExceptionBase {};
    struct UnimplementedQuery : public war::ExceptionBase {};
    struct UnknownDomain : public war::ExceptionBase {};
    struct UnknownSubDomain : public war::ExceptionBase {};
    struct NoSoaRecord : public war::ExceptionBase {};
    struct Truncated : public war::ExceptionBase {};
    using buffer_t = std::vector<char>;
    using zones_t = std::vector<Zone::ptr_t>;

    DnsDaemonImpl(war::Threadpool& ioThreadpool, ZoneMgr::ptr_t mgr);

    void StartReceivingUdpAt(std::string host, std::string port) override;

    ZoneMgr& GetZoneMgr() override {
        assert(zone_mgr_);
        return *zone_mgr_;
    }

private:
    // This is run individually for each thread processing DNS queries
    void ScheduleDnsHousekeeping(war::Pipeline& pipeline);
    void UpdateStats();
    void ProcesssQuerey(const char *queryBuffer,
                        size_t queryLen,
                        buffer_t& replyBuffer,
                        RequestStats& requestStats);

    void ProcessQuestions(const char *queryBuffer,
                         size_t queryLen,
                         buffer_t& replyBuffer,
                         const MessageHeader& header);

    void ProcessQuestion(const Question& question, buffer_t& replyBuffer,
                         bool& authoritative, uint16_t& numAnswers,
                         AnswerBase::existing_labels_t& existingLabels,
                         zones_t& authoritativeZones, zones_t& nsZones);

    void AddZone(zones_t& authoritativeZones,
                        const Zone::ptr_t& zone);

     /*! Store the zone in the AUTH section. */
    void ProcessNsAuthZone(const Zone& zone, uint16_t& numAuth,
                           buffer_t& replyBuffer,
                           AnswerBase::existing_labels_t& existingLabels,
                           zones_t& nsZones);

    /*! Store the zone in the AUTH section. */
    void ProcessNsZone(const Zone& zone, uint16_t& numOpt,
                           buffer_t& replyBuffer,
                           AnswerBase::existing_labels_t& existingLabels);

    std::string StoreCname(const Zone& zone, const Question& question,
                           buffer_t& replyBuffer, uint16_t& numAnswers,
                           AnswerBase::existing_labels_t& existingLabels);

    Zone::ptr_t GetSoaZone(const Zone::ptr_t& zone);

    static void Store(AnswerBase& answer, buffer_t& buffer, uint16_t& numAnswers);

    static void CreateErrorReply(MessageHeader::Rcode errCode,
                          const MessageHeader& hdr,
                          buffer_t& replyBuffer);

    war::Threadpool& io_threadpool_;
    ZoneMgr::ptr_t zone_mgr_;
    static constexpr size_t max_query_buffer_ = 512;
    static constexpr size_t max_reply_buffer_ = 512;
    const unsigned housekeeping_interval_in_seconds_{5};
};

} // namespace
