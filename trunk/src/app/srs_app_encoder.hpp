//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_APP_ENCODER_HPP
#define SRS_APP_ENCODER_HPP

#include <srs_core.hpp>

#include <string>
#include <vector>

#include <srs_app_st.hpp>

class SrsConfDirective;
class SrsRequest;
class SrsPithyPrint;
class SrsFFMPEG;

// The encoder for a stream, may use multiple
// ffmpegs to transcode the specified stream.
class SrsEncoder : public ISrsCoroutineHandler
{
private:
    std::string input_stream_name;
    std::vector<SrsFFMPEG*> ffmpegs;
private:
    SrsCoroutine* trd;
    SrsPithyPrint* pprint;
private:
    // How long the engines outlive their publisher, from the transcode config.
    srs_utime_t hold_;
    // When the hold runs out, or 0 when not holding. The encoder loop kills the engines at its next
    // check after this, up to 3 seconds later.
    srs_utime_t hold_deadline_;
    // Whether on_unpublish is stopping the loop. A second stop while the first is still joining trips
    // the assert in SrsFastCoroutine::stop and aborts the server, so later callers wait for this one.
    bool stopping_;
public:
    SrsEncoder();
    virtual ~SrsEncoder();
public:
    virtual srs_error_t on_publish(SrsRequest* req);
    // Stop the engines now, without a hold. Held engines are killed. Engines with a publisher get the
    // polite stop, a SIGTERM and up to a second each before SIGKILL. A call made while a stop is running
    // waits for that stop instead of starting another.
    virtual void on_unpublish();
    // Keep the engines running for the hold after the publisher leaves, so a publisher that returns
    // within it reuses them. The encoder loop kills them when the hold runs out. With a hold of 0, no
    // engines or a stop already running, this stops them now, as on_unpublish does.
    virtual void hold_on_unpublish();
    // Kill held engines at once when the server quits. Engines with a publisher are left as they are.
    virtual void dispose();
// Interface ISrsReusableThreadHandler.
public:
    virtual srs_error_t cycle();
private:
    virtual srs_error_t do_cycle();
private:
    virtual void clear_engines();
    virtual void kill_engines();
    virtual void fast_kill_engines();
    virtual SrsFFMPEG* at(int index);
    virtual srs_error_t parse_scope_engines(SrsRequest* req);
    virtual srs_error_t parse_ffmpeg(SrsRequest* req, SrsConfDirective* conf);
    virtual srs_error_t initialize_ffmpeg(SrsFFMPEG* ffmpeg, SrsRequest* req, SrsConfDirective* engine);
    virtual void show_encode_log_message();
};

#endif

