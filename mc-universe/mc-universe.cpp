#include "common.hpp"
#include "WsServer.hpp"

// ===== MAIN =====

int main (int argc, char* argv[]) {
    WsServer server;
    server.config.port = 1177;
    server.config.thread_pool_size = 4;

    add_endpoints(server);

    thread server_thread([&server]() { server.start(); });

    cout << "listening on ws://localhost:" << server.config.port << endl;

    server_thread.join();
    server.stop();

    UNUSED(argc);
    UNUSED(argv);
    return 0;
}
