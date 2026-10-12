#include "gui/gui_theme.hpp"
#include "gui/gui_app.hpp"
#include "gui/dialog_login.hpp"
#include "core/config.hpp"
#include "database/database.hpp"
#include "utilities/logger.hpp"
#include "utilities/fast_ops.hpp"

#include "security/auth.hpp"
#include <iostream>
#include <csignal>
#include <filesystem>

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);

    std::string configPath = "config/hrms.conf";
    std::string autoLoginRole;
    int exitAfterSeconds = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "--auto-login" && i + 1 < argc) {
            autoLoginRole = argv[++i];
        } else if (arg == "--exit-after" && i + 1 < argc) {
            exitAfterSeconds = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: hrms_gui [options]\n";
            std::cout << "  -c, --config <file>     Specify configuration file (default: config/hrms.conf)\n";
            std::cout << "  --auto-login <role>     Auto-login as 'admin', 'hr', or 'emp'\n";
            std::cout << "  --exit-after <seconds>  Automatically close after N seconds (diagnostics/test)\n";
            std::cout << "  -h, --help              Show help message\n";
            return 0;
        }
    }

    // 1. Load configuration
    auto& cfg = hrms::core::Config::instance();
    if (!cfg.loadFromFile(configPath)) {
        if (!cfg.loadFromFile("../" + configPath)) {
            std::cerr << "Notice: Could not load '" << configPath << "'. Using default settings.\n";
        }
    }

    // 2. Initialize logger
    std::string logPath = cfg.getString("gui_log_path", "logs/hrms_gui.log");
    std::filesystem::create_directories("logs");
    std::filesystem::create_directories("reports");
    std::filesystem::create_directories("data");

    hrms::utils::Logger::instance().init(logPath, hrms::utils::LogLevel::INFO, false);
    LOG_INFO("Starting Lightweight HRMS Native GUI Application...");

    // 3. Configure Assembly Optimization
    bool enableAsm = cfg.getBool("enable_assembly", true);
    hrms::utils::set_assembly_enabled(enableAsm);
    LOG_INFO("Assembly optimizations: " + std::string(hrms::utils::is_assembly_active() ? "Active" : "Disabled"));

    // 4. Connect SQLite Database
    std::string dbPath = cfg.getString("database_path", "data/hrms.db");
    try {
        auto& db = hrms::db::Database::instance();
        db.open(dbPath);
        LOG_INFO("Database connected successfully at " + dbPath);

        // Auto-bootstrap schema if database is brand new
        if (!db.tableExists("employees")) {
            LOG_INFO("Bootstrapping database schema from database/schema.sql...");
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
        std::cerr << "Database Initialization Error: " << ex.what() << "\n";
        return 1;
    }

    // 5. Initialize Theme
    hrms::gui::Theme::init();

    // 6. Present Authentication Modal (or Auto-Login if requested)
    if (!autoLoginRole.empty()) {
        auto& auth = hrms::sec::AuthService::instance();
        if (autoLoginRole == "admin") {
            auth.login("admin", "admin123");
        } else if (autoLoginRole == "hr" || autoLoginRole == "hr_mgr") {
            auth.login("hr_mgr", "hr12345");
        } else if (autoLoginRole == "emp" || autoLoginRole == "employee" || autoLoginRole == "emp_user") {
            auth.login("emp_user", "emp12345");
        } else {
            std::cerr << "Unknown auto-login role: " << autoLoginRole << "\n";
            return 1;
        }
    } else {
        hrms::gui::LoginDialog login;
        if (!login.execute()) {
            LOG_INFO("Authentication canceled or closed by user. Exiting.");
            return 0;
        }
    }

    // 7. Launch Main GUI Application Window
    try {
        hrms::gui::MainWindow app(1200, 780, "Lightweight HRMS - Human Resource Management Suite");
        app.show();

        if (exitAfterSeconds > 0) {
            Fl::add_timeout(exitAfterSeconds, [](void* data) {
                Fl_Window* w = static_cast<Fl_Window*>(data);
                w->hide();
            }, &app);
        }

        int ret = Fl::run();
        LOG_INFO("HRMS GUI terminated normally.");
        return ret;
    } catch (const std::exception& ex) {
        LOG_ERROR("Unhandled GUI exception: " + std::string(ex.what()));
        std::cerr << "An unexpected GUI error occurred: " << ex.what() << "\n";
        return 1;
    }
}
