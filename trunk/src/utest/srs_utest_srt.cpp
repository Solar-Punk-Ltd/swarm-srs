//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//
#include <srs_utest_srt.hpp>

#include <srs_kernel_error.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_protocol_srt.hpp>
#include <srs_protocol_rtmp_stack.hpp>
#include <srs_app_srt_utility.hpp>
#include <srs_app_srt_server.hpp>
#include <srs_core_autofree.hpp>
#include <srs_app_srt_conn.hpp>
#include <srs_app_srt_source.hpp>
#include <srs_app_source.hpp>
#include <srs_app_server.hpp>
#include <srs_app_hybrid.hpp>
#include <srs_app_statistic.hpp>
#include <srs_app_st.hpp>
#include <srs_app_conn.hpp>
#include <srs_app_config.hpp>
#include <srs_utest_config.hpp>

#include <sstream>
#include <vector>
using namespace std;

#include <srt/srt.h>

extern SrsSrtEventLoop* _srt_eventloop;

// TODO: FIXME: set srt log handler.

// Test srt st service
VOID TEST(ServiceSrtPoller, SrtPollOperateSocket) 
{
    srs_error_t err = srs_success;

    ISrsSrtPoller* srt_poller = srs_srt_poller_new();
    HELPER_EXPECT_SUCCESS(srt_poller->initialize());

    srs_srt_t srt_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srs_srt_socket(&srt_fd));
    EXPECT_TRUE(srt_fd > 0);

    SrsSrtSocket* srt_socket = new SrsSrtSocket(srt_poller, srt_fd);
    EXPECT_EQ(srt_socket->events(), 0);

    // Enable read, will subscribe SRT_EPOLL_IN and  SRT_EPOLL_ERR event in srt poller.
    HELPER_EXPECT_SUCCESS(srt_socket->enable_read());
    EXPECT_TRUE(srt_socket->events() & SRT_EPOLL_IN);
    EXPECT_TRUE(srt_socket->events() & SRT_EPOLL_ERR);

    // Enable read, will subscribe SRT_EPOLL_OUT and  SRT_EPOLL_ERR event in srt poller.
    HELPER_EXPECT_SUCCESS(srt_socket->enable_write());
    EXPECT_TRUE(srt_socket->events() & SRT_EPOLL_OUT);
    EXPECT_TRUE(srt_socket->events() & SRT_EPOLL_ERR);

    // Disable read, will unsubscribe SRT_EPOLL_IN event in srt poller.
    HELPER_EXPECT_SUCCESS(srt_socket->disable_read());
    EXPECT_FALSE(srt_socket->events() & SRT_EPOLL_IN);
    EXPECT_TRUE(srt_socket->events() & SRT_EPOLL_ERR);

    // Disable write, will unsubscribe SRT_EPOLL_OUT event in srt poller.
    HELPER_EXPECT_SUCCESS(srt_socket->disable_write());
    EXPECT_FALSE(srt_socket->events() & SRT_EPOLL_OUT);
    EXPECT_TRUE(srt_socket->events() & SRT_EPOLL_ERR);

    EXPECT_EQ(srt_poller->size(), 1);
    // Delete socket, will remove in srt poller.
    srs_freep(srt_socket);
    EXPECT_EQ(srt_poller->size(), 0);

    srs_freep(srt_poller);
}

VOID TEST(ServiceSrtPoller, SrtSetGetSocketOpt) 
{
    srs_error_t err = srs_success;

    srs_srt_t srt_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srs_srt_socket(&srt_fd));
    HELPER_EXPECT_SUCCESS(srs_srt_nonblock(srt_fd));

    int64_t maxbw = 20000;
    int mss = 1400;
    int payload_size = 1316;
    int connect_timeout = 5000;
    int peer_idle_timeout = 10000;
    bool tsbpdmode = false;
    int sndbuf = 2 * 1024 * 1024;
    int rcvbuf = 10 * 1024 * 1024;
    bool tlpktdrop = false;
    int latency = 0;
    int rcv_latency = 120;
    int peer_latency = 120;
    std::string streamid = "SRS_SRT";

    HELPER_EXPECT_SUCCESS(srs_srt_set_maxbw(srt_fd, maxbw));
    HELPER_EXPECT_SUCCESS(srs_srt_set_mss(srt_fd, mss));
    HELPER_EXPECT_SUCCESS(srs_srt_set_payload_size(srt_fd, payload_size));
    HELPER_EXPECT_SUCCESS(srs_srt_set_connect_timeout(srt_fd, connect_timeout));
    HELPER_EXPECT_SUCCESS(srs_srt_set_peer_idle_timeout(srt_fd, peer_idle_timeout));
    HELPER_EXPECT_SUCCESS(srs_srt_set_tsbpdmode(srt_fd, tsbpdmode));
    HELPER_EXPECT_SUCCESS(srs_srt_set_sndbuf(srt_fd, sndbuf));
    HELPER_EXPECT_SUCCESS(srs_srt_set_rcvbuf(srt_fd, rcvbuf));
    HELPER_EXPECT_SUCCESS(srs_srt_set_tlpktdrop(srt_fd, tlpktdrop));
    HELPER_EXPECT_SUCCESS(srs_srt_set_latency(srt_fd, latency));
    HELPER_EXPECT_SUCCESS(srs_srt_set_rcv_latency(srt_fd, rcv_latency));
    HELPER_EXPECT_SUCCESS(srs_srt_set_peer_latency(srt_fd, peer_latency));
    HELPER_EXPECT_SUCCESS(srs_srt_set_streamid(srt_fd, streamid));

    bool b;
    int i = 0;
    int64_t i64 = 0;
    std::string s;

    HELPER_EXPECT_SUCCESS(srs_srt_get_maxbw(srt_fd, i64));
    EXPECT_EQ(i64, maxbw);
    HELPER_EXPECT_SUCCESS(srs_srt_get_mss(srt_fd, i));
    EXPECT_EQ(i, mss);
    HELPER_EXPECT_SUCCESS(srs_srt_get_payload_size(srt_fd, i));
    EXPECT_EQ(i, payload_size);
    HELPER_EXPECT_SUCCESS(srs_srt_get_connect_timeout(srt_fd, i));
    EXPECT_EQ(i, connect_timeout);
    HELPER_EXPECT_SUCCESS(srs_srt_get_peer_idle_timeout(srt_fd, i));
    EXPECT_EQ(i, peer_idle_timeout);

    // Don't check b equal to option blow, because some opt will deterimated after srt handshake done or change when set.
    HELPER_EXPECT_SUCCESS(srs_srt_get_tsbpdmode(srt_fd, b));
    HELPER_EXPECT_SUCCESS(srs_srt_get_sndbuf(srt_fd, i));
    HELPER_EXPECT_SUCCESS(srs_srt_get_rcvbuf(srt_fd, i));
    HELPER_EXPECT_SUCCESS(srs_srt_get_tlpktdrop(srt_fd, b));
    HELPER_EXPECT_SUCCESS(srs_srt_get_latency(srt_fd, i));
    HELPER_EXPECT_SUCCESS(srs_srt_get_rcv_latency(srt_fd, i));
    HELPER_EXPECT_SUCCESS(srs_srt_get_peer_latency(srt_fd, i));

    HELPER_EXPECT_SUCCESS(srs_srt_get_streamid(srt_fd, s));
    EXPECT_EQ(s, streamid);
}

class MockSrtServer
{
public:
    SrsSrtSocket* srt_socket_;
    srs_srt_t srt_server_fd_;

    MockSrtServer() {
        srt_server_fd_ = srs_srt_socket_invalid();
        srt_socket_ = NULL;
    }

    srs_error_t create_socket() {
        srs_error_t err = srs_success;
        if ((err = srs_srt_socket_with_default_option(&srt_server_fd_)) != srs_success) {
            return srs_error_wrap(err, "create srt socket");
        }
        return err;
    }

    srs_srt_t fd() {
        return srt_server_fd_;
    }

    srs_error_t listen(std::string ip, int port) {
        srs_error_t err = srs_success;

        if ((err = srs_srt_listen(srt_server_fd_, ip, port)) != srs_success) {
            return srs_error_wrap(err, "srt listen");
        }

        srt_socket_ = new SrsSrtSocket(_srt_eventloop->poller(), srt_server_fd_);

        return err;
    }

    virtual ~MockSrtServer() {
        srs_freep(srt_socket_);
    }

    virtual srs_error_t accept(srs_srt_t* client_fd) {
        srs_error_t err = srs_success;

        if ((err = srt_socket_->accept(client_fd)) != srs_success) {
            return srs_error_wrap(err, "srt accept");
        }

        return err;
    }
};

VOID TEST(ServiceStSRTTest, ListenConnectAccept) 
{
    srs_error_t err = srs_success;

    std::string server_ip = "127.0.0.1";
    int server_port = 19000;

    MockSrtServer srt_server;
    HELPER_EXPECT_SUCCESS(srt_server.create_socket());
    HELPER_EXPECT_SUCCESS(srt_server.listen(server_ip, server_port));

    srs_srt_t srt_client_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srs_srt_socket(&srt_client_fd));

    SrsUniquePtr<SrsSrtSocket> srt_client_socket(new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd));

    // No client connected, accept will timeout.
    srs_srt_t srt_fd = srs_srt_socket_invalid();
    // Make utest fast timeout.
    srt_server.srt_socket_->set_recv_timeout(50 * SRS_UTIME_MILLISECONDS);
    err = srt_server.accept(&srt_fd);
    EXPECT_EQ(srs_error_code(err), ERROR_SRT_TIMEOUT);
    EXPECT_EQ(srt_fd, srs_srt_socket_invalid());
    srs_freep(err);

    // Client connect to server
    HELPER_EXPECT_SUCCESS(srt_client_socket->connect(server_ip, server_port));

    // Server will accept one client.
    HELPER_EXPECT_SUCCESS(srt_server.accept(&srt_fd));
    EXPECT_NE(srt_fd, srs_srt_socket_invalid());
}

VOID TEST(ServiceStSRTTest, ConnectTimeout) 
{
    srs_error_t err = srs_success;

    srs_srt_t srt_client_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srs_srt_socket_with_default_option(&srt_client_fd));
    SrsSrtSocket* srt_client_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd);

    srt_client_socket->set_send_timeout(50 * SRS_UTIME_MILLISECONDS);
    // Client connect to server which is no listening.
    HELPER_EXPECT_FAILED(srt_client_socket->connect("127.0.0.1", 9099));
}

VOID TEST(ServiceStSRTTest, ConnectWithStreamid) 
{
    srs_error_t err = srs_success;

    std::string server_ip = "127.0.0.1";
    int server_port = 19000;

    MockSrtServer srt_server;
    HELPER_EXPECT_SUCCESS(srt_server.create_socket());
    HELPER_EXPECT_SUCCESS(srt_server.listen(server_ip, server_port));

    std::string streamid = "SRS_SRT_Streamid";
    srs_srt_t srt_client_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srs_srt_socket_with_default_option(&srt_client_fd));
    HELPER_EXPECT_SUCCESS(srs_srt_set_streamid(srt_client_fd, streamid));
    SrsSrtSocket* srt_client_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd);

    HELPER_EXPECT_SUCCESS(srt_client_socket->connect(server_ip, server_port));

    srs_srt_t srt_server_accepted_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srt_server.accept(&srt_server_accepted_fd));
    EXPECT_NE(srt_server_accepted_fd, srs_srt_socket_invalid());
    std::string s;
    HELPER_EXPECT_SUCCESS(srs_srt_get_streamid(srt_server_accepted_fd, s));
    EXPECT_EQ(s, streamid);
}

VOID TEST(ServiceStSRTTest, ReadWrite) 
{
    srs_error_t err = srs_success;

    std::string server_ip = "127.0.0.1";
    int server_port = 19000;

    MockSrtServer srt_server;
    HELPER_EXPECT_SUCCESS(srt_server.create_socket());
    HELPER_EXPECT_SUCCESS(srt_server.listen(server_ip, server_port));

    srs_srt_t srt_client_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srs_srt_socket_with_default_option(&srt_client_fd));
    SrsSrtSocket* srt_client_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd);

    // Client connect to server
    HELPER_EXPECT_SUCCESS(srt_client_socket->connect(server_ip, server_port));

    // Server will accept one client.
    srs_srt_t srt_server_accepted_fd = srs_srt_socket_invalid();
    HELPER_EXPECT_SUCCESS(srt_server.accept(&srt_server_accepted_fd));
    EXPECT_NE(srt_server_accepted_fd, srs_srt_socket_invalid());
    SrsSrtSocket* srt_server_accepted_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_server_accepted_fd);

    if (true) {
        std::string content = "Hello, SRS SRT!";

        // Client send msg to server.
        ssize_t nb_write = 0;
        HELPER_EXPECT_SUCCESS(srt_client_socket->sendmsg((char*)content.data(), content.size(), &nb_write));
        EXPECT_EQ((size_t)nb_write, content.size());

        // Server recv msg from client
        char buf[1500];
        ssize_t nb_read = 0;
        HELPER_EXPECT_SUCCESS(srt_server_accepted_socket->recvmsg(buf, sizeof(buf), &nb_read));
        EXPECT_EQ((size_t)nb_read, content.size());
        EXPECT_EQ(std::string(buf, nb_read), content);

        // Server echo msg back to client.
        HELPER_EXPECT_SUCCESS(srt_server_accepted_socket->sendmsg(buf, nb_read, &nb_write));
        EXPECT_EQ((size_t)nb_write, content.size());

        // Client recv echo msg from server.
        HELPER_EXPECT_SUCCESS(srt_client_socket->recvmsg(buf, sizeof(buf), &nb_read));
        EXPECT_EQ((size_t)nb_read, content.size());
        EXPECT_EQ(std::string(buf, nb_read), content);
    }

    if (true) {
        char buf[1500];
        ssize_t nb_read = 0;
        // Make socket fast timeout in ustet.
        srt_server_accepted_socket->set_recv_timeout(50 * SRS_UTIME_MILLISECONDS);
        // Recv msg from client, but client no send any msg, so will be timeout.
        err = srt_server_accepted_socket->recvmsg(buf, sizeof(buf), &nb_read);
        EXPECT_EQ(srs_error_code(err), ERROR_SRT_TIMEOUT);
        srs_freep(err);
    }
}

// Test srt server 
class MockSrtHandler : public ISrsSrtHandler
{
private:
    srs_srt_t srt_fd;
public:
	MockSrtHandler() {
        srt_fd = srs_srt_socket_invalid();
	}
	virtual ~MockSrtHandler() {
	}
public:
    virtual srs_error_t on_srt_client(srs_srt_t fd) {
        srt_fd = fd;
        return srs_success;
	}
};

VOID TEST(SrtServerTest, SrtListener) 
{
    srs_error_t err = srs_success;

    if (true) {
        MockSrtHandler h;
        SrsSrtListener srt_listener(&h, "127.0.0.1", 9000);
	    HELPER_EXPECT_SUCCESS(srt_listener.create_socket());
        HELPER_EXPECT_SUCCESS(srt_listener.listen());
		EXPECT_TRUE(srt_listener.fd() > 0);
    }
}

// Test srt app
VOID TEST(ProtocolSrtTest, SrtGetStreamInfoNormal) 
{
    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::r=live/livestream,key1=value1,key2=value2", mode, vhost, subpath));
        EXPECT_EQ(SrtModePull, mode);
        EXPECT_STREQ("", vhost.c_str());
        EXPECT_STREQ("live/livestream?key1=value1&key2=value2", subpath.c_str());
    }

    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::h=host.com,r=live/livestream,key1=value1,key2=value2", mode, vhost, subpath));
        EXPECT_EQ(SrtModePull, mode);
        EXPECT_STREQ("host.com", vhost.c_str());
        EXPECT_STREQ("live/livestream?vhost=host.com&key1=value1&key2=value2", subpath.c_str());
    }
}

VOID TEST(ProtocolSrtTest, SrtGetStreamInfoMethod) 
{
    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::r=live/livestream,m=request", mode, vhost, subpath));
        EXPECT_EQ(SrtModePull, mode);
        EXPECT_STREQ("live/livestream", subpath.c_str());
    }

    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::r=live/livestream,m=publish", mode, vhost, subpath));
        EXPECT_EQ(SrtModePush, mode);
        EXPECT_STREQ("live/livestream", subpath.c_str());
    }
}

VOID TEST(ProtocolSrtTest, SrtGetStreamInfoCompatible) 
{
    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::h=live/livestream,m=request", mode, vhost, subpath));
        EXPECT_EQ(SrtModePull, mode);
        EXPECT_STREQ("", vhost.c_str());
        EXPECT_STREQ("live/livestream", subpath.c_str());
    }

    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::h=live/livestream,m=publish", mode, vhost, subpath));
        EXPECT_EQ(SrtModePush, mode);
        EXPECT_STREQ("", vhost.c_str());
        EXPECT_STREQ("live/livestream", subpath.c_str());
    }

    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::h=srs.srt.com.cn/live/livestream,m=request", mode, vhost, subpath));
        EXPECT_EQ(SrtModePull, mode);
        EXPECT_STREQ("srs.srt.com.cn", vhost.c_str());
        EXPECT_STREQ("live/livestream?vhost=srs.srt.com.cn", subpath.c_str());
    }

    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::h=srs.srt.com.cn/live/livestream,m=publish", mode, vhost, subpath));
        EXPECT_EQ(SrtModePush, mode);
        EXPECT_STREQ("srs.srt.com.cn", vhost.c_str());
        EXPECT_STREQ("live/livestream?vhost=srs.srt.com.cn", subpath.c_str());
    }

    if (true) {
        SrtMode mode; string vhost; string subpath;
        EXPECT_TRUE(srs_srt_streamid_info("#!::h=live/livestream?secret=d6d2be37,m=publish", mode, vhost, subpath));
        EXPECT_EQ(SrtModePush, mode);
        EXPECT_STREQ("", vhost.c_str());
        EXPECT_STREQ("live/livestream?secret=d6d2be37", subpath.c_str());
    }
}

VOID TEST(ProtocolSrtTest, SrtStreamIdToRequest)
{
    if (true) {
        SrtMode mode;
        SrsRequest req;
        EXPECT_TRUE(srs_srt_streamid_to_request("#!::r=live/livestream?key1=val1,key2=val2", mode, &req));
        EXPECT_EQ(mode, SrtModePull);
        EXPECT_STREQ(req.vhost.c_str(), srs_get_public_internet_address().c_str());
        EXPECT_STREQ(req.app.c_str(), "live");
        EXPECT_STREQ(req.stream.c_str(), "livestream");
        EXPECT_STREQ(req.param.c_str(), "key1=val1&key2=val2");
    }

    if (true) {
        SrtMode mode;
        SrsRequest req;
        EXPECT_TRUE(srs_srt_streamid_to_request("#!::h=srs.srt.com.cn,r=live/livestream?key1=val1,key2=val2", mode, &req));
        EXPECT_EQ(mode, SrtModePull);
        EXPECT_STREQ(req.vhost.c_str(), "srs.srt.com.cn");
        EXPECT_STREQ(req.app.c_str(), "live");
        EXPECT_STREQ(req.stream.c_str(), "livestream");
        EXPECT_STREQ(req.param.c_str(), "vhost=srs.srt.com.cn&key1=val1&key2=val2");
    }

    if (true) {
        SrtMode mode;
        SrsRequest req;
        EXPECT_TRUE(srs_srt_streamid_to_request("#!::h=live/livestream?key1=val1,key2=val2", mode, &req));
        EXPECT_EQ(mode, SrtModePull);
        EXPECT_STREQ(req.vhost.c_str(), srs_get_public_internet_address().c_str());
        EXPECT_STREQ(req.app.c_str(), "live");
        EXPECT_STREQ(req.stream.c_str(), "livestream");
        EXPECT_STREQ(req.param.c_str(), "key1=val1&key2=val2");
    }

    if (true) {
        SrtMode mode;
        SrsRequest req;
        EXPECT_TRUE(srs_srt_streamid_to_request("#!::h=srs.srt.com.cn/live/livestream?key1=val1,key2=val2", mode, &req));
        EXPECT_EQ(mode, SrtModePull);
        EXPECT_STREQ(req.vhost.c_str(), "srs.srt.com.cn");
        EXPECT_STREQ(req.app.c_str(), "live");
        EXPECT_STREQ(req.stream.c_str(), "livestream");
        EXPECT_STREQ(req.param.c_str(), "vhost=srs.srt.com.cn&key1=val1&key2=val2");
    }
}

VOID TEST(ServiceSRTTest, Encrypt) 
{
    srs_error_t err = srs_success;

    std::string server_ip = "127.0.0.1";
    int server_port = 19000;

    MockSrtServer srt_server;
    HELPER_EXPECT_SUCCESS(srt_server.create_socket());

    string passphrase = "srt_passphrase";
    HELPER_EXPECT_SUCCESS(srs_srt_set_passphrase(srt_server.fd(), passphrase));
    HELPER_EXPECT_SUCCESS(srt_server.listen(server_ip, server_port));

    std::string streamid = "SRS_SRT_Streamid";
    if (true) {
        srs_srt_t srt_client_fd = srs_srt_socket_invalid();
        HELPER_EXPECT_SUCCESS(srs_srt_socket_with_default_option(&srt_client_fd));
        HELPER_EXPECT_SUCCESS(srs_srt_set_streamid(srt_client_fd, streamid));
        SrsSrtSocket* srt_client_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd);

        // SRT connect without passphrase, will reject.
        HELPER_EXPECT_FAILED(srt_client_socket->connect(server_ip, server_port));
    }

    if (true) {
        srs_srt_t srt_client_fd = srs_srt_socket_invalid();
        HELPER_EXPECT_SUCCESS(srs_srt_socket_with_default_option(&srt_client_fd));
        HELPER_EXPECT_SUCCESS(srs_srt_set_streamid(srt_client_fd, streamid));
        HELPER_EXPECT_SUCCESS(srs_srt_set_passphrase(srt_client_fd, "wrong_passphrase"));
        SrsSrtSocket* srt_client_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd);

        // SRT connect with wrong passphrase, will reject.
        HELPER_EXPECT_FAILED(srt_client_socket->connect(server_ip, server_port));
    }

    if (true) {
        srs_srt_t srt_client_fd = srs_srt_socket_invalid();
        HELPER_EXPECT_SUCCESS(srs_srt_socket_with_default_option(&srt_client_fd));
        HELPER_EXPECT_SUCCESS(srs_srt_set_streamid(srt_client_fd, streamid));
        // Set correct passphrase.
        HELPER_EXPECT_SUCCESS(srs_srt_set_passphrase(srt_client_fd, passphrase));
        SrsSrtSocket* srt_client_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd);
        HELPER_EXPECT_SUCCESS(srt_client_socket->connect(server_ip, server_port));

        srs_srt_t srt_server_accepted_fd = srs_srt_socket_invalid();
        HELPER_EXPECT_SUCCESS(srt_server.accept(&srt_server_accepted_fd));
        EXPECT_NE(srt_server_accepted_fd, srs_srt_socket_invalid());
        std::string s;
        HELPER_EXPECT_SUCCESS(srs_srt_get_streamid(srt_server_accepted_fd, s));
        EXPECT_EQ(s, streamid);
    }

    if (true) {
        int pbkeylens[4] = {0, 16, 24, 32};
        for (int i = 0; i < (int)(sizeof(pbkeylens) / sizeof(pbkeylens[0])); ++i) {
            srs_srt_t srt_client_fd = srs_srt_socket_invalid();
            HELPER_EXPECT_SUCCESS(srs_srt_socket_with_default_option(&srt_client_fd));
            HELPER_EXPECT_SUCCESS(srs_srt_set_streamid(srt_client_fd, streamid));
            // Set correct passphrase.
            HELPER_EXPECT_SUCCESS(srs_srt_set_passphrase(srt_client_fd, passphrase));
            // Set different pbkeylen.
            HELPER_EXPECT_SUCCESS(srs_srt_set_pbkeylen(srt_client_fd, pbkeylens[i]));
            SrsSrtSocket* srt_client_socket = new SrsSrtSocket(_srt_eventloop->poller(), srt_client_fd);
            HELPER_EXPECT_SUCCESS(srt_client_socket->connect(server_ip, server_port));

            srs_srt_t srt_server_accepted_fd = srs_srt_socket_invalid();
            HELPER_EXPECT_SUCCESS(srt_server.accept(&srt_server_accepted_fd));
            EXPECT_NE(srt_server_accepted_fd, srs_srt_socket_invalid());
            std::string s;
            HELPER_EXPECT_SUCCESS(srs_srt_get_streamid(srt_server_accepted_fd, s));
            EXPECT_EQ(s, streamid);
        }
    }
}

// TODO: FIXME: add mpegts conn test
// set srt option, recv srt client, get srt client opt and check.

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

static SrsRequest* mock_takeover_request(std::string stream)
{
    SrsRequest* req = new SrsRequest();
    req->vhost = "__defaultVhost__";
    req->app = "live";
    req->stream = stream;
    return req;
}

static void mock_takeover_publish(MockTakeoverPublisher* old, SrsRequest* req)
{
    SrsStatistic* stat = SrsStatistic::instance();
    srs_error_t err = stat->on_client(old->id, req, old, SrsSrtConnPublish);
    srs_freep(err);
    stat->on_stream_publish(req, old->id);
}

VOID TEST(SrtTakeoverTest, ConfigDefaultOffAndOn)
{
    srs_error_t err;

    if (true) {
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost v { srt { enabled on; } }"));
        EXPECT_FALSE(conf.get_srt_takeover("v"));
        EXPECT_FALSE(conf.get_srt_takeover("absent"));
    }

    if (true) {
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost v { srt { enabled on; takeover on; } }"));
        EXPECT_TRUE(conf.get_srt_takeover("v"));
    }
}

// Keeps the warnings logged while it is installed.
class MockTakeoverWarnLog : public ISrsLog
{
public:
    std::vector<std::string> warnings;
private:
    ISrsLog* saved_;
public:
    MockTakeoverWarnLog() {
        saved_ = _srs_log;
        _srs_log = this;
    }
    virtual ~MockTakeoverWarnLog() {
        _srs_log = saved_;
    }
    virtual srs_error_t initialize() {
        return srs_success;
    }
    virtual void reopen() {
    }
    virtual void log(SrsLogLevel level, const char* /*tag*/, const SrsContextId& /*context_id*/, const char* fmt, va_list args) {
        if (level != SrsLogLevelWarn) {
            return;
        }
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, args);
        warnings.push_back(buf);
    }
    int count(std::string text) {
        int n = 0;
        for (int i = 0; i < (int)warnings.size(); i++) {
            if (warnings[i].find(text) != std::string::npos) {
                n++;
            }
        }
        return n;
    }
};

// Turning the takeover on without an on_publish hook is allowed, but warned about, because then any publisher the
// security rules allow can take a live stream over.
VOID TEST(SrtTakeoverTest, WarnsWhenOnWithoutAPublishHook)
{
    srs_error_t err;

    if (true) {
        MockTakeoverWarnLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost nohooks { srt { enabled on; takeover on; } }"));
        EXPECT_EQ(1, log.count("takeover of nohooks"));
    }

    if (true) {
        MockTakeoverWarnLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost hooksoff { srt { enabled on; takeover on; } "
            "http_hooks { enabled off; on_publish http://127.0.0.1:8085/api/v1/streams; } }"));
        EXPECT_EQ(1, log.count("takeover of hooksoff"));
    }

    if (true) {
        MockTakeoverWarnLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost nourl { srt { enabled on; takeover on; } "
            "http_hooks { enabled on; on_publish; } }"));
        EXPECT_EQ(1, log.count("takeover of nourl"));
    }

    if (true) {
        MockTakeoverWarnLog log;
        MockSrsConfig conf;
        HELPER_ASSERT_SUCCESS(conf.parse(_MIN_OK_CONF "vhost hooked { srt { enabled on; takeover on; } "
            "http_hooks { enabled on; on_publish http://127.0.0.1:8085/api/v1/streams; } } "
            "vhost off { srt { enabled on; } }"));
        EXPECT_EQ(0, log.count("takeover of"));
    }
}

// The old publisher is expired, and the takeover returns only once it is gone, which for a real connection is
// after its on_unpublish hook.
VOID TEST(SrtTakeoverTest, ExpiresThePublisherAndWaitsForIt)
{
    srs_error_t err;

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("takeover-waits"));
    MockTakeoverPublisher old("takeover-old-1", false);
    mock_takeover_publish(&old, req.get());

    HELPER_EXPECT_SUCCESS(srs_srt_takeover_publisher(req.get(), 5 * SRS_UTIME_SECONDS));

    // The old publisher leaves only from its own coroutine, so finding it gone shows the takeover waited for it.
    EXPECT_TRUE(old.expired);
    EXPECT_TRUE(SrsStatistic::instance()->find_client(old.id) == NULL);
}

// A publisher whose stream is already closed is leaving, so the takeover waits for it without interrupting its
// teardown, which may be stopping engines or running its on_unpublish hook.
VOID TEST(SrtTakeoverTest, WaitsForALeavingPublisherWithoutExpiringIt)
{
    srs_error_t err;

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("takeover-leaving"));
    MockTakeoverPublisher old("takeover-old-4", false);
    mock_takeover_publish(&old, req.get());
    SrsStatistic::instance()->on_stream_close(req.get());
    old.leave_soon();

    HELPER_EXPECT_SUCCESS(srs_srt_takeover_publisher(req.get(), 5 * SRS_UTIME_SECONDS));

    EXPECT_FALSE(old.expired);
    EXPECT_TRUE(SrsStatistic::instance()->find_client(old.id) == NULL);
}

// An old publisher that does not go within the bound is not taken over, so the publish is refused as before.
VOID TEST(SrtTakeoverTest, RefusesWhenThePublisherDoesNotGo)
{
    srs_error_t err;

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("takeover-hangs"));
    MockTakeoverPublisher old("takeover-old-2", true);
    mock_takeover_publish(&old, req.get());

    srs_utime_t starttime = srs_update_system_time();
    HELPER_EXPECT_FAILED(srs_srt_takeover_publisher(req.get(), 100 * SRS_UTIME_MILLISECONDS));
    srs_utime_t elapsed = srs_update_system_time() - starttime;

    EXPECT_TRUE(old.expired);
    EXPECT_TRUE(SrsStatistic::instance()->find_client(old.id) != NULL);
    // Half the bound, because ST may end the first of the waits early.
    EXPECT_GE(elapsed, 50 * SRS_UTIME_MILLISECONDS);
}

// Holds the scheduler for a while each time it runs, as a busy server does, so a coroutine's short sleeps take
// longer than asked.
class MockTakeoverBusyServer : public ISrsCoroutineHandler
{
public:
    bool quit;
public:
    MockTakeoverBusyServer() : quit(false) {
    }
    virtual srs_error_t cycle() {
        while (!quit) {
            ::usleep(30 * 1000);
            srs_usleep(1 * SRS_UTIME_MILLISECONDS);
        }
        return srs_success;
    }
};

// The bound is real time, not a count of nominal sleeps, so a busy server does not stretch it.
VOID TEST(SrtTakeoverTest, TheBoundIsRealTime)
{
    srs_error_t err;

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("takeover-busy"));
    MockTakeoverPublisher old("takeover-old-5", true);
    mock_takeover_publish(&old, req.get());

    MockTakeoverBusyServer busy;
    SrsSTCoroutine trd("busy", &busy, _srs_context->get_id());
    HELPER_ASSERT_SUCCESS(trd.start());

    srs_utime_t starttime = srs_update_system_time();
    HELPER_EXPECT_FAILED(srs_srt_takeover_publisher(req.get(), 100 * SRS_UTIME_MILLISECONDS));
    srs_utime_t elapsed = srs_update_system_time() - starttime;
    busy.quit = true;

    // Counting ten nominal 10 ms sleeps here takes ten turns of the busy coroutine, about 400 ms.
    EXPECT_LT(elapsed, 250 * SRS_UTIME_MILLISECONDS);
}

// Runs a takeover on its own coroutine, as a new connection does.
class MockTakeoverCaller : public ISrsCoroutineHandler
{
public:
    SrsRequest* req;
    bool done;
    srs_error_t result;
public:
    MockTakeoverCaller(SrsRequest* r) : req(r), done(false), result(srs_success) {
    }
    virtual ~MockTakeoverCaller() {
        srs_freep(result);
    }
    virtual srs_error_t cycle() {
        result = srs_srt_takeover_publisher(req, 5 * SRS_UTIME_SECONDS);
        done = true;
        return srs_success;
    }
};

// A new connection that is itself interrupted while it waits, such as one kicked or taken over in turn, stops
// waiting with an error instead of waiting out the bound.
VOID TEST(SrtTakeoverTest, StopsWaitingWhenInterrupted)
{
    srs_error_t err;

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("takeover-interrupted"));
    MockTakeoverPublisher old("takeover-old-6", true);
    mock_takeover_publish(&old, req.get());

    MockTakeoverCaller caller(req.get());
    SrsSTCoroutine trd("caller", &caller, _srs_context->get_id());
    HELPER_ASSERT_SUCCESS(trd.start());

    srs_usleep(30 * SRS_UTIME_MILLISECONDS);
    trd.interrupt();

    for (int i = 0; i < 50 && !caller.done; i++) {
        srs_usleep(10 * SRS_UTIME_MILLISECONDS);
    }
    EXPECT_TRUE(caller.done);
    EXPECT_TRUE(caller.result != srs_success);
}

// With no publisher on record there is nothing to take over.
VOID TEST(SrtTakeoverTest, RefusesWithoutAPublisher)
{
    srs_error_t err;

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("takeover-none"));
    HELPER_EXPECT_FAILED(srs_srt_takeover_publisher(req.get(), 100 * SRS_UTIME_MILLISECONDS));
}

// acquire_publish takes a busy stream over only when the takeover is on, and still refuses with the old code while
// the old publisher's source stays busy.
VOID TEST(SrtTakeoverTest, AcquirePublishTakesOverOnlyWhenOn)
{
    srs_error_t err;

    for (int on = 0; on <= 1; on++) {
        MockTakeoverConfig mc;
        HELPER_ASSERT_SUCCESS(mc.conf.parse(std::string(_MIN_OK_CONF) +
            "vhost __defaultVhost__ { srt { enabled on; takeover " + (on ? "on" : "off") + "; } }"));

        std::string name = on ? "acquire-on" : "acquire-off";
        SrsUniquePtr<SrsRequest> req(mock_takeover_request(name));
        MockTakeoverPublisher old(name + "-old", false);
        mock_takeover_publish(&old, req.get());

        SrsContextId cid = _srs_context->get_id();
        SrsMpegtsSrtConn* conn = new SrsMpegtsSrtConn(NULL, -1, "127.0.0.1", 9000);
        conn->req_->vhost = req->vhost;
        conn->req_->app = req->app;
        conn->req_->stream = req->stream;
        conn->srt_source_->can_publish_ = false;

        err = conn->acquire_publish();
        EXPECT_EQ(ERROR_SRT_SOURCE_BUSY, srs_error_code(err));
        srs_freep(err);
        EXPECT_EQ(on == 1, old.expired);

        srs_freep(conn);
        _srs_context->set_id(cid);
    }
}

// Runs acquire_publish on its own coroutine, as a new connection does.
class MockTakeoverAcquirer : public ISrsCoroutineHandler
{
public:
    SrsMpegtsSrtConn* conn;
    bool done;
    int code;
public:
    MockTakeoverAcquirer(SrsMpegtsSrtConn* c) : conn(c), done(false), code(0) {
    }
    virtual srs_error_t cycle() {
        srs_error_t err = conn->acquire_publish();
        code = srs_error_code(err);
        srs_freep(err);
        done = true;
        return srs_success;
    }
};

// A new connection interrupted while it waits for the old publisher is going itself, so acquire_publish refuses it
// at once instead of running the busy checks, which would let it publish if the old one had just gone.
VOID TEST(SrtTakeoverTest, AcquirePublishRefusesWhenInterrupted)
{
    srs_error_t err;

    MockTakeoverConfig mc;
    HELPER_ASSERT_SUCCESS(mc.conf.parse(_MIN_OK_CONF "vhost __defaultVhost__ { srt { enabled on; takeover on; } }"));

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("acquire-interrupted"));
    MockTakeoverPublisher old("acquire-interrupted-old", true);
    mock_takeover_publish(&old, req.get());

    SrsContextId cid = _srs_context->get_id();
    SrsMpegtsSrtConn* conn = new SrsMpegtsSrtConn(NULL, -1, "127.0.0.1", 9000);
    conn->req_->vhost = req->vhost;
    conn->req_->app = req->app;
    conn->req_->stream = req->stream;
    conn->srt_source_->can_publish_ = false;

    if (true) {
        MockTakeoverAcquirer acquirer(conn);
        SrsSTCoroutine trd("acquirer", &acquirer, _srs_context->get_id());
        HELPER_ASSERT_SUCCESS(trd.start());

        srs_usleep(30 * SRS_UTIME_MILLISECONDS);
        trd.interrupt();
        for (int i = 0; i < 50 && !acquirer.done; i++) {
            srs_usleep(10 * SRS_UTIME_MILLISECONDS);
        }

        EXPECT_TRUE(acquirer.done);
        EXPECT_EQ(ERROR_THREAD_INTERRUPED, acquirer.code);
    }

    srs_freep(conn);
    _srs_context->set_id(cid);
}

// A publisher the on_publish hook refuses is turned away before the busy check, so it never expires the publisher
// it would have replaced.
VOID TEST(SrtTakeoverTest, RefusedPublisherNeverTakesOver)
{
    srs_error_t err;

    MockTakeoverConfig mc;
    // Nothing listens on port 1, so the hook fails, which SRS treats as a refusal.
    HELPER_ASSERT_SUCCESS(mc.conf.parse(_MIN_OK_CONF "vhost __defaultVhost__ { srt { enabled on; takeover on; } "
        "http_hooks { enabled on; on_publish http://127.0.0.1:1/refuse; } }"));

    SrsUniquePtr<SrsRequest> req(mock_takeover_request("takeover-refused"));
    MockTakeoverPublisher old("takeover-old-3", false);
    mock_takeover_publish(&old, req.get());

    SrsContextId cid = _srs_context->get_id();
    SrsMpegtsSrtConn* conn = new SrsMpegtsSrtConn(NULL, -1, "127.0.0.1", 9000);
    std::string new_id = _srs_context->get_id().c_str();
    conn->req_->vhost = req->vhost;
    conn->req_->app = req->app;
    conn->req_->stream = req->stream;

    // Make the stream busy, so a takeover placed before the hook would run here and fail the test.
    conn->srt_source_->can_publish_ = false;

    HELPER_EXPECT_FAILED(conn->publishing());
    EXPECT_FALSE(old.expired);
    EXPECT_TRUE(SrsStatistic::instance()->find_client(old.id) != NULL);

    SrsStatistic::instance()->on_disconnect(new_id, srs_success);
    srs_freep(conn);
    _srs_context->set_id(cid);
}

// A bridge whose publish fails or succeeds on demand, so a publish can fail after the SRT source has marked itself
// busy. The source owns and frees it.
class MockSrtReleaseBridge : public ISrsStreamBridge
{
public:
    bool* fail;
public:
    MockSrtReleaseBridge(bool* v) : fail(v) {
    }
    virtual srs_error_t initialize(SrsRequest* /*r*/) {
        return srs_success;
    }
    virtual srs_error_t on_publish() {
        return *fail ? srs_error_new(ERROR_SRT_CONN, "mock bridge publish failed") : srs_success;
    }
    virtual srs_error_t on_frame(SrsSharedPtrMessage* /*frame*/) {
        return srs_success;
    }
    virtual void on_unpublish() {
    }
};

// Gives acquire_publish the server it hands the live source as handler, and removes the stream's sources from the
// global pools when done.
class MockSrtReleaseServer
{
public:
    SrsHybridServer hybrid;
    std::string url;
private:
    SrsHybridServer* saved_;
public:
    MockSrtReleaseServer(std::string u) : url(u) {
        hybrid.register_server(new SrsServerAdapter());
        saved_ = _srs_hybrid;
        _srs_hybrid = &hybrid;
    }
    virtual ~MockSrtReleaseServer() {
        _srs_hybrid = saved_;
        _srs_srt_sources->pool.erase(url);
        _srs_sources->pool.erase(url);
    }
};

static SrsMpegtsSrtConn* mock_srt_release_conn(SrsRequest* req)
{
    SrsMpegtsSrtConn* conn = new SrsMpegtsSrtConn(NULL, -1, "127.0.0.1", 9000);
    conn->req_->vhost = req->vhost;
    conn->req_->app = req->app;
    conn->req_->stream = req->stream;
    srs_error_t err = _srs_srt_sources->fetch_or_create(conn->req_, conn->srt_source_);
    srs_freep(err);
    return conn;
}

// A publish that fails after the SRT source marked itself busy releases the stream, so the next publisher is
// accepted instead of the stream staying busy until SRS restarts.
VOID TEST(SrtPublishReleaseTest, FailedPublishReleasesTheStream)
{
    srs_error_t err;

    MockTakeoverConfig mc;
    HELPER_ASSERT_SUCCESS(mc.conf.parse(_MIN_OK_CONF "vhost __defaultVhost__ { srt { enabled on; srt_to_rtmp off; } }"));
    SrsUniquePtr<SrsRequest> req(mock_takeover_request("release-failed"));
    MockSrtReleaseServer server(req->get_stream_url());
    SrsContextId cid = _srs_context->get_id();

    bool fail = true;
    SrsMpegtsSrtConn* first = mock_srt_release_conn(req.get());
    std::string first_id = _srs_context->get_id().c_str();
    first->srt_source_->set_bridge(new MockSrtReleaseBridge(&fail));
    err = first->publishing();
    EXPECT_TRUE(srs_error_desc(err).find("mock bridge publish failed") != std::string::npos);
    srs_freep(err);
    EXPECT_TRUE(first->srt_source_->can_publish());
    SrsStatistic::instance()->on_disconnect(first_id, srs_success);
    srs_freep(first);

    fail = false;
    SrsMpegtsSrtConn* second = mock_srt_release_conn(req.get());
    HELPER_EXPECT_SUCCESS(second->acquire_publish());
    second->release_publish();
    srs_freep(second);

    _srs_context->set_id(cid);
}

// Runs publishing() on its own coroutine, as a connection does.
class MockSrtReleasePublisher : public ISrsCoroutineHandler
{
public:
    SrsMpegtsSrtConn* conn;
    bool done;
public:
    MockSrtReleasePublisher(SrsMpegtsSrtConn* c) : conn(c), done(false) {
    }
    virtual srs_error_t cycle() {
        srs_error_t err = conn->publishing();
        srs_freep(err);
        done = true;
        return srs_success;
    }
};

// A publisher refused because the stream is busy, after a takeover that timed out, or because it was interrupted
// while it waited to take over, leaves the source it was refused for to the publisher that holds it.
VOID TEST(SrtPublishReleaseTest, RefusalNeverReleasesTheHolder)
{
    srs_error_t err;

    for (int arm = 0; arm < 3; arm++) {
        MockTakeoverConfig mc;
        HELPER_ASSERT_SUCCESS(mc.conf.parse(std::string(_MIN_OK_CONF) + "vhost __defaultVhost__ { srt { enabled on; "
            "srt_to_rtmp off; takeover " + (arm == 0 ? "off" : "on") + "; } }"));
        const char* names[] = {"release-busy", "release-timeout", "release-interrupted"};
        SrsUniquePtr<SrsRequest> req(mock_takeover_request(names[arm]));
        MockSrtReleaseServer server(req->get_stream_url());
        SrsContextId cid = _srs_context->get_id();

        // Another publisher holds the stream, and does not go when told to.
        MockTakeoverPublisher holder(std::string(names[arm]) + "-holder", true);
        mock_takeover_publish(&holder, req.get());
        SrsMpegtsSrtConn* conn = mock_srt_release_conn(req.get());
        std::string conn_id = _srs_context->get_id().c_str();
        conn->srt_source_->can_publish_ = false;

        if (arm < 2) {
            HELPER_EXPECT_FAILED(conn->publishing());
        } else {
            MockSrtReleasePublisher publisher(conn);
            SrsSTCoroutine trd("publisher", &publisher, _srs_context->get_id());
            HELPER_ASSERT_SUCCESS(trd.start());
            srs_usleep(30 * SRS_UTIME_MILLISECONDS);
            trd.interrupt();
            for (int i = 0; i < 50 && !publisher.done; i++) {
                srs_usleep(10 * SRS_UTIME_MILLISECONDS);
            }
            EXPECT_TRUE(publisher.done);
        }
        EXPECT_FALSE(conn->srt_source_->can_publish()) << names[arm];
        EXPECT_EQ(arm > 0, holder.expired) << names[arm];

        conn->srt_source_->can_publish_ = true;
        SrsStatistic::instance()->on_disconnect(conn_id, srs_success);
        srs_freep(conn);
        _srs_context->set_id(cid);
    }
}

