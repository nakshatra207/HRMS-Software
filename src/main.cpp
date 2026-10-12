#include "core/config.hpp"
#include "core/ui.hpp"
#include "database/database.hpp"
#include "utilities/logger.hpp"
#include "utilities/fast_ops.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <filesystem>

extern "C" {
#include "posix_sys.h"
}

static void printUsage(const char* progName) {
    std::cout << "Lightweight HRMS - Native High-Performance C++20 Human Resource Management System\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help                          Show this help message and exit\n";
    std::cout << "  -v, --version                       Display version, build, and compiler details\n";
    std::cout << "  -b, --benchmark                     Run high-precision benchmark suite and exit\n";
    std::cout << "  -c, --config <file>                 Specify custom configuration file path\n";
    std::cout << "  --export-report <type> <output.csv> Export report directly to CSV without TUI\n";
    std::cout << "                                      Types: employees, attendance, leave, payroll,\n";
    std::cout << "                                             departments, recruitment\n\n";
    std::cout << "Default Behavior:\n";
    std::cout << "  Launches the native terminal user interface (TUI).\n";
}

static void printVersion() {
    std::cout << "Lightweight HRMS v1.0.0 (Linux x86-64 native)\n";
    std::cout << "Architecture: Modern C++20, C11, SQLite3 Embedded, NASM x86-64 Assembly\n";
    std::cout << "Assembly Optimization: " << (hrms::utils::is_assembly_compiled() ? "Enabled (NASM)" : "Disabled (C++ Fallback)") << "\n";
#if defined(__GNUC__)
    std::cout << "Compiler: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
#elif defined(__clang__)
    std::cout << "Compiler: Clang " << __clang_version__ << "\n";
#endif
    std::cout << "Standard: C++20 / C11 / POSIX.1-2008\n";
}

int main(int argc, char* argv[]) {
    // Ignore SIGPIPE to avoid abrupt termination on closed sockets or pipes
    signal(SIGPIPE, SIG_IGN);

    std::string configPath = "config/hrms.conf";
    bool runBench = false;
    std::string exportType;
    std::string exportPath;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "-b" || arg == "--benchmark") {
            runBench = true;
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "--export-report" && i + 2 < argc) {
            exportType = argv[++i];
            exportPath = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    // 1. Load configuration
    auto& cfg = hrms::core::Config::instance();
    if (!cfg.loadFromFile(configPath)) {
        // Try fallback if running from build/ directory
        if (cfg.loadFromFile("../" + configPath)) {
            // Found in parent directory
        } else {
            std::cerr << "Warning: Could not open config file '" << configPath << "'. Using built-in defaults.\n";
        }
    }

    // 2. Initialize logger
    std::string logPath = cfg.getString("log_path", "logs/hrms.log");
    std::string logLevelStr = cfg.getString("log_level", "INFO");
    auto minLevel = hrms::utils::Logger::stringToLevel(logLevelStr);
    hrms::utils::Logger::instance().init(logPath, minLevel, false);
    LOG_INFO("Starting Lightweight HRMS Application...");

    // 3. Configure Assembly Optimization
    bool enableAsm = cfg.getBool("enable_assembly", true);
    hrms::utils::set_assembly_enabled(enableAsm);
    LOG_INFO("Assembly optimizations: " + std::string(hrms::utils::is_assembly_active() ? "Active" : "Disabled"));

    // 4. Initialize Database
    std::string dbPath = cfg.getString("database_path", "data/hrms.db");
    try {
        auto& db = hrms::db::Database::instance();
        db.open(dbPath);
        LOG_INFO("Database connected successfully at " + dbPath);

        // Auto-bootstrap schema if database is brand new
        if (!db.tableExists("employees")) {
            LOG_INFO("Initializing database schema from database/schema.sql...");
            std::string schemaPath = "database/schema.sql";
            if (!std::filesystem::exists(schemaPath)) schemaPath = "../database/schema.sql";
            db.executeFile(schemaPath);

            std::string seedPath = "database/seed.sql";
            if (!std::filesystem::exists(seedPath)) seedPath = "../database/seed.sql";
            if (std::filesystem::exists(seedPath)) {
                LOG_INFO("Seeding initial database data from database/seed.sql...");
                db.executeFile(seedPath);
            }
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Fatal database initialization error: " + std::string(ex.what()));
        std::cerr << "Database Error: " << ex.what() << "\n";
        return 1;
    }

    // 5. Handle CLI modes
    if (runBench) {
        hrms::ui::TerminalUI::runBenchmark();
        return 0;
    }

    if (!exportType.empty()) {
        hrms::ui::TerminalUI::exportReportCli(exportType, exportPath);
        return 0;
    }

    // 6. Launch Terminal UI
    try {
        hrms::ui::TerminalUI ui;
        ui.run();
    } catch (const std::exception& ex) {
        LOG_ERROR("Unhandled exception in UI: " + std::string(ex.what()));
        std::cerr << "An unexpected error occurred: " << ex.what() << "\n";
        return 1;
    }

    LOG_INFO("HRMS application terminated normally.");
    return 0;
}
