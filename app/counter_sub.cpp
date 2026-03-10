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
        << "  -n, --count <N>           Exit after N messages (0 = unlimited) "
           "[default: 0]\n"
        << "  -h, --help               Show this help\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string key_expr = "demo/counter";
    std::vector<std::string> connect_endpoints;
    std::vector<std::string> listen_endpoints;
    int count = 0;

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
        } else if ((arg == "-n" || arg == "--count") && i + 1 < argc) {
            count = std::atoi(argv[++i]);
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
        std::cout << "Subscribing on '" << key_expr << "'\n";

        // TODO(slice-4): Open session, declare subscriber, dispatch messages
        (void)connect_endpoints;
        (void)listen_endpoints;
        (void)count;

        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "\nShutting down...\n";
    } catch (const zenoh::ZException& e) {
        std::cerr << "Zenoh error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
