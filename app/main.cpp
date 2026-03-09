#include "counter/publisher.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "zenoh.hxx"

namespace {

std::atomic<bool> g_running{true};

void SignalHandler(int /*signal*/) {
    g_running.store(false);
}

void PrintUsage(const char* program) {
    std::cout
        << "Usage: " << program << " [OPTIONS]\n"
        << "\n"
        << "Options:\n"
        << "  -k, --key <KEYEXPR>      Key expression [default: demo/counter]\n"
        << "  -e, --connect <ENDPOINT>  Connect to endpoint (repeatable)\n"
        << "  -l, --listen <ENDPOINT>   Listen on endpoint (repeatable)\n"
        << "  -i, --interval <MS>       Publish interval in ms [default: "
           "1000]\n"
        << "  -h, --help               Show this help\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string key_expr = "demo/counter";
    std::vector<std::string> connect_endpoints;
    std::vector<std::string> listen_endpoints;
    int interval_ms = 1000;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if ((arg == "-k" || arg == "--key") && i + 1 < argc) {
            key_expr = argv[++i];
        } else if ((arg == "-e" || arg == "--connect") && i + 1 < argc) {
            connect_endpoints.push_back(argv[++i]);
        } else if ((arg == "-l" || arg == "--listen") && i + 1 < argc) {
            listen_endpoints.push_back(argv[++i]);
        } else if ((arg == "-i" || arg == "--interval") && i + 1 < argc) {
            interval_ms = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }

    // Install signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    try {
#ifdef ZENOHCXX_ZENOHC
        zenoh::init_log_from_env_or("error");
#endif
        std::cout << "Opening session...\n";
        counter::ShmCounterPublisher pub(key_expr, connect_endpoints,
                                         listen_endpoints);
        std::cout << "Publishing on '" << key_expr << "' every " << interval_ms
                  << "ms\n";
        std::cout << "Press CTRL-C to quit...\n";

        while (g_running.load()) {
            pub.Publish();
            std::cout << "[" << pub.Counter() << "] Published on '" << key_expr
                      << "'\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }

        std::cout << "\nShutting down... (published " << pub.Counter()
                  << " messages)\n";
    } catch (const zenoh::ZException& e) {
        std::cerr << "Zenoh error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
