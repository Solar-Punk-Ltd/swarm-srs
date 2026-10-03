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
#include <srs_app_rtmp_conn.hpp>
#include <srs_app_edge.hpp>
#include <srs_app_pithy_print.hpp>
#include <srs_utest_config.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
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
        // Off by default, so a config without the directive behaves as stock SRS.
        EXPECT_EQ(0, conf.get_transcode_unpublish_hold(transcode));
        EXPECT_EQ(0, conf.get_transcode_unpublish_hold(NULL));
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

// Ends a test with the hold's kill instead of the polite stop, which costs a second per engine because the mock
// ignores SIGTERM. Tests keep the polite stop only where it is the behaviour under test.
static void mock_encoder_kill_all(SrsEncoder* e)
{
    e->hold_on_unpublish();
    e->on_unpublish();
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

    mock_encoder_kill_all(&e);
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
    mock_encoder_kill_all(&e);
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
    mock_encoder_kill_all(&e);
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
    mock_encoder_kill_all(&e);
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
    mock_encoder_kill_all(&e);
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
    mock_encoder_kill_all(&e);
}

#endif

SrsRequest* mock_takeover_request(std::string stream)
{
    SrsRequest* req = new SrsRequest();
    req->vhost = "__defaultVhost__";
    req->app = "live";
    req->stream = stream;
    return req;
}

void mock_takeover_publish(MockTakeoverPublisher* old, SrsRequest* req, SrsRtmpConnType type)
{
    SrsStatistic* stat = SrsStatistic::instance();
    srs_error_t err = stat->on_client(old->id, req, old, type);
    srs_freep(err);
    stat->on_stream_publish(req, old->id);
}

SrsRtmpConn* mock_takeover_rtmp_conn(SrsServer* server, SrsRequest* req)
{
    SrsRtmpConn* conn = new SrsRtmpConn(server, NULL, "127.0.0.1", 1935);
    conn->info->type = SrsRtmpConnFMLEPublish;
    conn->info->req->vhost = req->vhost;
    conn->info->req->app = req->app;
    conn->info->req->stream = req->stream;
    return conn;
}

MockTakeoverRtmpPublisher::MockTakeoverRtmpPublisher()
{
    conn = NULL;
    ended = false;
    client_io_ = NULL;
    client_ = NULL;
    stream_id_ = 0;
}

MockTakeoverRtmpPublisher::~MockTakeoverRtmpPublisher()
{
    // A connection still serving is told to go first, so its coroutine has ended before it is freed.
    if (conn && !ended) {
        conn->expire();
        for (int i = 0; i < 300 && !ended; i++) {
            srs_usleep(10 * SRS_UTIME_MILLISECONDS);
        }
    }
    srs_freep(conn);
    srs_freep(client_);
    srs_freep(client_io_);
}

srs_error_t MockTakeoverRtmpPublisher::publish(SrsServer* server, SrsRequest* req)
{
    srs_error_t err = srs_success;

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        return srs_error_new(ERROR_SOCKET_CREATE, "socketpair");
    }
    srs_netfd_t server_fd = srs_netfd_open_socket(fds[0]);
    srs_netfd_t client_fd = srs_netfd_open_socket(fds[1]);
    if (!server_fd || !client_fd) {
        return srs_error_new(ERROR_SOCKET_CREATE, "open socketpair");
    }

    SrsContextId cid = _srs_context->get_id();
    conn = new SrsRtmpConn(server, server_fd, "127.0.0.1", 1935);
    conn->manager = this;
    id = conn->get_id().c_str();
    _srs_context->set_id(cid);

    client_io_ = new SrsTcpConnection(client_fd);
    client_ = new SrsRtmpClient(client_io_);
    client_->set_recv_timeout(3 * SRS_UTIME_SECONDS);
    client_->set_send_timeout(3 * SRS_UTIME_SECONDS);

    if ((err = conn->start()) != srs_success) {
        ended = true;
        return srs_error_wrap(err, "start");
    }
    if ((err = client_->handshake()) != srs_success) {
        return srs_error_wrap(err, "handshake");
    }
    if ((err = client_->connect_app(req->app, "rtmp://127.0.0.1/" + req->app, NULL, false, NULL)) != srs_success) {
        return srs_error_wrap(err, "connect app");
    }
    std::string stream = req->stream;
    if (req->vhost != SRS_CONSTS_RTMP_DEFAULT_VHOST) {
        stream += "?vhost=" + req->vhost;
    }
    if ((err = client_->fmle_publish(stream, stream_id_)) != srs_success) {
        return srs_error_wrap(err, "publish");
    }

    SrsStatistic* stat = SrsStatistic::instance();
    for (int i = 0; i < 300; i++) {
        SrsStatisticStream* stream = stat->find_stream_by_url(req->get_stream_url());
        if (stream && stream->active && stream->publisher_id == id) {
            return err;
        }
        srs_usleep(10 * SRS_UTIME_MILLISECONDS);
    }
    return srs_error_new(ERROR_SYSTEM_STREAM_BUSY, "%s never published %s", id.c_str(), req->get_stream_url().c_str());
}

srs_error_t MockTakeoverRtmpPublisher::send_metadata()
{
    return client_->send_and_free_packet(new SrsOnMetaDataPacket(), stream_id_);
}

bool MockTakeoverRtmpPublisher::gone()
{
    return SrsStatistic::instance()->find_client(id) == NULL;
}

void MockTakeoverRtmpPublisher::remove(ISrsResource* /*c*/)
{
    ended = true;
}

MockTakeoverHookServer::MockTakeoverHookServer()
{
    port = 0;
    lfd_ = NULL;
    trd_ = NULL;
}

MockTakeoverHookServer::~MockTakeoverHookServer()
{
    srs_freep(trd_);
    srs_close_stfd(lfd_);
}

srs_error_t MockTakeoverHookServer::start()
{
    srs_error_t err = srs_success;

    if ((err = srs_tcp_listen("127.0.0.1", 0, &lfd_)) != srs_success) {
        return srs_error_wrap(err, "listen");
    }

    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(srs_netfd_fileno(lfd_), (sockaddr*)&addr, &addrlen) != 0) {
        return srs_error_new(ERROR_SOCKET_BIND, "getsockname");
    }
    port = ntohs(addr.sin_port);

    trd_ = new SrsSTCoroutine("hooks", this, _srs_context->get_id());
    return trd_->start();
}

srs_error_t MockTakeoverHookServer::cycle()
{
    srs_error_t err = srs_success;

    while ((err = trd_->pull()) == srs_success) {
        srs_netfd_t fd = srs_accept(lfd_, NULL, NULL, SRS_UTIME_NO_TIMEOUT);
        if (fd) {
            answer(fd);
            srs_close_stfd(fd);
        }
    }

    return err;
}

void MockTakeoverHookServer::answer(srs_netfd_t fd)
{
    SrsStSocket skt(fd);
    skt.set_recv_timeout(3 * SRS_UTIME_SECONDS);
    skt.set_send_timeout(3 * SRS_UTIME_SECONDS);

    // Reads the whole request, the headers and then the body their Content-Length announces, because closing with
    // anything unread would reset the connection before the answer is read.
    std::string request;
    size_t body_at = std::string::npos;
    size_t body_size = 0;
    while (body_at == std::string::npos || request.size() < body_at + body_size) {
        char buf[4096];
        ssize_t nread = 0;
        srs_error_t err = skt.read(buf, sizeof(buf), &nread);
        if (err != srs_success) {
            srs_freep(err);
            return;
        }
        request.append(buf, nread);
        if (body_at == std::string::npos && (body_at = request.find("\r\n\r\n")) != std::string::npos) {
            body_at += 4;
            size_t header = request.find("Content-Length: ");
            body_size = (header == std::string::npos) ? 0 : (size_t)::atoi(request.c_str() + header + 16);
        }
    }

    srs_usleep(50 * SRS_UTIME_MILLISECONDS);
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 1\r\n\r\n0";
    srs_error_t err = skt.write((void*)response.data(), response.length(), NULL);
    srs_freep(err);
}

// The take-over line names the protocol of the new publisher, so a log tells an RTMP takeover from an SRT one.
VOID TEST(TakeoverTest, TakeOverLineNamesTheNewPublishersProtocol)
{
    srs_error_t err;

    SrsUniquePtr<SrsRequest> rtmp(mock_takeover_request("takeover-line-rtmp"));
    MockTakeoverPublisher rtmp_old("takeover-line-rtmp-old", false);
    mock_takeover_publish(&rtmp_old, rtmp.get());

    SrsUniquePtr<SrsRequest> srt(mock_takeover_request("takeover-line-srt"));
    MockTakeoverPublisher srt_old("takeover-line-srt-old", false);
    mock_takeover_publish(&srt_old, srt.get());

    MockTakeoverLog log;
    HELPER_EXPECT_SUCCESS(srs_takeover_publisher(rtmp.get(), "rtmp", SRS_TAKEOVER_TIMEOUT));
    HELPER_EXPECT_SUCCESS(srs_takeover_publisher(srt.get(), "srt", SRS_TAKEOVER_TIMEOUT));

    EXPECT_LE(0, log.find("rtmp: take over /live/takeover-line-rtmp from publisher takeover-line-rtmp-old"));
    EXPECT_LE(0, log.find("srt: take over /live/takeover-line-srt from publisher takeover-line-srt-old"));
}

VOID TEST(RtmpTakeoverTest, ConfigDefaultOffAndOn)
{
    srs_error_t err;

    if (true) {
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost v { publish { normal_timeout 7000; } }"));
        EXPECT_FALSE(conf.get_publish_takeover("v"));
        EXPECT_FALSE(conf.get_publish_takeover("absent"));
    }

    if (true) {
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost v { publish { takeover on; } }"));
        EXPECT_TRUE(conf.get_publish_takeover("v"));
    }
}

// Turning the takeover on without an on_publish hook is allowed, but warned about, because then any publisher the
// security rules allow can take a live stream over.
VOID TEST(RtmpTakeoverTest, WarnsWhenOnWithoutAPublishHook)
{
    srs_error_t err;

    if (true) {
        MockTakeoverLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost nohooks { publish { takeover on; } }"));
        EXPECT_EQ(1, log.count("publish takeover of nohooks"));
    }

    if (true) {
        MockTakeoverLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost hooksoff { publish { takeover on; } "
            "http_hooks { enabled off; on_publish http://127.0.0.1:8085/api/v1/streams; } }"));
        EXPECT_EQ(1, log.count("publish takeover of hooksoff"));
    }

    if (true) {
        MockTakeoverLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost nourl { publish { takeover on; } "
            "http_hooks { enabled on; on_publish; } }"));
        EXPECT_EQ(1, log.count("publish takeover of nourl"));
    }

    // A vhost with both takeovers on and no hook is warned about once for each.
    if (true) {
        MockTakeoverLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost both { publish { takeover on; } "
            "srt { enabled on; takeover on; } }"));
        EXPECT_EQ(1, log.count("publish takeover of both"));
        EXPECT_EQ(1, log.count("srt takeover of both"));
    }

    if (true) {
        MockTakeoverLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost hooked { publish { takeover on; } "
            "http_hooks { enabled on; on_publish http://127.0.0.1:8085/api/v1/streams; } } "
            "vhost off { publish { normal_timeout 7000; } }"));
        EXPECT_EQ(0, log.count("takeover of"));
    }
}

// acquire_publish takes a busy stream over only when the takeover is on, and still refuses as busy while the old
// publisher's source stays busy.
VOID TEST(RtmpTakeoverTest, AcquirePublishTakesOverOnlyWhenOn)
{
    srs_error_t err;

    for (int on = 0; on <= 1; on++) {
        MockTakeoverConfig mc;
        HELPER_ASSERT_SUCCESS(mc.conf.parse(std::string(_MIN_OK_CONF) +
            "vhost __defaultVhost__ { publish { takeover " + (on ? "on" : "off") + "; } }"));

        std::string name = on ? "rtmp-acquire-on" : "rtmp-acquire-off";
        SrsUniquePtr<SrsRequest> req(mock_takeover_request(name));
        MockTakeoverServer server(req->get_stream_url());
        SrsSharedPtr<SrsLiveSource> source;
        HELPER_ASSERT_SUCCESS(_srs_sources->fetch_or_create(req.get(), server.server(), source));
        source->can_publish_ = false;

        MockTakeoverPublisher old(name + "-old", false);
        mock_takeover_publish(&old, req.get(), SrsRtmpConnFMLEPublish);

        SrsContextId cid = _srs_context->get_id();
        SrsRtmpConn* conn = mock_takeover_rtmp_conn(server.server(), req.get());

        err = conn->acquire_publish(source);
        EXPECT_EQ(ERROR_SYSTEM_STREAM_BUSY, srs_error_code(err));
        srs_freep(err);
        EXPECT_EQ(on == 1, old.expired);

        source->can_publish_ = true;
        srs_freep(conn);
        _srs_context->set_id(cid);
    }
}

// An edge forwards its publishers to its origin, whose own setting decides, so a publish to an edge never takes over.
VOID TEST(RtmpTakeoverTest, AcquirePublishLeavesAnEdgePublishAlone)
{
    srs_error_t err;

    MockTakeoverConfig mc;
    HELPER_ASSERT_SUCCESS(mc.conf.parse(_MIN_OK_CONF "vhost __defaultVhost__ { "
        "cluster { mode remote; origin 127.0.0.1:19350; } publish { takeover on; } }"));

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("rtmp-acquire-edge"));
    MockTakeoverServer server(req->get_stream_url());
    SrsSharedPtr<SrsLiveSource> source;
    HELPER_ASSERT_SUCCESS(_srs_sources->fetch_or_create(req.get(), server.server(), source));
    source->can_publish_ = false;
    source->publish_edge->state = SrsEdgeStatePublish;

    MockTakeoverPublisher old("rtmp-acquire-edge-old", false);
    mock_takeover_publish(&old, req.get(), SrsRtmpConnFMLEPublish);

    SrsContextId cid = _srs_context->get_id();
    SrsRtmpConn* conn = mock_takeover_rtmp_conn(server.server(), req.get());
    conn->info->edge = true;

    err = conn->acquire_publish(source);
    EXPECT_EQ(ERROR_SYSTEM_STREAM_BUSY, srs_error_code(err));
    srs_freep(err);
    EXPECT_FALSE(old.expired);

    source->publish_edge->state = SrsEdgeStateInit;
    source->can_publish_ = true;
    srs_freep(conn);
    _srs_context->set_id(cid);
}

// Runs publishing() on its own coroutine, as a connection does.
class MockTakeoverRtmpPublishing : public ISrsCoroutineHandler
{
public:
    SrsRtmpConn* conn;
    SrsSharedPtr<SrsLiveSource> source;
    bool done;
    int code;
public:
    MockTakeoverRtmpPublishing(SrsRtmpConn* c, SrsSharedPtr<SrsLiveSource> s) : conn(c), source(s), done(false), code(0) {
    }
    virtual srs_error_t cycle() {
        srs_error_t err = conn->publishing(source);
        code = srs_error_code(err);
        srs_freep(err);
        done = true;
        return srs_success;
    }
};

// A publisher refused because the stream is busy, after a takeover that timed out, or because it was interrupted
// while it waited to take over, never published, so it leaves the source to the publisher that holds it and sends no
// on_unpublish.
VOID TEST(RtmpTakeoverTest, RefusalNeverReleasesTheHolder)
{
    srs_error_t err;

    const char* names[] = {"rtmp-release-busy", "rtmp-release-timeout", "rtmp-release-interrupted"};
    const int codes[] = {ERROR_SYSTEM_STREAM_BUSY, ERROR_SYSTEM_STREAM_BUSY, ERROR_THREAD_INTERRUPED};
    for (int arm = 0; arm < 3; arm++) {
        MockTakeoverConfig mc;
        // Nothing listens on port 1, so an on_unpublish fails, and logs a warning that names its client.
        HELPER_ASSERT_SUCCESS(mc.conf.parse(std::string(_MIN_OK_CONF) + "vhost __defaultVhost__ { publish { takeover "
            + (arm == 0 ? "off" : "on") + "; } http_hooks { enabled on; on_unpublish http://127.0.0.1:1/unpublish; } }"));
        SrsUniquePtr<SrsRequest> req(mock_takeover_request(names[arm]));
        MockTakeoverServer server(req->get_stream_url());
        SrsSharedPtr<SrsLiveSource> source;
        HELPER_ASSERT_SUCCESS(_srs_sources->fetch_or_create(req.get(), server.server(), source));

        // Another publisher holds the stream, and does not go when told to.
        MockTakeoverPublisher holder(std::string(names[arm]) + "-holder", true);
        mock_takeover_publish(&holder, req.get(), SrsRtmpConnFMLEPublish);
        source->can_publish_ = false;

        SrsContextId cid = _srs_context->get_id();
        SrsRtmpConn* conn = mock_takeover_rtmp_conn(server.server(), req.get());
        std::string conn_id = _srs_context->get_id().c_str();

        MockTakeoverLog log;
        MockTakeoverRtmpPublishing publishing(conn, source);
        SrsSTCoroutine trd("publisher", &publishing, _srs_context->get_id());
        HELPER_ASSERT_SUCCESS(trd.start());
        if (arm == 2) {
            srs_usleep(30 * SRS_UTIME_MILLISECONDS);
            trd.interrupt();
        }
        // The timed out arm waits the whole takeover bound.
        for (int i = 0; i < 700 && !publishing.done; i++) {
            srs_usleep(10 * SRS_UTIME_MILLISECONDS);
        }

        EXPECT_TRUE(publishing.done) << names[arm];
        EXPECT_EQ(codes[arm], publishing.code) << names[arm];
        EXPECT_FALSE(source->can_publish(false)) << names[arm];
        EXPECT_EQ(arm > 0, holder.expired) << names[arm];
        EXPECT_EQ(0, log.count("on_unpublish failed, client_id=" + conn_id)) << names[arm];

        source->can_publish_ = true;
        SrsStatistic::instance()->on_disconnect(conn_id, srs_success);
        srs_freep(conn);
        _srs_context->set_id(cid);
    }
}

// The old publisher is a real RTMP connection. Told to go, it releases its source and sends its on_unpublish, and only
// then leaves the statistics, which is what the takeover waits for, so the new publisher is accepted after both.
VOID TEST(RtmpTakeoverTest, TakesOverAnRtmpPublisherOnceItHasLeft)
{
    srs_error_t err;

    // The hook server answers late, so a takeover that went ahead before the on_unpublish was answered would show.
    MockTakeoverHookServer hooks;
    HELPER_ASSERT_SUCCESS(hooks.start());
    MockTakeoverConfig mc;
    HELPER_ASSERT_SUCCESS(mc.conf.parse(std::string(_MIN_OK_CONF) + "vhost __defaultVhost__ { publish { takeover on; } "
        "http_hooks { enabled on; on_unpublish http://127.0.0.1:" + srs_int2str(hooks.port) + "/unpublish; } }"));
    SrsUniquePtr<SrsRequest> req(mock_takeover_request("rtmp-takeover-real"));
    MockTakeoverServer server(req->get_stream_url());

    MockTakeoverLog log;
    MockTakeoverRtmpPublisher old;
    HELPER_ASSERT_SUCCESS(old.publish(server.server(), req.get()));
    SrsSharedPtr<SrsLiveSource> source = _srs_sources->fetch(req.get());
    ASSERT_TRUE(source.get() != NULL);
    EXPECT_FALSE(source->can_publish(false));

    SrsContextId cid = _srs_context->get_id();
    SrsRtmpConn* conn = mock_takeover_rtmp_conn(server.server(), req.get());
    err = conn->acquire_publish(source);
    bool accepted = (err == srs_success);
    EXPECT_TRUE(accepted) << srs_error_desc(err);
    srs_freep(err);

    EXPECT_LE(0, log.find("rtmp: take over /live/rtmp-takeover-real from publisher " + old.id));
    int unpublished = log.find("on_unpublish ok, client_id=" + old.id);
    EXPECT_LE(0, unpublished);
    EXPECT_TRUE(old.gone());
    // The connection logs how it ended right after it leaves the statistics.
    EXPECT_LT(unpublished, log.find("[" + old.id + "] serve error"));
    EXPECT_FALSE(source->can_publish(false));

    // Only a publisher that was accepted releases the stream, never one that would release the holder's.
    if (accepted) {
        conn->release_publish(source);
    }
    srs_freep(conn);
    _srs_context->set_id(cid);
}

// An old RTMP publisher that frees its source when told to go and then spends long enough in its on_unpublish hook for
// the source manager to drop the dead source, before it leaves the statistics.
class MockTakeoverDroppedHolder : public MockTakeoverPublisher
{
public:
    SrsSharedPtr<SrsLiveSource> source;
public:
    MockTakeoverDroppedHolder(std::string v, SrsSharedPtr<SrsLiveSource> s) : MockTakeoverPublisher(v, false), source(s) {
    }
    virtual srs_error_t cycle() {
        source->can_publish_ = true;
        SrsStatistic::instance()->on_stream_close(source->req);
        // Died long enough ago that the source manager's next tick, which may come during the hook, drops it.
        source->stream_die_at_ = srs_get_system_time() - 200 * SRS_UTIME_SECONDS;
        srs_error_t err = _srs_sources->notify(0, 0, 0);
        srs_freep(err);
        srs_usleep(30 * SRS_UTIME_MILLISECONDS);
        SrsStatistic::instance()->on_disconnect(id, srs_success);
        return srs_success;
    }
};

// A newcomer fetched its source before it waited. If the source manager dropped that source during the wait, the
// newcomer is refused as busy rather than publish where no player can find it, and it reconnects to the pool's source.
VOID TEST(RtmpTakeoverTest, RefusesWhenTheSourceWasDroppedDuringTheWait)
{
    srs_error_t err;

    MockTakeoverConfig mc;
    HELPER_ASSERT_SUCCESS(mc.conf.parse(_MIN_OK_CONF "vhost __defaultVhost__ { publish { takeover on; } }"));
    SrsUniquePtr<SrsRequest> req(mock_takeover_request("rtmp-takeover-dropped"));
    MockTakeoverServer server(req->get_stream_url());
    SrsSharedPtr<SrsLiveSource> source;
    HELPER_ASSERT_SUCCESS(_srs_sources->fetch_or_create(req.get(), server.server(), source));
    source->can_publish_ = false;

    MockTakeoverDroppedHolder holder("rtmp-takeover-dropped-holder", source);
    mock_takeover_publish(&holder, req.get(), SrsRtmpConnFMLEPublish);

    SrsContextId cid = _srs_context->get_id();
    SrsRtmpConn* conn = mock_takeover_rtmp_conn(server.server(), req.get());
    err = conn->acquire_publish(source);
    bool accepted = (err == srs_success);
    EXPECT_EQ(ERROR_SYSTEM_STREAM_BUSY, srs_error_code(err)) << srs_error_desc(err);
    srs_freep(err);
    EXPECT_TRUE(holder.expired);
    EXPECT_TRUE(_srs_sources->fetch(req.get()).get() == NULL);
    // Refused, so it never published on the dropped source.
    EXPECT_TRUE(source->can_publish(false));

    if (accepted) {
        conn->release_publish(source);
    }
    srs_freep(conn);
    _srs_context->set_id(cid);
}

extern SrsStageManager* _srs_stages;

// An RTMP publisher's periodic statistics line ends with the vhost its request resolved to: a configured vhost by its
// own name, as a transcode republish names one, and a host that no vhost names as the default vhost. So a log reader can
// tell a broadcaster from a republish onto another vhost without the lines that carry the stream key.
VOID TEST(RtmpPublishTest, PeriodicLineEndsWithItsVhost)
{
    srs_error_t err;

    // Every turn of the publish loop prints, instead of once in pithy_print_ms.
    SrsUniquePtr<SrsPithyPrint> pprint(SrsPithyPrint::create_rtmp_publish());
    SrsStageInfo* stage = _srs_stages->fetch_or_create(pprint->stage_id);
    srs_utime_t interval = stage->interval;
    stage->interval = 1 * SRS_UTIME_MILLISECONDS;

    const char* vhosts[] = {SRS_CONSTS_RTMP_DEFAULT_VHOST, "abr"};
    for (int i = 0; i < 2; i++) {
        MockTakeoverConfig mc;
        HELPER_ASSERT_SUCCESS(mc.conf.parse(_MIN_OK_CONF
            "vhost __defaultVhost__ { publish { firstpkt_timeout 200; normal_timeout 200; } } "
            "vhost abr { publish { firstpkt_timeout 200; normal_timeout 200; } }"));
        SrsUniquePtr<SrsRequest> req(mock_takeover_request(std::string("rtmp-stats-") + (i ? "rung" : "ingest")));
        req->vhost = vhosts[i];
        MockTakeoverServer server(req->get_stream_url());

        MockTakeoverLog log;
        MockTakeoverRtmpPublisher publisher;
        HELPER_ASSERT_SUCCESS(publisher.publish(server.server(), req.get()));

        // The loop needs traffic on every turn, or it ends the publish as timed out before a turn that prints. It
        // also measures the time between prints on the clock that a running server's timers refresh.
        int line = -1;
        for (int j = 0; j < 60 && line < 0; j++) {
            HELPER_EXPECT_SUCCESS(publisher.send_metadata());
            srs_usleep(50 * SRS_UTIME_MILLISECONDS);
            srs_update_system_time();
            line = log.find("[" + publisher.id + "] <- " SRS_CONSTS_LOG_CLIENT_PUBLISH " time=");
        }
        EXPECT_LE(0, line) << vhosts[i];
        if (line >= 0) {
            EXPECT_TRUE(srs_string_ends_with(log.lines[line], std::string(", pnt=200, vhost=") + vhosts[i]))
                << log.lines[line];
        }
    }

    stage->interval = interval;
}

// A bridge whose publish fails, so a publish can fail after the live source has marked itself busy. The source owns
// and frees it when the stream is released.
class MockRtmpReleaseBridge : public ISrsStreamBridge
{
public:
    MockRtmpReleaseBridge() {
    }
    virtual srs_error_t initialize(SrsRequest* /*r*/) {
        return srs_success;
    }
    virtual srs_error_t on_publish() {
        return srs_error_new(ERROR_SOCKET_CONNECT, "mock bridge publish failed");
    }
    virtual srs_error_t on_frame(SrsSharedPtrMessage* /*frame*/) {
        return srs_success;
    }
    virtual void on_unpublish() {
    }
};

// A publish that fails after the live source marked itself busy releases the stream, so the next publisher is
// accepted instead of the stream staying busy until SRS restarts.
VOID TEST(RtmpPublishTest, FailedPublishReleasesTheStream)
{
    srs_error_t err;

    MockTakeoverConfig mc;
    HELPER_ASSERT_SUCCESS(mc.conf.parse(_MIN_OK_CONF "vhost __defaultVhost__ { }"));
    SrsUniquePtr<SrsRequest> req(mock_takeover_request("rtmp-release-failed"));
    MockTakeoverServer server(req->get_stream_url());
    SrsSharedPtr<SrsLiveSource> source;
    HELPER_ASSERT_SUCCESS(_srs_sources->fetch_or_create(req.get(), server.server(), source));
    source->set_bridge(new MockRtmpReleaseBridge());

    SrsContextId cid = _srs_context->get_id();
    SrsRtmpConn* first = mock_takeover_rtmp_conn(server.server(), req.get());
    std::string first_id = _srs_context->get_id().c_str();
    err = first->publishing(source);
    EXPECT_TRUE(srs_error_desc(err).find("mock bridge publish failed") != std::string::npos);
    srs_freep(err);
    EXPECT_TRUE(source->can_publish(false));
    SrsStatistic::instance()->on_disconnect(first_id, srs_success);
    srs_freep(first);
    _srs_context->set_id(cid);

    // The release freed the failing bridge with the rest of the publish, so the second publisher meets none.
    SrsRtmpConn* second = mock_takeover_rtmp_conn(server.server(), req.get());
    err = second->acquire_publish(source);
    bool accepted = (err == srs_success);
    EXPECT_TRUE(accepted) << srs_error_desc(err);
    srs_freep(err);

    // Only a publisher that was accepted releases the stream. Without the fix the stream is still busy here, so it is
    // freed by hand for the tests that follow.
    if (accepted) {
        second->release_publish(source);
    }
    source->can_publish_ = true;
    srs_freep(second);
    _srs_context->set_id(cid);
}
