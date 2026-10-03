//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_UTEST_APP_HPP
#define SRS_UTEST_APP_HPP

/*
#include <srs_utest_app.hpp>
*/
#include <srs_utest.hpp>

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <srs_kernel_log.hpp>
#include <srs_app_conn.hpp>
#include <srs_app_st.hpp>
#include <srs_app_statistic.hpp>
#include <srs_utest_config.hpp>

// The helpers below serve the takeover tests of every protocol, so they live where every build compiles them.

// Points _srs_config at a test config for the life of this object.
class MockTakeoverConfig
{
public:
    MockSrsConfig conf;
private:
    SrsConfig* saved_;
public:
    MockTakeoverConfig() {
        saved_ = _srs_config;
        _srs_config = &conf;
    }
    virtual ~MockTakeoverConfig() {
        _srs_config = saved_;
    }
};

// Stands in for the old publisher's connection. When expired it goes after a short delay, as a real connection
// does once its read is interrupted, unless it is told to hang on.
class MockTakeoverPublisher : public ISrsExpire, public ISrsCoroutineHandler
{
public:
    std::string id;
    bool expired;
    bool hangs;
private:
    SrsCoroutine* trd_;
public:
    MockTakeoverPublisher(std::string v, bool hang) {
        id = v;
        expired = false;
        hangs = hang;
        trd_ = new SrsDummyCoroutine();
    }
    virtual ~MockTakeoverPublisher() {
        srs_freep(trd_);
        SrsStatistic::instance()->on_disconnect(id, srs_success);
    }
    virtual void expire() {
        expired = true;
        if (hangs) {
            return;
        }
        leave_soon();
    }
    // Leaves the statistics from its own coroutine after a short delay, as a connection does at the end of its
    // teardown.
    void leave_soon() {
        srs_freep(trd_);
        trd_ = new SrsSTCoroutine("old-publisher", this, _srs_context->get_id());
        srs_error_t err = trd_->start();
        srs_freep(err);
    }
    virtual srs_error_t cycle() {
        srs_usleep(30 * SRS_UTIME_MILLISECONDS);
        SrsStatistic::instance()->on_disconnect(id, srs_success);
        return srs_success;
    }
};

extern SrsRequest* mock_takeover_request(std::string stream);
extern void mock_takeover_publish(MockTakeoverPublisher* old, SrsRequest* req);

// Keeps what is logged while it is installed: the warnings, and every line of every level in order, each led by the
// id of the context that logged it.
class MockTakeoverLog : public ISrsLog
{
public:
    std::vector<std::string> warnings;
    std::vector<std::string> lines;
private:
    ISrsLog* saved_;
public:
    MockTakeoverLog() {
        saved_ = _srs_log;
        _srs_log = this;
    }
    virtual ~MockTakeoverLog() {
        _srs_log = saved_;
    }
    virtual srs_error_t initialize() {
        return srs_success;
    }
    virtual void reopen() {
    }
    virtual void log(SrsLogLevel level, const char* /*tag*/, const SrsContextId& context_id, const char* fmt, va_list args) {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, args);
        lines.push_back(std::string("[") + context_id.c_str() + "] " + buf);
        if (level == SrsLogLevelWarn) {
            warnings.push_back(buf);
        }
    }
    // The number of warnings that contain text.
    int count(std::string text) {
        int n = 0;
        for (int i = 0; i < (int)warnings.size(); i++) {
            if (warnings[i].find(text) != std::string::npos) {
                n++;
            }
        }
        return n;
    }
    // The position of the first line, of any level, that contains text, or -1 when none does.
    int find(std::string text) {
        for (int i = 0; i < (int)lines.size(); i++) {
            if (lines[i].find(text) != std::string::npos) {
                return i;
            }
        }
        return -1;
    }
};

#endif

