#ifndef __WSSERVER_HPP__
#define __WSSERVER_HPP__
#define CATCH_CONFIG_RUNNER // I'll provide the main.

#include "common.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace boost::multi_index;
// using namespace boost; // conflicts with std::shared_ptr

// websocket includes
#include "Simple-WebSocket-Server/mutex.hpp"
#include "Simple-WebSocket-Server/server_ws.hpp"

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsConfig = SimpleWeb::SocketServer<SimpleWeb::WS>::Config;
using WsConnection = SimpleWeb::SocketServer<SimpleWeb::WS>::Connection;
using LockGuard = SimpleWeb::LockGuard;
using Mutex = SimpleWeb::Mutex;

// meaningful-chaos includes
#include "Grid.hpp"

std::unordered_map<shared_ptr<WsConnection>, shared_ptr<Grid>> connection_grid;

struct Subscription {
    WsConnection* conn;
    string event;

    Subscription (WsConnection* _conn, string _event) : conn(_conn), event(_event) {};

    friend std::ostream& operator<<(std::ostream& os, const Subscription& s) {
        os << "Sub(" << s.conn << " -> " << s.event << ")" << endl;
        return os;
    }
};

struct by_conn{};
struct by_event{};
struct by_conn_event:composite_key<
  Subscription,
  member<Subscription, WsConnection*, &Subscription::conn>,
  member<Subscription, string, &Subscription::event>
>{};

typedef multi_index_container<
  Subscription,
  indexed_by<
    // select by unique connection
    hashed_non_unique<
        tag<by_conn>,
        member<Subscription, WsConnection*, &Subscription::conn>
    >,
    // select by event
    hashed_non_unique<
        tag<by_event>,
        member<Subscription, string, &Subscription::event>
    >,
    // select by event and connection (for subscribe, unsubscribe)
    hashed_unique<
        tag<by_conn_event>,
        composite_key<
            Subscription,
            member<Subscription, WsConnection*, &Subscription::conn>,
            member<Subscription, string, &Subscription::event>
        >
    >
  >
> Subscriptions;

Subscriptions subs;
Mutex subs_mutex;

auto&& subs_by_event = subs.get<by_event>();
auto&& subs_by_conn_event = subs.get<by_conn_event>();
auto&& subs_by_conn = subs.get<by_conn>();


// api
void add_endpoints (WsServer &server);
void emit(string event, string data);
void subscribe (WsConnection* conn, string event);
void unsubscribe (WsConnection* conn, string event);
void unsubscribe_all (WsConnection* conn);

// ===== SERVER =====
#ifdef BUILD_TESTING
atomic<int> event_emitter_callback_count(0);
atomic<int> grid_callback_count(0);
vector<string> events_emitted;
#endif // BUILD_TESTING

void add_endpoints (WsServer &server) {

    // ==== EvENTS ====

    auto &event_emitter = server.endpoint["^/events/?$"];

    event_emitter.on_close =
    [](shared_ptr<WsConnection> conn, int, const string &) {
        #ifdef BUILD_TESTING
        COUT << "emitter(" << conn.get() << ").on_close" << endl;
        event_emitter_callback_count++;
        #endif // BUILD_TESTING

        unsubscribe_all(conn.get());
    };

    event_emitter.on_error =
    [](shared_ptr<WsConnection> conn, const error_code) {
        #ifdef BUILD_TESTING
        COUT << "emitter(" << conn.get() << ").on_error" << endl;
        event_emitter_callback_count++;
        #endif // BUILD_TESTING

        unsubscribe_all(conn.get());
    };

    event_emitter.on_message =
    [](shared_ptr<WsConnection> conn, shared_ptr<WsServer::InMessage> msg) {
        #ifdef BUILD_TESTING
        event_emitter_callback_count++;
        #endif // BUILD_TESTING

        string str = msg->string();
        auto len = str.length();
        if (len < 2 || str[1] != ':') return; // do nothing
        char cmd = str[0];
        string event = str.substr(2, 255);
        switch (cmd) {
        case 's': {
            // unsubscribe an event
            #ifdef BUILD_TESTING
            COUT << "emitter(" << conn.get() << ").subscribe(" << event << ')' << endl;
            #endif // BUILD_TESTING

            subscribe(conn.get(), event);
            break;
        } case 'u': {
            // subscribe to an event
            #ifdef BUILD_TESTING
            COUT << "emitter(" << conn.get() << ").unsubscribe(" << event << ')' << endl;
            #endif // BUILD_TESTING

            unsubscribe(conn.get(), event);
            break;
        } case 'U': {
            // unsubscribe to *all* events
            #ifdef BUILD_TESTING
            COUT << "emitter(" << conn.get() << ").unsubscribe_all" << event << ')' << endl;
            #endif // BUILD_TESTING

            unsubscribe_all(conn.get());
            break;
        } case 'L': {
            // list subscriptions
            #ifdef BUILD_TESTING
            COUT << "emitter(" << conn.get() << ").list_all" << event << ')' << endl;
            #endif // BUILD_TESTING

            auto&& events = subs.get<by_conn>(); // is this necessary to do every time? can this be cached?
            auto it = events.find(conn.get());
            if (it == events.end()) COUT << "no subscriptions" << endl; else
            for (; it != events.end(); it++) {
                // COUT << "sub:" << it->event << endl;
            }
            break;
        }

        default:
            // unknown command
            COUT << "unknown command: '" << cmd << "' for event: " << event << endl;
        }
    };

    // ==== GRID ====

    auto &grid = server.endpoint["^/grid/([0-9]+)/([0-9]+)/([a-zA-Z0-9_-]+)/?$"];

    grid.on_open =
    [](shared_ptr<WsConnection> conn) {
        #ifdef BUILD_TESTING
        COUT << "grid(" << conn->path_match[1] << '/' << conn->path_match[2] << '/' << conn->path_match[3] << ").on_open" << endl;
        grid_callback_count++;
        #endif // BUILD_TESTING

        auto width = stoul(conn->path_match[1]);
        auto height = stoul(conn->path_match[2]);
        auto id = conn->path_match[3];

        auto it = connection_grid.find(conn);
        if (it == connection_grid.end()) {
            COUT << "making grid: " << width << "x" << height << endl;
            auto grid = make_shared<Grid>(id, width, height);
            connection_grid[conn] = grid;
        }
    };

    grid.on_close =
    [](shared_ptr<WsConnection> conn, int, const string &) {
        #ifdef BUILD_TESTING
        COUT << "grid(" << conn->path_match[1] << '/' << conn->path_match[2] << '/' << conn->path_match[3] << ").on_close" << endl;
        grid_callback_count++;
        #endif // BUILD_TESTING

        connection_grid.erase(conn);
    };

    grid.on_error =
    [](shared_ptr<WsConnection> conn, const error_code) {
        #ifdef BUILD_TESTING
        COUT << "grid(" << conn->path_match[1] << '/' << conn->path_match[2] << '/' << conn->path_match[3] << ").on_error" << endl;
        grid_callback_count++;
        #endif // BUILD_TESTING

        connection_grid.erase(conn);
    };

    grid.on_message =
    [](shared_ptr<WsConnection> conn, shared_ptr<WsServer::InMessage> msg) {
        #ifdef BUILD_TESTING
        COUT << "grid(" << conn->path_match[1] << '/' << conn->path_match[2] << '/' << conn->path_match[3] << ").on_message" << endl;
        grid_callback_count++;
        #endif // BUILD_TESTING

        auto it = connection_grid.find(conn);
        assert(it != connection_grid.end());
        auto grid = it->second;

        // @Incomplete: do stuff with the grid msg
        auto data_str = msg->string();
        u16* dd = (u16*) data_str.data();

        // accumulate the grid px
        // @Optimise: use absil stack vector here instead and init them in stack space instead of thrashing the memory
        vector<Initialiser*> inits;
        grid->accumulate(dd, inits);

        if (inits.size() > 0) {
            // run all sequences on every overflow value (TODO)
            string event = "init:grid/" + (string)conn->path_match[1] + '/' + (string)conn->path_match[2] + '/' + (string)conn->path_match[3];
            for (auto i = inits.begin(); i != inits.end(); i++) {
                StringBuffer s;
                Writer<StringBuffer> j(s);
                auto init = *i;

                j.StartObject();
                j.Key("x");
                j.Double(init->origin.x);
                j.Key("y");
                j.Double(init->origin.y);
                j.Key("v");
                j.Double(init->value);
                j.EndObject();

                emit(event, s.GetString());

                // @Incomplete: for each sequence, run them with this initial data.
                //              for now, though just generate random values
                const u16 dimensions = 20;
                // fill with random values:
                float* query = (float*) calloc(sizeof(float), dimensions);
                for (int i = 0; i < dimensions; i++) {
                    query[i] = random_number(-1000, 1000);
                }

                // @Inocomplete: query the universe for the value using inner product distance function.

                delete *i;
            }

            // query the universe for nearest points to those values (TODO)

            // emit any events back to the client
        }
    };
}


void emit(const string event, const string data) {
    string msg = event + "::" + data;
    COUT << "emit(): " << msg << endl;
    auto it = subs_by_event.find(event);
    // if (it == subs_by_event.end()) COUT << "no listeners" << endl; else
    for (; it != subs_by_event.end(); it++) {
        COUT << "emit(" << it->conn << ',' << it->event << ')' << endl;
        it->conn->send(msg);
    }
}

void subscribe (WsConnection* conn, const string event) {
    LockGuard lock(subs_mutex);
    subs.insert(Subscription(conn, event));
}

void unsubscribe (WsConnection* conn, const string event) {
    auto it = subs_by_conn_event.find(make_tuple(conn, event));
    if (it != subs_by_conn_event.end()) {
        LockGuard lock(subs_mutex);
        subs_by_conn_event.erase(it);
    }
}

void unsubscribe_all (WsConnection* conn) {
    auto it = subs_by_conn.find(conn);
    if (it != subs_by_conn.end()) {
        LockGuard lock(subs_mutex);
        subs_by_conn.erase(it);
    }
}

#ifdef BUILD_TESTING

#include "client_ws.hpp"

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using OutMessage = WsClient::OutMessage;



int main (int argc, char* argv[]) {
    WsServer server;
    server.config.port = 1177;
    // server.config.thread_pool_size = 4;

    add_endpoints(server);

    thread server_thread([&server]() { server.start(); });

    cout << "listening on ws://localhost:" << server.config.port << endl;

    // give it time to bind..
    this_thread::sleep_for(chrono::milliseconds(100));

    int result = Catch::Session().run(argc, argv);

    // give it time to finish up (justin timberlake)..
    this_thread::sleep_for(chrono::milliseconds(1000));

    // @Incomplete: I really want a function that stops accepting new connections and then stops when the last one disconnects.
    server.stop();
    server_thread.join();

    COUT << "server stopped.." << endl;
    return result;
}

// TEST_CASE("hash tables actually work", "") {
//     // subscribe(conn, "test");
// }

TEST_CASE("server manipulates an 8x8 grid", "[server][grid]") {
    int width = 8;
    int height = 8;

    WsClient gclient("localhost:1177/grid/8/8/lala");
    WsClient eclient("localhost:1177/events");

    atomic<int> gclient_callback_count(0);
    atomic<bool> gclosed(false);

    gclient.on_message = [&](shared_ptr<WsClient::Connection> conn, shared_ptr<WsClient::InMessage> msg) {
        REQUIRE(msg->string() == "should not receive a message from the grid!");
        REQUIRE(!gclosed);

        ++gclient_callback_count;

        conn->send_close(1000);
    };

    gclient.on_open = [&](shared_ptr<WsClient::Connection> conn) {
        ++gclient_callback_count;
        REQUIRE(!gclosed);
        auto len = width * height * 2;

        auto out = std::make_shared<OutMessage>(len);

        // seed the prng so the results are always the same.
        rc4srand(1234);

        // send them
        for (int i = 0; i < 20; i++) {
            // generate random bytes
            string bytes(len, 0);
            u16* px = (u16*) bytes.data();
            for (auto x = 0; x < width; x++) {
                for (auto y = 0; y < height; y++) {
                    px[y * width + x] = (u16) random_number(0, 22);
                }
            }

            conn->send(bytes);
            this_thread::sleep_for(chrono::milliseconds(1));
        }

        conn->send_close(1000);
    };

    gclient.on_close = [&](shared_ptr<WsClient::Connection> /*conn*/, int /*status*/, const string & /*reason*/) {
        REQUIRE(!gclosed);
        gclosed = true;
        eclient.stop();
    };

    gclient.on_error = [](shared_ptr<WsClient::Connection> /*conn*/, const SimpleWeb::error_code &ec) {
        cerr << ec.message() << endl;
        REQUIRE(false);
    };

    atomic<bool> eclosed(false);
    atomic<int> eclient_callback_count(0);

    eclient.on_message = [&](shared_ptr<WsClient::Connection> conn, shared_ptr<WsClient::InMessage> msg) {
        REQUIRE(!eclosed);

        ++eclient_callback_count;

        string str(msg->string());

        COUT << "event received:" << str << endl;
        if (str == "exit") {
            conn->send_close(1000);
        } else {
            // emit(): init:grid/8/8/lala::{"x":4.0,"y":0.0,"v":267.0}
            // emit(): init:grid/8/8/lala::{"x":1.0,"y":6.0,"v":265.0}
            // emit(): init:grid/8/8/lala::{"x":0.0,"y":7.0,"v":272.0}
        }
    };

    eclient.on_open = [&](shared_ptr<WsClient::Connection> conn) {
        ++eclient_callback_count;
        REQUIRE(!eclosed);

        // subscribe to events
        conn->send("u:init");
        conn->send("u:does_not_exist");
        conn->send("u:event with spaces");
        conn->send("s:test");
        conn->send("s:init:grid/8/8/lala");
    };

    eclient.on_close = [&](shared_ptr<WsClient::Connection> /*conn*/, int /*status*/, const string & /*reason*/) {
        REQUIRE(!eclosed);
        eclosed = true;
    };

    eclient.on_error = [](shared_ptr<WsClient::Connection> /*conn*/, const SimpleWeb::error_code &ec) {
        cerr << ec.message() << endl;
        REQUIRE(false);
    };

    thread eclient_thread([&eclient]() { eclient.start(); });
    thread gclient_thread([&gclient]() { gclient.start(); });

    while (!gclosed && !eclosed) {
        this_thread::sleep_for(chrono::milliseconds(5));
    }

    // not doing this. they should close theirselves after performing all the steps in the test.
    // eclient.stop();
    // gclient.stop();
    eclient_thread.join();
    gclient_thread.join();

    // no messages + close
    REQUIRE(gclient_callback_count == 1);
    // init:grid/8/8/lala(3) + close
    REQUIRE(eclient_callback_count == 4);
    // gopen + 20 messages + gclose (22)
    REQUIRE(grid_callback_count == 22);
    // 3 unsubs + 2 subs + eclose (6)
    REQUIRE(event_emitter_callback_count == 6);

    // TODO: check one grid exists and that it's got the right properties.
}

#endif // BUILD_TESTING
#endif // __WSSERVER_HPP__
