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
#include "server_ws.hpp"

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsConfig = SimpleWeb::SocketServer<SimpleWeb::WS>::Config;
using WsConnection = SimpleWeb::SocketServer<SimpleWeb::WS>::Connection;

// meaningful-chaos includes
#include "Grid.hpp"

// typedef WsServer::Connection Conn;

std::unordered_map<shared_ptr<WsServer::Connection>, shared_ptr<Grid>> connection_grid;

struct subscription {
    shared_ptr<WsServer::Connection> conn;
    string event;

    subscription (shared_ptr<WsServer::Connection> _conn, string _event) : conn(_conn), event(_event) {};

    friend std::ostream& operator<<(std::ostream& os,const subscription& s) {
        os << "Sub(" << s.conn << " -> " << s.event << ")" << endl;
        return os;
    }
};

struct key_conn{};
struct key_event{};
struct key_conn_event:composite_key<
  subscription,
  // BOOST_MULTI_INDEX_MEMBER(subscription, shared_ptr<WsServer::Connection>, conn),
  member<subscription, shared_ptr<WsServer::Connection>, &subscription::conn>,
  // BOOST_MULTI_INDEX_MEMBER(subscription, std::string, event)
  member<subscription, string, &subscription::event>
>{};

typedef multi_index_container<
  subscription,
  indexed_by<
    // select by unique connection
    hashed_unique<
        tag<key_conn>,
        member<subscription, shared_ptr<WsServer::Connection>, &subscription::conn>
    >,
    // select by event
    hashed_non_unique<
        tag<key_event>,
        member<subscription, string, &subscription::event>
    >,
    // select by event and connection (for subscribe, unsubscribe)
    hashed_unique<
        tag<key_conn_event>,
        composite_key<
            subscription,
            member<subscription, shared_ptr<WsServer::Connection>, &subscription::conn>,
            member<subscription, string, &subscription::event>
        >
    >
  >
> Subscriptions;

Subscriptions subs;

// api
void add_endpoints (WsServer &server);
void emit(string event, string data);
void subscribe (shared_ptr<WsServer::Connection>& conn, string event);
void unsubscribe (shared_ptr<WsServer::Connection>& conn, string event);
void unsubscribe_all (shared_ptr<WsServer::Connection>& conn);

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
    [](shared_ptr<WsServer::Connection> conn, int, const string &) {
        #ifdef BUILD_TESTING
        event_emitter_callback_count++;
        #endif // BUILD_TESTING

        unsubscribe_all(conn);
    };

    event_emitter.on_error =
    [](shared_ptr<WsServer::Connection> conn, const error_code) {
        #ifdef BUILD_TESTING
        event_emitter_callback_count++;
        #endif // BUILD_TESTING

        unsubscribe_all(conn);
    };

    event_emitter.on_message =
    [](shared_ptr<WsServer::Connection> conn, shared_ptr<WsServer::InMessage> msg) {
        #ifdef BUILD_TESTING
        COUT << "events(" << conn.get() << ").message" << endl;
        event_emitter_callback_count++;
        #endif // BUILD_TESTING

        string str = msg->string();
        auto len = str.length();
        if (len < 2 || str[1] != ':') return; // do nothing
        char cmd = str[0];
        string event = str.substr(2, 255);
        switch (cmd) {
        case 's': // unsubscribe an event
            subscribe(conn, event);
            #ifdef BUILD_TESTING
            COUT << "events(" << conn.get() << ").subscribe(" << event << ')' << endl;
            #endif // BUILD_TESTING
            break;

        case 'u': // subscribe to an event
            unsubscribe(conn, event);
            #ifdef BUILD_TESTING
            COUT << "events(" << conn.get() << ").unsubscribe(" << event << ')' << endl;
            #endif // BUILD_TESTING
            break;

        case 'U': // unsubscribe to all events
            unsubscribe_all(conn);
            #ifdef BUILD_TESTING
            COUT << "events(" << conn.get() << ").unsubscribe_all" << event << ')' << endl;
            #endif // BUILD_TESTING
            break;

        default:
            // unknown command
            COUT << "unknown command: '" << cmd << "' for event: " << event << endl;
        }
    };

    // ==== GRID ====

    auto &grid = server.endpoint["^/grid/([0-9]+)/([0-9]+)/([a-zA-Z0-9_-]+)/?$"];

    grid.on_open =
    [](shared_ptr<WsServer::Connection> conn) {
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
    [](shared_ptr<WsServer::Connection> conn, int, const string &) {
        #ifdef BUILD_TESTING
        COUT << "grid(" << conn->path_match[1] << '/' << conn->path_match[2] << '/' << conn->path_match[3] << ").on_close" << endl;
        grid_callback_count++;
        #endif // BUILD_TESTING

        connection_grid.erase(conn);
    };

    grid.on_error =
    [](shared_ptr<WsServer::Connection> conn, const error_code) {
        #ifdef BUILD_TESTING
        COUT << "grid(" << conn->path_match[1] << '/' << conn->path_match[2] << '/' << conn->path_match[3] << ").on_error" << endl;
        grid_callback_count++;
        #endif // BUILD_TESTING

        connection_grid.erase(conn);
    };

    grid.on_message =
    [](shared_ptr<WsServer::Connection> conn, shared_ptr<WsServer::InMessage> msg) {
        #ifdef BUILD_TESTING
        COUT << "grid(" << conn->path_match[1] << '/' << conn->path_match[2] << '/' << conn->path_match[3] << ").on_message" << endl;
        grid_callback_count++;
        #endif // BUILD_TESTING

        auto it = connection_grid.find(conn);
        assert(it != connection_grid.end());
        auto grid = it->second;

        // @Incomplete: do stuff with the grid msg
        auto data_str = msg->string();
        uint16_t* dd = (uint16_t*) data_str.data();
        COUT << "data.length:" << data_str.length() << endl;

        // accumulate the grid px (TODO)
        vector<Initialiser*> inits;
        grid->accumulate(dd, inits);

        // run all sequences on every overflow value (TODO)
        string event = "init:grid/" + (string)conn->path_match[1] + '/' + (string)conn->path_match[2] + '/' + (string)conn->path_match[3];
        for (auto init = inits.begin(); init != inits.end(); init++) {
            COUT << "!!! x: " << (*init)->x << " y: " << (*init)->y << " value: " << (*init)->value << endl;

            StringBuffer s;
            Writer<StringBuffer> j(s);

            j.StartObject();
            j.Key("x");
            j.Double((*init)->x);
            j.Key("y");
            j.Double((*init)->y);
            j.Key("x");
            j.Double((*init)->value);
            j.EndObject();

            emit(event, s.GetString());
            // for each sequence, run it.
            delete *init;
        }

        // query the universe for nearest points to those values (TODO)

        // emit any events back to the client
        emit("test", "testing data");
    };
}


void emit(const string event, const string data) {
    string msg = event + "::" + data;
    auto& events = subs.get<key_event>();
    for (auto it = events.find(event); it != events.end(); it++) {
        // COUT << it->conn.get() << "," << it->event << endl;
        it->conn->send(msg);
    }
}

void subscribe (shared_ptr<WsServer::Connection>& conn, const string event) {
    subs.insert(subscription(conn, event));
}

void unsubscribe (shared_ptr<WsServer::Connection>& conn, const string event) {
    auto& events = subs.get<key_conn_event>();
    auto it = events.find(make_tuple(conn, event));
    if (it != events.end()) events.erase(it);
}

void unsubscribe_all (shared_ptr<WsServer::Connection>& conn) {
    auto& events = subs.get<key_conn>();
    auto it = events.find(conn);
    if (it != events.end()) events.erase(it);
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

    server.stop();
    server_thread.join();

    COUT << "server stopped.." << endl;
    return result;
}

TEST_CASE("server manipulates an 8x8 grid", "[server][grid]" ) {
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

        // send them
        for (int i = 0; i < 20; i++) {
            // generate random bytes
            string bytes(len, 0);
            uint16_t* px = (uint16_t*) bytes.data();
            for (auto x = 0; x < width; x++) {
                for (auto y = 0; y < height; y++) {
                    double v = rc4rand() / 0xFFFFFFFF;
                    px[y * width + x] = (uint16_t) (v * 20);
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

        }
    };

    eclient.on_open = [&](shared_ptr<WsClient::Connection> conn) {
        ++eclient_callback_count;
        REQUIRE(!eclosed);

        // subscribe to events
        conn->send("u:init");
        conn->send("u:does_not_exist");
        conn->send("u:event with spaces");
        conn->send("s:init");
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
    // no messages + close
    REQUIRE(eclient_callback_count == 1);
    // gopen + 20 messages + gclose (22)
    REQUIRE(grid_callback_count == 22);
    // eopen + 3 subs + eclose (5)
    REQUIRE(event_emitter_callback_count == (5));

    // TODO: check one grid exists and that it's got the right properties.
}

#endif // BUILD_TESTING
#endif // __WSSERVER_HPP__
