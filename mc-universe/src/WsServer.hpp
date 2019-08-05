
#include "common.hpp"

#include <vector>
#include <map>

// websocket includes
#include "server_ws.hpp"

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsConfig = SimpleWeb::SocketServer<SimpleWeb::WS>::Config;

// meaningful-chaos includes
#include "Grid.hpp"

// @Incomplete: Grid should be a shared_ptr: https://en.cppreference.com/w/cpp/memory/shared_ptr
unordered_map<shared_ptr<WsServer::Connection>, shared_ptr<Grid>> connection_grid;

unordered_multimap<shared_ptr<WsServer::Connection>, string> subed_to;
unordered_multimap<string, shared_ptr<WsServer::Connection>> subs;

// api
// WsServer &init_server (unsigned short port);
void add_endpoints (WsServer &server);
void emit(string event, string data);
void subscribe (shared_ptr<WsServer::Connection> conn, string event);
void unsubscribe (shared_ptr<WsServer::Connection> conn, string event);
void unsubscribe_all (shared_ptr<WsServer::Connection> conn);

// ===== SERVER =====

void add_endpoints (WsServer &server) {

    // ==== EvENTS ====

    auto &event_emitter = server.endpoint["^/events/?$"];
    // event_emitter.on_open =
    // [](shared_ptr<WsServer::Connection> conn) {}

    event_emitter.on_close =
    [](shared_ptr<WsServer::Connection> conn, int, const std::string &) {
        unsubscribe_all(conn);
    };

    event_emitter.on_error =
    [](shared_ptr<WsServer::Connection> conn, const error_code) {
        unsubscribe_all(conn);
    };

    event_emitter.on_message =
    [](shared_ptr<WsServer::Connection> conn, shared_ptr<WsServer::InMessage> msg) {
        string str = msg->string();
        auto len = str.length();
        if (len < 2 || str[1] != ':') return; // do nothing
        char cmd = str[0];
        string event = str.substr(2, 255);
        switch (cmd) {
        case 'u': // unsubscribe an event
            subscribe(conn, event);
            break;

        case 's': // subscribe to an event
            unsubscribe(conn, event);
            break;

        case 'U': // unsubscribe to all events
            unsubscribe_all(conn);
            break;

        default:
            // unknown command
            cout << "unknown command: '" << cmd << "' for event: " << event << endl;
        }
    };

    // ==== GRID ====

    // https://www.regextester.com/97058
    auto &grid = server.endpoint["^/grid/([0-9]+)/([0-9]+)/([a-zA-Z0-9_-]+)/?$"];

    grid.on_open =
    [](shared_ptr<WsServer::Connection> conn) {
        cout << "grid(" << conn->path_match[1] << '|' << conn->path_match[2] << '|' << conn->path_match[3] << ").on_open" << endl;

        auto width = stoul(conn->path_match[1]);
        auto height = stoul(conn->path_match[2]);
        auto id = conn->path_match[3];

        auto it = connection_grid.find(conn);
        if (it == connection_grid.end()) {
            cout << "making grid: " << width << "x" << height << endl;
            auto grid = make_shared<Grid>(id, width, height);
            connection_grid[conn] = grid;
        }
    };

    grid.on_close =
    [](shared_ptr<WsServer::Connection> conn, int, const std::string &) {
        cout << "grid(" << conn->path_match[1] << '|' << conn->path_match[2] << '|' << conn->path_match[3] << ").on_close" << endl;
        connection_grid.erase(conn);
    };

    grid.on_error =
    [](shared_ptr<WsServer::Connection> conn, const error_code) {
        cout << "grid(" << conn->path_match[1] << '|' << conn->path_match[2] << '|' << conn->path_match[3] << ").on_error" << endl;
        connection_grid.erase(conn);
    };

    grid.on_message =
    [](shared_ptr<WsServer::Connection> conn, shared_ptr<WsServer::InMessage> msg) {
        cout << "grid(" << conn->path_match[1] << '|' << conn->path_match[2] << '|' << conn->path_match[3] << ").on_message" << endl;
        auto it = connection_grid.find(conn);
        assert(it != connection_grid.end());
        auto grid = it->second;

        // @Incomplete: do stuff with the grid msg
        auto data_str = msg->string();
        uint16_t* dd = (uint16_t*) data_str.data();
        cout << "data.length:" << data_str.length() << endl;

        // accumulate the grid px (TODO)
        vector<Initialiser*> inits;
        grid->accumulate(dd, inits);

        // run all sequences on every overflow value (TODO)
        // for (Initialiser *init; inits) {
        for (auto init = inits.begin(); init != inits.end(); init++) {
            cout << "!!! x: " << (*init)->x << " y: " << (*init)->y << " value: " << (*init)->value << endl;
            // for each sequence, run it.
            delete *init;
        }

        // query the universe for nearest points to those values (TODO)

        // emit any events back to the client
        emit("test", "testing data");
    };
}


void emit(string event, string data) {
    string msg = event + ':' + data;
    auto it = subs.find(event);
    if (it != subs.end()) {
        it->second->send(msg);
    }
}

void subscribe (shared_ptr<WsServer::Connection> conn, string event) {
    subs.insert(make_pair(event, conn));
    subed_to.insert(make_pair(conn, event));
}

void unsubscribe (shared_ptr<WsServer::Connection> conn, string event) {
    for (auto it = subs.find(event); it != subs.end(); ) {
        if (it->second == conn) subs.erase(it);
        else it++;
    }

    for (auto it = subed_to.find(conn); it != subed_to.end(); ) {
        if (it->second == event) subed_to.erase(it);
        else it++;
    }
}

void unsubscribe_all (shared_ptr<WsServer::Connection> conn) {
    for (auto it = subs.begin(); it != subs.end(); ) {
        if(it->second == conn) it = subs.erase(it);
        else ++it;
    }

    for (auto it = subed_to.begin(); it != subed_to.end(); ) {
        if(it->first == conn) it = subed_to.erase(it);
        else ++it;
    }
}

#ifdef BUILD_TESTING
// #define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_RUNNER // I'll provide the main.
#include <catch2/catch.hpp>

#include "client_ws.hpp"

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;



int main (int argc, char* argv[]) {
    WsServer server;
    server.config.port = 1177;
    server.config.thread_pool_size = 4;

    add_endpoints(server);

    thread server_thread([&server]() { server.start(); });

    cout << "listening on ws://localhost:" << server.config.port << endl;

    int result = Catch::Session().run(argc, argv);

    server.stop();
    server_thread.join();

    cout << "server stopped.." << endl;
    return result;
}

TEST_CASE("server creates a grid", "[server][grid]" ) {
    WsClient client("localhost:1177/grid/8/8/lala");

    client.on_message = [&](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        REQUIRE(in_message->string() == "fragmented message");

        ++client_callback_count;

        connection->send_close(1000);
    };

    client.on_open = [&](shared_ptr<WsClient::Connection> connection) {
        ++client_callback_count;
        REQUIRE(!closed);

        // connection->send("fragmented", nullptr, 1);
        // connection->send(" ", nullptr, 0);
        // connection->send("message", nullptr, 128);
    };

    client.on_close = [&](shared_ptr<WsClient::Connection> /*connection*/, int /*status*/, const string & /*reason*/) {
        REQUIRE(!closed);
        closed = true;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        cerr << ec.message() << endl;
        REQUIRE(false);
    };

    thread client_thread([&client]() {
        client.start();
    });

    while(!closed)
        this_thread::sleep_for(chrono::milliseconds(5));

    client.stop();
    client_thread.join();

    REQUIRE(client_callback_count == 2);
    REQUIRE(server_callback_count == 1);
}

#endif // BUILD_TESTING
