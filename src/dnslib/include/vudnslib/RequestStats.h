#pragma once

#include "vudnslib/Statistics.h"

namespace vuberdns {
/*! Helper that updates the statistics based on the outcome of the request */
struct RequestStats
{
    enum class State { SUCESS, IGNORE, FAIL, BAD, UNSET };

    RequestStats(Statistics& stats) : stats_{stats} {}

    ~RequestStats() {

        switch(state) {
            case State::SUCESS:
                ++stats_.ok_requests;
                break;
            case State::IGNORE:
                // Do not update
                break;
            case State::FAIL:
            case State::UNSET:
                ++stats_.failed_requests;
                break;
            case State::BAD:
                ++stats_.bad_requests;
                break;
        }
    }

    State state {State::UNSET};
private:
    Statistics& stats_;
};

} // namespace
