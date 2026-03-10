#include <csignal>

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "zenoh.hxx"

namespace {

std::atomic<bool> g_running{true};

void SignalHandler(int /*signal*/) {
    g_running.store(false);
}

void PrintUsage(const char* program) {
    std::cout << "Usage: " << program << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  -l, --listen <ENDPOINT>  Listen on endpoint "
                 "[default: tcp/0.0.0.0:7447]\n"
              << "  -h, --help               Show this help\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string listen_endpoint = "tcp/0.0.0.0:7447";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if ((arg == "-l" || arg == "--listen") && i + 1 < argc) {
            listen_endpoint = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    try {
#ifdef ZENOHCXX_ZENOHC
        zenoh::init_log_from_env_or("error");
#endif
        zenoh::Config config = zenoh::Config::create_default();
        config.insert_json5("mode", "\"router\"");
        config.insert_json5("listen/endpoints",
                            "[\"" + listen_endpoint + "\"]");

        auto session = zenoh::Session::open(std::move(config));
        std::cout << "Router listening on " << listen_endpoint << "\n";
        std::cout << "Press CTRL-C to quit...\n";

        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "\nShutting down router...\n";
    } catch (const zenoh::ZException& e) {
        std::cerr << "Zenoh error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
