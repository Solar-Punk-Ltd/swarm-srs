//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//
#include <srs_utest_app.hpp>

using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_app_fragment.hpp>
#include <srs_app_security.hpp>
#include <srs_app_config.hpp>
#include <srs_app_statistic.hpp>

#include <srs_app_st.hpp>
#include <srs_protocol_conn.hpp>
#include <srs_app_conn.hpp>
#include <srs_app_encoder.hpp>
#include <srs_app_ffmpeg.hpp>
#include <srs_app_process.hpp>
#include <srs_app_source.hpp>
#include <srs_protocol_rtmp_stack.hpp>
#include <srs_core_autofree.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_utest_config.hpp>

#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>

class MockIDResource : public ISrsResource
{
public:
    int id;
    MockIDResource(int v) {
        id = v;
    }
    virtual ~MockIDResource() {
    }
    virtual const SrsContextId& get_id() {
        return _srs_context->get_id();
    }
    virtual std::string desc() {
        return "";
    }
};

class MockStatisticExpire : public ISrsExpire
{
public:
    virtual void expire() {
    }
};

VOID TEST(StatisticTest, GracefulDisconnectsDoNotIncrementErrors)
{
    srs_error_t err = srs_success;

    SrsStatistic stat;
    SrsRequest req;
    req.vhost = "test.vhost";
    req.app = "live";
    req.stream = "livestream";
    MockStatisticExpire conn;

    int graceful_codes[] = {
        ERROR_SOCKET_READ,
        ERROR_SOCKET_READ_FULLY,
        ERROR_SOCKET_WRITE,
        ERROR_SRT_IO,
        ERROR_HTTP_STREAM_EOF,
    };
    int nb_graceful_codes = sizeof(graceful_codes) / sizeof(int);

    for (int i = 0; i < nb_graceful_codes; i++) {
        stringstream ss;
        ss << "client-" << i;
        string client_id = ss.str();
        HELPER_EXPECT_SUCCESS(stat.on_client(client_id, &req, &conn, SrsRtmpConnPlay));

        srs_error_t graceful_close = srs_error_new(graceful_codes[i], "gracefully closed");
        EXPECT_TRUE(srs_is_client_gracefully_close(graceful_close) || srs_is_server_gracefully_close(graceful_close));
        stat.on_disconnect(client_id, graceful_close);
        srs_freep(graceful_close);
    }

    HELPER_EXPECT_SUCCESS(stat.on_client("failed-client", &req, &conn, SrsRtmpConnPlay));
    srs_error_t failure = srs_error_new(ERROR_RTMP_HANDSHAKE, "genuine failure");
    stat.on_disconnect("failed-client", failure);
    srs_freep(failure);

    int64_t send_bytes = 0;
    int64_t recv_bytes = 0;
    int64_t nstreams = 0;
    int64_t nclients = 0;
    int64_t total_nclients = 0;
    int64_t nerrs = 0;
    HELPER_EXPECT_SUCCESS(stat.dumps_metrics(send_bytes, recv_bytes, nstreams, nclients, total_nclients, nerrs));

    EXPECT_EQ(0, nclients);
    EXPECT_EQ(nb_graceful_codes + 1, total_nclients);
    EXPECT_EQ(1, nerrs);
}

VOID TEST(AppResourceManagerTest, FindByFastID)
{
    srs_error_t err = srs_success;

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        m.add_with_fast_id(101, new MockIDResource(1));
        m.add_with_fast_id(102, new MockIDResource(2));
        m.add_with_fast_id(103, new MockIDResource(3));
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(102))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(103))->id);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        MockIDResource* r1 = new MockIDResource(1);
        MockIDResource* r2 = new MockIDResource(2);
        MockIDResource* r3 = new MockIDResource(3);
        m.add_with_fast_id(101, r1);
        m.add_with_fast_id(102, r2);
        m.add_with_fast_id(103, r3);
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(102))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(103))->id);

        m.remove(r2); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(102) == NULL);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        MockIDResource* r1 = new MockIDResource(1);
        MockIDResource* r2 = new MockIDResource(2);
        MockIDResource* r3 = new MockIDResource(3);
        m.add_with_fast_id(1, r1);
        m.add_with_fast_id(100001, r2);
        m.add_with_fast_id(1000001, r3);
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(1))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(100001))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(1000001))->id);

        m.remove(r2); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(100001) == NULL);

        m.remove(r3); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(1000001) == NULL);

        m.remove(r1); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(1) == NULL);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        m.add_with_fast_id(101, new MockIDResource(1));
        m.add_with_fast_id(10101, new MockIDResource(2));
        m.add_with_fast_id(1010101, new MockIDResource(3));
        m.add_with_fast_id(101010101, new MockIDResource(4));
        m.add_with_fast_id(10101010101LL, new MockIDResource(5));
        m.add_with_fast_id(1010101010101LL, new MockIDResource(6));
        m.add_with_fast_id(101010101010101LL, new MockIDResource(7));
        m.add_with_fast_id(10101010101010101LL, new MockIDResource(8));
        m.add_with_fast_id(1010101010101010101ULL, new MockIDResource(9));
        m.add_with_fast_id(11010101010101010101ULL, new MockIDResource(10));
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(10101))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(1010101))->id);
        EXPECT_EQ(4, ((MockIDResource*)m.find_by_fast_id(101010101))->id);
        EXPECT_EQ(5, ((MockIDResource*)m.find_by_fast_id(10101010101LL))->id);
        EXPECT_EQ(6, ((MockIDResource*)m.find_by_fast_id(1010101010101LL))->id);
        EXPECT_EQ(7, ((MockIDResource*)m.find_by_fast_id(101010101010101LL))->id);
        EXPECT_EQ(8, ((MockIDResource*)m.find_by_fast_id(10101010101010101LL))->id);
        EXPECT_EQ(9, ((MockIDResource*)m.find_by_fast_id(1010101010101010101ULL))->id);
        EXPECT_EQ(10, ((MockIDResource*)m.find_by_fast_id(11010101010101010101ULL))->id);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        m.add_with_fast_id(101, new MockIDResource(1));
        m.add_with_fast_id(101, new MockIDResource(4));
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
    }
}

VOID TEST(AppCoroutineTest, Dummy)
{
    SrsDummyCoroutine dc;

    if (true) {
        SrsContextId v = dc.cid();
        EXPECT_TRUE(v.empty());

        srs_error_t err = dc.pull();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);

        err = dc.start();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        dc.stop();

        SrsContextId v = dc.cid();
        EXPECT_TRUE(v.empty());

        srs_error_t err = dc.pull();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);

        err = dc.start();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        dc.interrupt();

        SrsContextId v = dc.cid();
        EXPECT_TRUE(v.empty());

        srs_error_t err = dc.pull();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);

        err = dc.start();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);
    }
}

class MockCoroutineHandler : public ISrsCoroutineHandler {
public:
    SrsSTCoroutine* trd;
    srs_error_t err;
    srs_cond_t running;
    srs_cond_t exited;
    SrsContextId cid;
    // Quit without error.
    bool quit;
public:
    MockCoroutineHandler() : trd(NULL), err(srs_success), quit(false) {
        cid.set_value("0");
        running = srs_cond_new();
        exited = srs_cond_new();
    }
    virtual ~MockCoroutineHandler() {
        srs_cond_destroy(running);
        srs_cond_destroy(exited);
    }
public:
    virtual srs_error_t cycle() {
        srs_error_t r0 = srs_success;

        srs_cond_signal(running);

        // The cid should be generated if empty.
        cid = _srs_context->get_id();

        while (!quit && (r0 = trd->pull()) == srs_success && err == srs_success) {
            srs_usleep(10 * SRS_UTIME_MILLISECONDS);
        }

        srs_cond_signal(exited);

        // The cid might be updated.
        cid = _srs_context->get_id();

        if (err != srs_success) {
            srs_freep(r0);
            return err;
        }

        return r0;
    }
};

VOID TEST(AppCoroutineTest, SetCidOfCoroutine)
{
    srs_error_t err = srs_success;

    MockCoroutineHandler ch;
    SrsSTCoroutine sc("test", &ch);
    ch.trd = &sc;
    EXPECT_TRUE(sc.cid().empty());

    // Start coroutine, which will create the cid.
    HELPER_ASSERT_SUCCESS(sc.start());
    HELPER_ASSERT_SUCCESS(sc.pull());

    srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);
    EXPECT_TRUE(!sc.cid().empty());
    EXPECT_TRUE(!ch.cid.empty());

    // Should be a new cid.
    SrsContextId cid = _srs_context->generate_id();
    EXPECT_TRUE(sc.cid().compare(cid) != 0);
    EXPECT_TRUE(ch.cid.compare(cid) != 0);

    // Set the cid and stop the coroutine.
    sc.set_cid(cid);
    sc.stop();

    // Now the cid should be the new one.
    srs_cond_timedwait(ch.exited, 100 * SRS_UTIME_MILLISECONDS);
    EXPECT_TRUE(sc.cid().compare(cid) == 0);
    EXPECT_TRUE(ch.cid.compare(cid) == 0);
}

VOID TEST(AppCoroutineTest, StartStop)
{
    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;
        EXPECT_TRUE(sc.cid().empty());

        // Thread stop after created.
        sc.stop();

        EXPECT_TRUE(sc.cid().empty());

        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_TERMINATED == srs_error_code(err));
        srs_freep(err);

        // Should never reuse a disposed thread.
        err = sc.start();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_DISPOSED == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;
        EXPECT_TRUE(sc.cid().empty());

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);
        EXPECT_TRUE(!sc.cid().empty());

        // Thread stop after started.
        sc.stop();

        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_INTERRUPED == srs_error_code(err));
        srs_freep(err);

        // Should never reuse a disposed thread.
        err = sc.start();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_DISPOSED == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;
        EXPECT_TRUE(sc.cid().empty());

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // Error when start multiple times.
        srs_error_t err = sc.start();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_STARTED == srs_error_code(err));
        srs_freep(err);

        err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_STARTED == srs_error_code(err));
        srs_freep(err);
    }
}

VOID TEST(AppCoroutineTest, Cycle)
{
    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // Set cycle to error.
        ch.err = srs_error_new(-1, "cycle");

        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);

        // The cycle error should be pulled.
        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(-1 == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsContextId cid;
        SrsSTCoroutine sc("test", &ch, cid.set_value("250"));
        ch.trd = &sc;
        EXPECT_TRUE(!sc.cid().compare(cid));

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // After running, the cid in cycle should equal to the thread.
        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);
        EXPECT_TRUE(!ch.cid.compare(cid));
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);

        // Interrupt thread, set err to interrupted.
        sc.interrupt();

        // Set cycle to error.
        ch.err = srs_error_new(-1, "cycle");

        // When thread terminated, thread will get its error.
        srs_cond_timedwait(ch.exited, 100 * SRS_UTIME_MILLISECONDS);

        // Override the error by cycle error.
        sc.stop();

        // Should be cycle error.
        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(-1 == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // Quit without error.
        ch.quit = true;

        // Wait for thread to done.
        srs_cond_timedwait(ch.exited, 100 * SRS_UTIME_MILLISECONDS);

        // Override the error by cycle error.
        sc.stop();

        // Should be cycle error.
        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success == err);
        srs_freep(err);
    }
}

void* mock_st_thread_create(void *(*/*start*/)(void *arg), void */*arg*/, int /*joinable*/, int /*stack_size*/) {
    return NULL;
}

VOID TEST(AppCoroutineTest, StartThread)
{
    MockCoroutineHandler ch;
    SrsSTCoroutine sc("test", &ch);
    ch.trd = &sc;

    _ST_THREAD_CREATE_PFN ov = _pfn_st_thread_create;
    _pfn_st_thread_create = (_ST_THREAD_CREATE_PFN)mock_st_thread_create;

    srs_error_t err = sc.start();
    _pfn_st_thread_create = ov;

    EXPECT_TRUE(srs_success != err);
    EXPECT_TRUE(ERROR_ST_CREATE_CYCLE_THREAD == srs_error_code(err));
    srs_freep(err);
}

// A worker that ignores interrupts and finishes on its own after a while, like a coroutine that stops child
// processes politely before it returns.
class MockSlowWorker : public ISrsCoroutineHandler
{
public:
    bool done;
public:
    MockSlowWorker() : done(false) {
    }
    virtual srs_error_t cycle() {
        srs_utime_t starttime = srs_update_system_time();
        while (srs_update_system_time() - starttime < 100 * SRS_UTIME_MILLISECONDS) {
            srs_usleep(10 * SRS_UTIME_MILLISECONDS);
        }
        done = true;
        return srs_success;
    }
};

// Stops another coroutine from a coroutine of its own, as a connection's teardown stops its workers.
class MockCoroutineStopper : public ISrsCoroutineHandler
{
public:
    SrsCoroutine* target;
    bool done;
public:
    MockCoroutineStopper(SrsCoroutine* v) : target(v), done(false) {
    }
    virtual srs_error_t cycle() {
        target->stop();
        done = true;
        return srs_success;
    }
};

// A coroutine interrupted while it waits in stop() for another to finish, as when a connection is expired during
// its own teardown, keeps waiting instead of aborting the server.
VOID TEST(AppCoroutineTest, InterruptedStopKeepsWaiting)
{
    srs_error_t err;

    MockSlowWorker worker;
    SrsSTCoroutine wtrd("worker", &worker, _srs_context->get_id());
    HELPER_ASSERT_SUCCESS(wtrd.start());

    MockCoroutineStopper stopper(&wtrd);
    SrsSTCoroutine strd("stopper", &stopper, _srs_context->get_id());
    HELPER_ASSERT_SUCCESS(strd.start());

    // The stopper now waits in st_thread_join for the worker.
    srs_usleep(20 * SRS_UTIME_MILLISECONDS);
    strd.interrupt();

    for (int i = 0; i < 100 && !stopper.done; i++) {
        srs_usleep(10 * SRS_UTIME_MILLISECONDS);
    }
    EXPECT_TRUE(stopper.done);
    EXPECT_TRUE(worker.done);
}

VOID TEST(AppFragmentTest, CheckDuration)
{
	if (true) {
		SrsFragment frg;
		EXPECT_EQ(-1, frg.start_dts);
		EXPECT_EQ(0, frg.dur);
		EXPECT_FALSE(frg.sequence_header);
	}

	if (true) {
		SrsFragment frg;

		frg.append(0);
		EXPECT_EQ(0, frg.duration());

		frg.append(10);
		EXPECT_EQ(10 * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(99);
		EXPECT_EQ(99 * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(0x7fffffffLL);
		EXPECT_EQ(0x7fffffffLL * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(0xffffffffLL);
		EXPECT_EQ(0xffffffffLL * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(0x20c49ba5e353f7LL);
		EXPECT_EQ(0x20c49ba5e353f7LL * SRS_UTIME_MILLISECONDS, frg.duration());
	}

	if (true) {
		SrsFragment frg;

		frg.append(0);
		EXPECT_EQ(0, frg.duration());

		frg.append(0x7fffffffffffffffLL);
		EXPECT_EQ(0, frg.duration());
	}

	if (true) {
		SrsFragment frg;

		frg.append(100);
		EXPECT_EQ(0, frg.duration());

		frg.append(10);
		EXPECT_EQ(0, frg.duration());

		frg.append(100);
		EXPECT_EQ(90 * SRS_UTIME_MILLISECONDS, frg.duration());
	}

	if (true) {
		SrsFragment frg;

		frg.append(-10);
		EXPECT_EQ(0, frg.duration());

		frg.append(-5);
		EXPECT_EQ(0, frg.duration());

		frg.append(10);
		EXPECT_EQ(10 * SRS_UTIME_MILLISECONDS, frg.duration());
	}
}

VOID TEST(AppSecurity, CheckSecurity)
{
    srs_error_t err;

    // Deny if no rules.
    if (true) {
        SrsSecurity sec; SrsRequest rr;
        HELPER_EXPECT_FAILED(sec.do_check(NULL, SrsRtmpConnUnknown, "", &rr));
    }

    // Deny if not allowed.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnUnknown, "", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("others"); rules.get_or_create("any");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnUnknown, "", &rr));
    }

    // Deny by rule.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "11.12.13.14");
        if (true) {
            SrsConfDirective* d = new SrsConfDirective();
            d->name = "deny";
            d->args.push_back("play");
            d->args.push_back("12.13.14.15");
            rules.directives.push_back(d);
        }
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "11.12.13.14");
        if (true) {
            SrsConfDirective* d = new SrsConfDirective();
            d->name = "deny";
            d->args.push_back("play");
            d->args.push_back("12.13.14.15");
            rules.directives.push_back(d);
        }
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFlashPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFlashPublish, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPublish, "11.12.13.14", &rr));
    }

    // Allowed if not denied.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnUnknown, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFlashPublish, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }

    // Allowed by rule.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFlashPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }

    // Allowed if not denied.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }

    // Denied if not allowd.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnUnknown, "11.12.13.14", &rr));
    }

    // Denied if dup.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        rules.get_or_create("deny", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        rules.get_or_create("deny", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "11.12.13.14", &rr));
    }

    // SRS apply the following simple strategies one by one:
    //       1. allow all if security disabled.
    //       2. default to deny all when security enabled.
    //       3. allow if matches allow strategy.
    //       4. deny if matches deny strategy.
}


// Stands in for ffmpeg in the encoder tests. Like an ffmpeg whose input has stalled, it ignores SIGINT and
// SIGTERM, so only SIGKILL stops it. It lives beside the test binary rather than at a shared path.
extern const char* _srs_binary;

static std::string mock_encoder_ffmpeg()
{
    return srs_path_dirname(_srs_binary ? _srs_binary : "./srs_utest") + "/srs-utest-encoder-ffmpeg.sh";
}

static std::string mock_encoder_config(std::string hold)
{
    return std::string(_MIN_OK_CONF) + "ff_log_dir /dev/null; vhost test.hold { transcode { enabled on; "
        "ffmpeg " + mock_encoder_ffmpeg() + "; " + hold +
        " engine a { enabled on; vcodec copy; acodec copy; output rtmp://127.0.0.1:[port]/[app]/[stream]_[engine]?vhost=abr.test; }"
        " engine b { enabled on; vcodec copy; acodec copy; output rtmp://127.0.0.1:[port]/[app]/[stream]_[engine]?vhost=abr.test; }"
        " } }";
}

VOID TEST(AppEncoderTest, UnpublishHoldConfig)
{
    srs_error_t err;

    if (true) {
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(mock_encoder_config("")));
        SrsConfDirective* transcode = conf.get_transcode("test.hold", "");
        ASSERT_TRUE(transcode != NULL);
        EXPECT_EQ(60 * SRS_UTIME_SECONDS, conf.get_transcode_unpublish_hold(transcode));
    }

    if (true) {
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(mock_encoder_config("unpublish_hold 7;")));
        SrsConfDirective* transcode = conf.get_transcode("test.hold", "");
        ASSERT_TRUE(transcode != NULL);
        EXPECT_EQ(7 * SRS_UTIME_SECONDS, conf.get_transcode_unpublish_hold(transcode));
    }

    if (true) {
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(mock_encoder_config("unpublish_hold 0;")));
        SrsConfDirective* transcode = conf.get_transcode("test.hold", "");
        ASSERT_TRUE(transcode != NULL);
        EXPECT_EQ(0, conf.get_transcode_unpublish_hold(transcode));
    }
}

// The tests below fork a process from a coroutine, which Cygwin's fork cannot do.
#ifndef SRS_CYGWIN64

// Returns whether the mock was written and made executable. It exits on its own once the test binary is gone, so
// an interrupted run leaves nothing behind.
static bool mock_encoder_write_ffmpeg()
{
    std::string path = mock_encoder_ffmpeg();
    FILE* f = fopen(path.c_str(), "w");
    if (!f) {
        return false;
    }
    int written = fprintf(f, "#!/bin/sh\ntrap '' INT TERM\nwhile kill -0 $PPID 2>/dev/null; do sleep 1; done\n");
    if (fclose(f) != 0 || written <= 0) {
        return false;
    }
    return chmod(path.c_str(), 0755) == 0;
}

// Points _srs_config at a test config for the life of this object.
class MockEncoderConfig
{
public:
    MockSrsConfig conf;
    // Each test asserts this, because a gtest assertion cannot live in a constructor.
    bool ffmpeg_ready;
private:
    SrsConfig* saved_;
public:
    MockEncoderConfig() {
        saved_ = _srs_config;
        _srs_config = &conf;
        ffmpeg_ready = mock_encoder_write_ffmpeg();
    }
    virtual ~MockEncoderConfig() {
        _srs_config = saved_;
    }
};

static SrsRequest* mock_encoder_request()
{
    SrsRequest* req = new SrsRequest();
    req->vhost = "test.hold";
    req->app = "live";
    req->stream = "livestream";
    req->port = 1935;
    return req;
}

static std::vector<int> mock_encoder_pids(SrsEncoder* e)
{
    std::vector<int> pids;
    for (int i = 0; i < (int)e->ffmpegs.size(); i++) {
        pids.push_back(e->ffmpegs[i]->process->get_pid());
    }
    return pids;
}

static bool mock_encoder_pid_alive(int pid)
{
    return pid > 0 && kill(pid, 0) == 0;
}

// Waits up to about a second for every engine to count as started, and returns without failing, so a slow
// start shows up in the caller's own checks.
static void mock_encoder_wait_started(SrsEncoder* e)
{
    for (int i = 0; i < 100; i++) {
        // The pid is set at fork, but the process counts as started only when start() returns.
        bool started = !e->ffmpegs.empty();
        for (int j = 0; j < (int)e->ffmpegs.size(); j++) {
            started = started && e->ffmpegs[j]->process->started();
        }
        if (started) {
            return;
        }
        srs_usleep(10 * SRS_UTIME_MILLISECONDS);
    }
}

// A publisher returning after the hold has run out, but before the encoder loop's next check has killed the engines,
// still gets the same engines. That is the "up to 3 seconds later" full.conf documents.
VOID TEST(AppEncoderTest, ReturnAfterDeadlineBeforeTheLoopKeepsEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    e.hold_on_unpublish();
    e.hold_deadline_ = srs_update_system_time() - 1;

    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    EXPECT_EQ(0, e.hold_deadline_);
    EXPECT_TRUE(pids == mock_encoder_pids(&e));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[1]));

    e.hold_on_unpublish();
    e.dispose();
}

// A publisher that returns within the hold gets the same engines, still running, and no second set.
VOID TEST(AppEncoderTest, HoldThenReturnKeepsEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<SrsFFMPEG*> engines = e.ffmpegs;
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    e.hold_on_unpublish();
    EXPECT_TRUE(e.hold_deadline_ > 0);
    ASSERT_EQ(2, (int)e.ffmpegs.size());
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[1]));

    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    EXPECT_EQ(0, e.hold_deadline_);
    EXPECT_TRUE(engines == e.ffmpegs);
    EXPECT_TRUE(pids == mock_encoder_pids(&e));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[1]));

    e.on_unpublish();
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
}

// Nobody returns, so the encoder loop kills the engines at its first check after the hold runs out.
VOID TEST(AppEncoderTest, HoldThenExpiryStopsEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 1;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    e.hold_on_unpublish();
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));

    // The loop looks every 3s, so the engines go between 1s and about 4s from now.
    for (int i = 0; i < 80 && !e.ffmpegs.empty(); i++) {
        srs_usleep(100 * SRS_UTIME_MILLISECONDS);
    }
    EXPECT_TRUE(e.ffmpegs.empty());
    EXPECT_EQ(0, e.hold_deadline_);
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));

    // A publisher after the expiry gets a fresh set.
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> fresh = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)fresh.size());
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[1]));
    e.on_unpublish();
}

// A hold of 0 is the behaviour without the hold: the engines stop when the publisher leaves.
VOID TEST(AppEncoderTest, HoldZeroStopsWhenThePublisherLeaves)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 0;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[1]));

    e.hold_on_unpublish();
    EXPECT_EQ(0, e.hold_deadline_);
    EXPECT_TRUE(e.ffmpegs.empty());
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
}

// A source destroyed during the hold kills its engines at once.
VOID TEST(AppEncoderTest, DestroyDuringHoldStopsEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder* e = new SrsEncoder();
    HELPER_ASSERT_SUCCESS(e->on_publish(req.get()));
    mock_encoder_wait_started(e);
    std::vector<int> pids = mock_encoder_pids(e);
    ASSERT_EQ(2, (int)pids.size());

    e->hold_on_unpublish();
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));

    srs_freep(e);
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
}

// A transcode reload during the hold, which calls on_unpublish, kills the held engines at once, and the next
// publish starts a fresh set rather than resuming the hold.
VOID TEST(AppEncoderTest, ReloadDuringHoldRestartsEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    e.hold_on_unpublish();
    e.on_unpublish();
    EXPECT_EQ(0, e.hold_deadline_);
    EXPECT_TRUE(e.ffmpegs.empty());
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));

    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> fresh = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)fresh.size());
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[1]));
    EXPECT_TRUE(fresh[0] != pids[0]);
    e.on_unpublish();
}

// An engine that dies while held is restarted by the encoder loop, which keeps running through the hold. An
// engine whose idle output SRS cut during the hold dies on its first write after the publisher returns, so the
// test kills one during the hold and checks the restart after the return.
VOID TEST(AppEncoderTest, HoldKeepsRestartingDeadEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    e.hold_on_unpublish();
    kill(pids[0], SIGKILL);
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));

    // The loop reaps the dead engine on one pass and starts it on the next, 3s apart.
    int restarted = -1;
    for (int i = 0; i < 80; i++) {
        restarted = e.ffmpegs[0]->process->get_pid();
        if (restarted != pids[0] && mock_encoder_pid_alive(restarted)) {
            break;
        }
        srs_usleep(100 * SRS_UTIME_MILLISECONDS);
    }
    EXPECT_TRUE(restarted != pids[0]);
    EXPECT_TRUE(mock_encoder_pid_alive(restarted));
    EXPECT_EQ(pids[1], e.ffmpegs[1]->process->get_pid());
    e.on_unpublish();
}

// kill_engines, which the hold expiry calls, kills every engine at once instead of giving each one the polite
// SIGTERM wait, which an ffmpeg with a stalled input ignores.
VOID TEST(AppEncoderTest, KillEnginesSkipsThePoliteStop)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    srs_utime_t starttime = srs_update_system_time();
    e.kill_engines();
    srs_utime_t elapsed = srs_update_system_time() - starttime;

    // The polite stop waits a whole second per engine that ignores SIGTERM.
    EXPECT_LT(elapsed, 500 * SRS_UTIME_MILLISECONDS);
    EXPECT_TRUE(e.ffmpegs.empty());
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
}

// A process that fast_kill has killed and reaped counts as stopped, so the stop that follows neither signals its
// pid, which may belong to another process by then, nor logs a SIGTERM stop that never happened.
VOID TEST(AppEncoderTest, FastKillMarksTheProcessStopped)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    e.fast_kill_engines();

    ASSERT_EQ(2, (int)e.ffmpegs.size());
    for (int i = 0; i < (int)e.ffmpegs.size(); i++) {
        EXPECT_FALSE(e.ffmpegs[i]->process->started());
        EXPECT_EQ(-1, e.ffmpegs[i]->process->get_pid());
        EXPECT_FALSE(mock_encoder_pid_alive(pids[i]));
    }
    e.kill_engines();
}

// The origin hub holds the engines when the publisher leaves, and the server's quit kills them.
VOID TEST(AppEncoderTest, OriginHubHoldsAndQuitKills)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsSharedPtr<SrsLiveSource> source(new SrsLiveSource());
    SrsUniquePtr<SrsOriginHub> hub(new SrsOriginHub());
    HELPER_ASSERT_SUCCESS(hub->initialize(source, req.get()));
    HELPER_ASSERT_SUCCESS(hub->on_publish());
    mock_encoder_wait_started(hub->encoder);
    std::vector<int> pids = mock_encoder_pids(hub->encoder);
    ASSERT_EQ(2, (int)pids.size());

    hub->on_unpublish();
    EXPECT_TRUE(hub->encoder->hold_deadline_ > 0);
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[1]));

    hub->dispose();
    EXPECT_TRUE(hub->encoder->ffmpegs.empty());
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
}

// Shutting down during the hold kills the held engines at once, and leaves engines with a publisher as they are.
VOID TEST(AppEncoderTest, DisposeDuringHoldKillsEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    e.dispose();
    EXPECT_TRUE(mock_encoder_pid_alive(pids[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(pids[1]));

    e.hold_on_unpublish();
    srs_utime_t starttime = srs_update_system_time();
    e.dispose();
    srs_utime_t elapsed = srs_update_system_time() - starttime;

    EXPECT_LT(elapsed, 500 * SRS_UTIME_MILLISECONDS);
    EXPECT_EQ(0, e.hold_deadline_);
    EXPECT_TRUE(e.ffmpegs.empty());
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
}


// Runs on_unpublish on its own coroutine, as a transcode reload does while a publisher comes and goes.
class MockEncoderStopper : public ISrsCoroutineHandler
{
public:
    SrsEncoder* e;
    bool done;
public:
    MockEncoderStopper(SrsEncoder* v) {
        e = v;
        done = false;
    }
    virtual srs_error_t cycle() {
        e->on_unpublish();
        done = true;
        return srs_success;
    }
};

static void mock_encoder_wait_stopper(MockEncoderStopper* s)
{
    for (int i = 0; i < 500 && !s->done; i++) {
        srs_usleep(10 * SRS_UTIME_MILLISECONDS);
    }
}

// A publisher returning while a reload stops the held engines waits for that stop, instead of stopping the loop a
// second time, which aborts SRS, and then gets a fresh set.
VOID TEST(AppEncoderTest, PublishDuringReloadStopGetsFreshEngines)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());
    e.hold_on_unpublish();

    MockEncoderStopper reload(&e);
    SrsSTCoroutine trd("reload", &reload, _srs_context->get_id());
    HELPER_ASSERT_SUCCESS(trd.start());
    srs_usleep(1 * SRS_UTIME_MILLISECONDS);

    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_stopper(&reload);
    EXPECT_TRUE(reload.done);
    mock_encoder_wait_started(&e);

    std::vector<int> fresh = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)fresh.size());
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[1]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
    e.on_unpublish();
}

// A publisher expired while its unpublish stops the engines, as a takeover does to a publisher that is already
// leaving, still waits for the encoder loop to stop them instead of aborting the server.
VOID TEST(AppEncoderTest, InterruptedStopStillWaitsForTheLoop)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 0;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> pids = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)pids.size());

    MockEncoderStopper s(&e);
    SrsSTCoroutine trd("stopper", &s, _srs_context->get_id());
    HELPER_ASSERT_SUCCESS(trd.start());

    // The stopper now waits in st_thread_join, because the engines ignore SIGTERM.
    srs_usleep(100 * SRS_UTIME_MILLISECONDS);
    trd.interrupt();

    mock_encoder_wait_stopper(&s);
    EXPECT_TRUE(s.done);
    EXPECT_TRUE(e.ffmpegs.empty());
    EXPECT_FALSE(mock_encoder_pid_alive(pids[0]));
    EXPECT_FALSE(mock_encoder_pid_alive(pids[1]));
}

// A publisher leaving while a reload stops the engines does not start a hold over engines that are going away, so
// the next publisher gets a fresh set instead of keeping nothing.
VOID TEST(AppEncoderTest, UnpublishDuringReloadStopDoesNotHold)
{
    srs_error_t err;

    MockEncoderConfig mc;
    ASSERT_TRUE(mc.ffmpeg_ready);
    HELPER_ASSERT_SUCCESS(mc.conf.parse(mock_encoder_config("unpublish_hold 60;")));
    SrsUniquePtr<SrsRequest> req(mock_encoder_request());

    SrsEncoder e;
    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);

    MockEncoderStopper reload(&e);
    SrsSTCoroutine trd("reload", &reload, _srs_context->get_id());
    HELPER_ASSERT_SUCCESS(trd.start());
    srs_usleep(1 * SRS_UTIME_MILLISECONDS);

    e.hold_on_unpublish();
    mock_encoder_wait_stopper(&reload);
    EXPECT_TRUE(reload.done);
    EXPECT_EQ(0, e.hold_deadline_);

    HELPER_ASSERT_SUCCESS(e.on_publish(req.get()));
    mock_encoder_wait_started(&e);
    std::vector<int> fresh = mock_encoder_pids(&e);
    ASSERT_EQ(2, (int)fresh.size());
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[0]));
    EXPECT_TRUE(mock_encoder_pid_alive(fresh[1]));
    e.on_unpublish();
}

#endif
