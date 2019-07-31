#include <stdio.h>

#include <map>
#include <vector>

using namespace std;

// websocket includes
// #include "client_ws.hpp"
#include "server_ws.hpp"

// meaningful-chaos includes
#include "Grid.hpp"

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
// using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

// @Incomplete: Grid should be a shared_ptr: https://en.cppreference.com/w/cpp/memory/shared_ptr
map<shared_ptr<WsServer::Connection>, shared_ptr<Grid>> connection_grid;


// api
void emit(WsServer::Endpoint &event_emitter, string event, string data);


int main () {
    WsServer server;
    server.config.port = 1177;

    // event emitter paths
    auto &event_emitter = server.endpoint["^/event/([a-zA-Z0-9_-]+)/?$"];

    // https://www.regextester.com/97058
    auto &grid = server.endpoint["^/grid/([0-9]+)/([0-9]+)/([a-zA-Z0-9_-]+)/?$"];
    grid.on_handshake =
    grid.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap & /*response_header*/) {
        cout << "handshake!" << endl;
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };

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
    [&event_emitter](shared_ptr<WsServer::Connection> conn, shared_ptr<WsServer::InMessage> msg) {
        cout << "grid(" << conn->path_match[1] << '|' << conn->path_match[2] << '|' << conn->path_match[3] << ").on_message" << endl;
        auto it = connection_grid.find(conn);
        assert(it != connection_grid.end());
        auto grid = it->second;

        // @Incomplete: do stuff with the grid msg
        auto data_str = msg->string();
        uint16_t *dd = (uint16_t*) data_str.data();
        cout << "data.length:" << data_str.length() << endl;

        // accumulate the grid px (TODO)
        vector<Initialiser *> inits;
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
        emit(event_emitter, "test", "testing data");
    };

    thread server_thread([&server]() { server.start(); });

    cout << "listening on ws://localhost:" << server.config.port << endl;

    server_thread.join();
    server.stop();

    cout << "server stopped.." << endl;

    return 0;
}


void emit(WsServer::Endpoint &event_emitter, string event, string data) {
    for(auto &listener : event_emitter.get_connections()) {
        if (listener->path_match[1] == event) {
            listener->send(data);
        }
    }
}
