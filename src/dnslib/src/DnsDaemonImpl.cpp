
#include <warlib/asio.h>

#include "vudnslib/Statistics.h"
#include "vudnslib/AnswerBase.h"
#include "vudnslib/type_values.h"

#include "DnsDaemonImpl.h"


using namespace war;
using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace {

int to_int(const udp::socket& sck) {
    return static_cast<int>(const_cast<udp::socket&>(sck).native_handle());
}

std::ostream& operator << (std::ostream& out, const udp::socket& v) {
    return out << "{ socket " << to_int(v) << " }";
}

std::ostream& operator << (std::ostream& out,
                           const std::vector<boost::string_ref>& v)
{
    bool virgin = true;
    for(const auto s : v) {

        if (virgin)
            virgin = false;
        else
            out << '/';

        out << s;
    }

    return out;
}

} // anonymous namespace


namespace vuberdns {

namespace {
thread_local Statistics thd_stats_dns_;

} // anonymous namespace

DnsDaemonImpl::DnsDaemonImpl(war::Threadpool& ioThreadpool, ZoneMgr::ptr_t mgr)
: io_threadpool_(ioThreadpool), zone_mgr_{move(mgr)} {
    LOG_DEBUG_FN << "Creating dnsd object";
}

void DnsDaemonImpl::StartReceivingUdpAt(std::string host, std::string port) {
    auto& pipeline = io_threadpool_.GetPipeline(0);
    auto& ios = pipeline.GetIoService();

    LOG_DEBUG_FN << "Preparing to listen to: " <<
    log::Esc(host) << " on UDP port " << log::Esc(port);

    udp::resolver resolver(ios);
    auto endpoint = resolver.resolve({host, port});
    udp::resolver::iterator end;
    for(; endpoint != end; ++endpoint) {
        udp::endpoint ep = endpoint->endpoint();
        LOG_NOTICE << "Adding dnsd endpoint: " << ep;

        auto& dns_pipeline = io_threadpool_.GetAnyPipeline();

        // Start receiving incoming requests
        boost::asio::spawn(
            dns_pipeline.GetIoService(), [ep,&dns_pipeline,&ios,this]
            (boost::asio::yield_context yield) {

            // Assign the sockets round-robin to pipelines
            udp::socket socket(dns_pipeline.GetIoService());
            socket.open(ep.protocol());
            socket.set_option(boost::asio::socket_base::reuse_address(true));
            socket.bind(ep);
            buffer_t query_buffer(max_query_buffer_);
            boost::system::error_code ec;
            buffer_t reply_buffer(max_reply_buffer_);
            udp::endpoint sender_endpoint;
            ScheduleDnsHousekeeping(dns_pipeline);

            // Loop over incoming requests
            for (;socket.is_open();) {
                const size_t bytes_received = socket.async_receive_from(
                    boost::asio::buffer(query_buffer),
                    sender_endpoint,
                    yield);

                thd_stats_dns_.bytes_received += bytes_received;
                RequestStats request_stats(thd_stats_dns_);

                if (!ec) {
                    ++thd_stats_dns_.udp_connections;
                    LOG_TRACE2_FN << "Incoming Query: "
                        << socket << ' '
                        << socket.local_endpoint()
                        << " <-- "
                        << sender_endpoint
                        << " of " << bytes_received << "  bytes.";

                    try {

                        ProcesssQuerey(&query_buffer[0],
                                        bytes_received,
                                        reply_buffer,
                                        request_stats);

                    } catch(const UnknownDomain&) {
                        ++thd_stats_dns_.unknown_names;
                        // Do not reply
                        continue;
                    } WAR_CATCH_ERROR;

                    if (socket.is_open() && !reply_buffer.empty()) {
                        LOG_TRACE3_FN << "Replying " << reply_buffer.size()
                            << " bytes to " << socket;

                        socket.async_send_to(
                            boost::asio::buffer(reply_buffer),
                            sender_endpoint,
                            yield);

                        if (!ec) {
                            request_stats.state = RequestStats::State::SUCESS;
                            thd_stats_dns_.bytes_sent += reply_buffer.size();
                        }
                    }
                } else { // error
                    LOG_DEBUG_FN << "Receive error " << ec << " from socket " << socket;
                    ++thd_stats_dns_.failed_udp_connections;;
                }
            }
        });
    } // for endpoints
}

 // This is run individually for each thread processing DNS queries
void DnsDaemonImpl::ScheduleDnsHousekeeping(Pipeline& pipeline)
{
    pipeline.PostWithTimer(task_t{[this, &pipeline]() {
        UpdateStats();
        ScheduleDnsHousekeeping(pipeline);
    }, "Periodic DNS connection-housekeeping"},
    housekeeping_interval_in_seconds_ * 1000);
}

void DnsDaemonImpl::UpdateStats() {

//     StatisticsManager::GetManager().AddStat(
//         StatisticsManager::StatType::DNS, thd_stats_dns_);

    thd_stats_dns_.Reset();
}

void DnsDaemonImpl::ProcesssQuerey(const char *queryBuffer,
                    size_t queryLen,
                    buffer_t& replyBuffer,
                    RequestStats& requestStats)
{
    replyBuffer.resize(0);
    replyBuffer.reserve(max_reply_buffer_);
    MessageHeader header(queryBuffer, queryLen);

    try {
        if (header.GetQr() != 0) {
            WAR_THROW_T(InvalidQuery, "Not a question");
        }

        LOG_TRACE3_FN << "Processing request " << header.GetId();

        switch(header.GetOpcode()) {
            case 0: // Standard query
                ProcessQuestions(queryBuffer, queryLen, replyBuffer, header);
                break;
            case 1: // Inverse query
            case 2: // Server status request
            default:
                CreateErrorReply(MessageHeader::Rcode::NOT_IMPLEMENTED,
                                    header, replyBuffer);
        }
    } catch(const InvalidQuery& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::FORMAT_ERROR,
                            header, replyBuffer);
        requestStats.state = RequestStats::State::BAD;
        return;
    } catch(const UnknownDomain& ex) {
        /* This exception indicate that we are not authoritative for this
            * domain, so we will not reply.
            */
        LOG_TRACE3_FN << "Caught exception for unknown domain: " << ex;
        ++thd_stats_dns_.unknown_names;
        throw;
    } catch(const UnknownSubDomain& ex) {
        LOG_TRACE3_FN << "Caught exception for unknown sub-domain: " << ex;
        // Unknown subdomain for a zone we own. Reply with error.
        CreateErrorReply(MessageHeader::Rcode::NAME_ERROR,
                            header, replyBuffer);
        ++thd_stats_dns_.unknown_names;
        return;
    } catch(const NoSoaRecord& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::SERVER_FAILURE,
                            header, replyBuffer);
        return;
    } catch(const Refused& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::REFUSED,
                            header, replyBuffer);
        return;
    } catch(const LabelHeader::NoLabelsException& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::FORMAT_ERROR,
                            header, replyBuffer);
        requestStats.state = RequestStats::State::BAD;
        return;
    } catch(const LabelHeader::IllegalLabelException& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::FORMAT_ERROR,
                            header, replyBuffer);
        requestStats.state = RequestStats::State::BAD;
        return;
    } catch(const war::ExceptionBase& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::SERVER_FAILURE,
                            header, replyBuffer);
        return;
    } catch(const boost::exception& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::SERVER_FAILURE,
                            header, replyBuffer);
        return;
    } catch(const std::exception& ex) {
        LOG_DEBUG_FN << "Caught exception: " << ex;
        CreateErrorReply(MessageHeader::Rcode::SERVER_FAILURE,
                            header, replyBuffer);
        return;
    }

}

void DnsDaemonImpl::ProcessQuestions(const char *queryBuffer,
                        size_t queryLen,
                        buffer_t& replyBuffer,
                        const MessageHeader& header) {
    size_t bytes_left = queryLen - header.GetSize();
    LabelHeader::labels_t pointers;

    bool truncated = false; // If we run out of buffer space
    bool authoritative = true;
    uint16_t num_questions {0};
    uint16_t num_answers {0};
    uint16_t num_ns {0};
    uint16_t num_opt {0};

    AnswerBase::existing_labels_t existing_labels;
    zones_t authoritative_zones;
    zones_t ns_zones;

    // Reserve buffer-space for the message-header
    MessageHeader reply_hdr(header);
    replyBuffer.resize(reply_hdr.GetSize());

    LOG_TRACE3_FN << "Header: " << header.GetQdCount()
        << " questions ";

    /*! Since we use DNS header compression, we need to know the
        * offsets of the name-labels in the reply buffer. That means
        * that we have to copy all the question's into the buffer
        * before we start adding answers.
        * We therefore do a two-pass loop over the questions; first
        * to parse and copy them, and then to process them.
        */

    std::vector<Question> questions;
    for(uint16_t i = 0; i < header.GetQdCount(); ++i) {
        if (bytes_left == 0) {
            WAR_THROW_T(InvalidQuery, "Buffer underrun preparing next query");
        }

        const char *p = queryBuffer + (queryLen - bytes_left);

        questions.emplace_back<Question>(
            {p, bytes_left, pointers, queryBuffer,
                static_cast<uint16_t>(replyBuffer.size())});
        Question& question = questions.back();

        const auto qclass = question.GetQclass();
        if (qclass != CLASS_IN) {
            WAR_THROW_T(Refused, "Unsupported Qclass");
        }

        // Copy the question to the reply
        const size_t new_len = replyBuffer.size() + question.GetSize();
        if (new_len < max_reply_buffer_) {
            // Copy the original question unmodified
            const size_t len =  question.GetSize();
            std::copy(p, p + len, back_inserter(replyBuffer));
            ++num_questions;
        } else {
            WAR_THROW_T(InvalidQuery, "Too many or too thick questions!");
        }
    }

    // Process the questions and append the answers to the reply-buffer
    // until we are done or the buffer overflows (in which case the latest
    // answer will be rolled back and the truncated flag set).
    try {
        for(const Question& question : questions) {

            LOG_TRACE2_FN << "Request ID " << header.GetId()
                << " asks about type " << question.GetQtype()
                << " class " << question.GetQclass()
                << " regarding " << log::Esc(question.GetDomainName());

            ProcessQuestion(question, replyBuffer, authoritative, num_answers,
                            existing_labels, authoritative_zones, ns_zones);
        }

        // Add NS references in the AUTH section
        for(const auto& z : authoritative_zones) {
            ProcessNsAuthZone(*z, num_ns, replyBuffer, existing_labels, ns_zones);
        }

        // Add IP addresse for NS servers we have named, and have data about
        for(const auto& z : ns_zones) {
            ProcessNsZone(*z, num_opt, replyBuffer, existing_labels);
        }

    } catch (const Truncated& ex) {
        LOG_WARN_FN << "Reply is truncated! " << ex;
        truncated = true;
    }

    // Write out the message-header
    WAR_ASSERT(replyBuffer.size() > reply_hdr.GetSize());
    reply_hdr.SetTc(truncated);
    reply_hdr.SetQr(true);
    reply_hdr.SetRa(false);
    reply_hdr.SetAa(authoritative);
    reply_hdr.SetQdCount(num_questions);
    reply_hdr.SetAnCount(num_answers);
    reply_hdr.SetNsCount(num_ns);
    reply_hdr.SetArCount(num_opt);
    reply_hdr.Write(&replyBuffer[0]);
}

void DnsDaemonImpl::ProcessQuestion(const Question& question, buffer_t& replyBuffer,
                        bool& authoritative, uint16_t& numAnswers,
                        AnswerBase::existing_labels_t& existingLabels,
                        zones_t& authoritativeZones, zones_t& nsZones)
{
    bool is_authoritative_during_lookup {false};
    auto zone = zone_mgr_->Lookup(question.GetLabels(),
                                        &is_authoritative_during_lookup);
    if (!zone) {
        LOG_TRACE1_FN << "Failed to lookup " << log::Esc(question.GetDomainName());

        if (is_authoritative_during_lookup) {
            WAR_THROW_T(UnknownSubDomain, "Unknown sub-domain");
        }

        WAR_THROW_T(UnknownDomain, "Unknown domain");
    }

    // If any of the questions regards a zone where we are not authoritative,
    // we do not set the authoritative flag in the message header.
    const auto soa_zone = GetSoaZone(zone);
    if (!soa_zone || !soa_zone->authorative()) {
        authoritative = false;
    }

    const auto qtype = question.GetQtype();
    if (qtype == TYPE_A || qtype == TYPE_AAAA || qtype == QTYPE_ALL) {
        if (zone->HaveCname()) {
            // This is a CNAME node
            WAR_ASSERT(zone->a.empty());
            std::string full_cname = StoreCname(*zone, question, replyBuffer,
                                                numAnswers, existingLabels);

            if (qtype == TYPE_A) {
                // Add the A records for the cname if available
                zone = zone_mgr_->Lookup(AnswerBase::ToFraments(full_cname));
                if (zone) {
                    if (auto a = zone->a()) {
                        for(const auto& ip : *a) {
                            RdataA a_answer{full_cname, ip, existingLabels};
                            Store(a_answer, replyBuffer, numAnswers);
                        }
                    }
                    if (auto aaaa = zone->aaaa()) {
                        for(const auto& ip : *aaaa) {
                            RdataAaaa a_answer{full_cname, ip, existingLabels};
                            Store(a_answer, replyBuffer, numAnswers);
                        }
                    }
                }
            }
        } else {
            if (qtype == TYPE_A) {
                if (auto a = zone->a()) {
                    for(const auto& ip : *a) {
                        RdataA a_answer{question.GetOffset(), ip, existingLabels};
                        Store(a_answer, replyBuffer, numAnswers);
                    }
                }
            } else if (qtype == TYPE_AAAA) {
                if (auto aaaa = zone->aaaa()) {
                    for(const auto& ip : *aaaa) {
                        RdataAaaa aaaa_answer{question.GetOffset(), ip, existingLabels};
                        Store(aaaa_answer, replyBuffer, numAnswers);
                    }
                }
            }
        }
    } else if (qtype == TYPE_CNAME) {
        if (zone->HaveCname()) {
            StoreCname(*zone, question, replyBuffer, numAnswers, existingLabels);
        }
    } else if (qtype == TYPE_SOA) {
        if (!soa_zone) {
            WAR_THROW_T(NoSoaRecord, "No SOA record found");
        }
        RdataSoa soa_answer(question.GetOffset(), soa_zone, existingLabels);
        Store(soa_answer, replyBuffer, numAnswers);
        AddZone(authoritativeZones, soa_zone);
    }
    else if (qtype == TYPE_NS) {
        AddZone(authoritativeZones, soa_zone);
    }
    else if (qtype == TYPE_MX) {
        if (auto mxlist = zone->mx()) {
            for(const auto mx : *mxlist) {
                const std::string fdqn = mx.GetFdqn();

                RdataMx mx_answer(question.GetOffset(), fdqn, mx.priority, existingLabels);
                Store(mx_answer, replyBuffer, numAnswers);

                AddZone(nsZones, zone_mgr_->Lookup(
                    AnswerBase::ToFraments(fdqn)));
            }
        }
    }
    if (qtype == TYPE_TXT || qtype == QTYPE_ALL) {
        if (zone->HaveTxt()) {
            RdataTxt txt_answer(question.GetOffset(), zone->txt(), existingLabels);
            Store(txt_answer, replyBuffer, numAnswers);
        }
    }
    if (qtype == QTYPE_ALL) {
        AddZone(authoritativeZones, soa_zone);
    }
}

Zone::ptr_t DnsDaemonImpl::GetSoaZone(const Zone::ptr_t& zone) {
    Zone::ptr_t z;

    for(z = zone; z && !z->soa(); z = zone->parent())
        ;

    return z;
}

void DnsDaemonImpl::AddZone(zones_t& authoritativeZones, const Zone::ptr_t& zone)
{
    if (!zone)
        return; // Not an error

    if (std::find(authoritativeZones.begin(), authoritativeZones.end(), zone)
        == authoritativeZones.end()) {
        authoritativeZones.push_back(zone);
    }
}

/*! Store the zone in the AUTH section. */
void DnsDaemonImpl::ProcessNsAuthZone(const Zone& zone, uint16_t& numAuth,
                        buffer_t& replyBuffer,
                        AnswerBase::existing_labels_t& existingLabels,
                        zones_t& nsZones)
{
    // Store Name Servers
    if (auto nslist = zone.ns()) {
        for(const auto ns : *nslist) {
            const string nameserver_fdqn = ns.GetFdqn();
            RdataNs auth_ns(zone.GetDomainName(), nameserver_fdqn, existingLabels);
            Store(auth_ns, replyBuffer, numAuth);

            // FIXME:
            AddZone(nsZones, zone_mgr_->Lookup(
                AnswerBase::ToFraments(nameserver_fdqn)));
        }
    }
}

/*! Store the zone in the AUTH section. */
void DnsDaemonImpl::ProcessNsZone(const Zone& zone, uint16_t& numOpt,
                        buffer_t& replyBuffer,
                        AnswerBase::existing_labels_t& existingLabels)
{
    if (auto a = zone.a()) {
        for(const auto& ip : *a) {
            RdataA a_answer(zone.GetDomainName(), ip, existingLabels);
            Store(a_answer, replyBuffer, numOpt);
        }
    }

    if (auto aaaa = zone.aaaa()) {
        for(const auto& ip : *aaaa) {
            RdataAaaa aaaa_answer(zone.GetDomainName(), ip, existingLabels);
            Store(aaaa_answer, replyBuffer, numOpt);
        }
    }
}

std::string DnsDaemonImpl::StoreCname(const Zone& zone, const Question& question,
                        buffer_t& replyBuffer, uint16_t& numAnswers,
                        AnswerBase::existing_labels_t& existingLabels)
{
    WAR_ASSERT(zone->HaveCname());

    std::string full_cname = zone.cname();
    if (full_cname.back() != '.') {
        if (auto parent = zone.parent()) {
            full_cname += '.';
            full_cname += parent->GetDomainName();
        } else {
            LOG_WARN_FN << "No parent zone in CNAME scope :"
            << log::Esc(zone.GetDomainName());
        }
    }
    RdataCname cname_answer(question.GetOffset(), full_cname, existingLabels);
    Store(cname_answer, replyBuffer, numAnswers);

    return full_cname;
}

void DnsDaemonImpl::Store(AnswerBase& answer, buffer_t& buffer,
                          uint16_t& numAnswers)
{
    answer.Write(buffer);
    if (buffer.size() > max_reply_buffer_) {
        answer.Revert(buffer);
        WAR_THROW_T(Truncated, "Truncated reply");
    }
    ++numAnswers;
}

void DnsDaemonImpl::CreateErrorReply(MessageHeader::Rcode errCode,
                        const MessageHeader& hdr,
                        buffer_t& replyBuffer)
{
    MessageHeader reply_hdr(hdr);
    replyBuffer.resize(reply_hdr.GetSize());
    reply_hdr.SetQr(true);
    reply_hdr.SetRcode(errCode);
    reply_hdr.ResetAllCounters();
    reply_hdr.Write(&replyBuffer[0]);

    LOG_DEBUG_FN << "Query with ID " << hdr.GetId()
        << " failed with error " << static_cast<int>(errCode);
}



} // namespace
