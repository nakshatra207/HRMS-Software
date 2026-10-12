#include "gui/dialog_login.hpp"
#include "gui/gui_theme.hpp"
#include "security/auth.hpp"
#include "security/crypto.hpp"
#include "utilities/logger.hpp"

namespace hrms::gui {

LoginDialog::LoginDialog()
    : Fl_Window(480, 420, "Lightweight HRMS - Enterprise Authentication") {
    set_modal();
    color(Theme::COLOR_BG);

    // Title Card
    Fl_Box* headerCard = new Fl_Box(0, 0, 480, 80);
    headerCard->box(FL_FLAT_BOX);
    headerCard->color(Theme::COLOR_HEADER_BG);

    Fl_Box* title = new Fl_Box(0, 15, 480, 28, "LIGHTWEIGHT HRMS");
    title->labelcolor(Theme::COLOR_PRIMARY_HOVER);
    title->labelfont(FL_HELVETICA_BOLD);
    title->labelsize(20);

    Fl_Box* subtitle = new Fl_Box(0, 45, 480, 18, "Native Linux C++20 | SQLite3 WAL | NASM Assembly");
    subtitle->labelcolor(Theme::COLOR_TEXT_SECONDARY);
    subtitle->labelfont(FL_HELVETICA);
    subtitle->labelsize(11);

    // Inputs Area
    int startY = 105;

    Fl_Box* userLbl = new Fl_Box(40, startY, 400, 18, "Username");
    userLbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    userLbl->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    userLbl->labelfont(FL_HELVETICA_BOLD);
    userLbl->labelsize(12);

    m_userInput = new Fl_Input(40, startY + 20, 400, 34);
    Theme::styleInput(m_userInput);

    Fl_Box* passLbl = new Fl_Box(40, startY + 65, 400, 18, "Password");
    passLbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    passLbl->labelcolor(Theme::COLOR_TEXT_PRIMARY);
    passLbl->labelfont(FL_HELVETICA_BOLD);
    passLbl->labelsize(12);

    m_passInput = new Fl_Secret_Input(40, startY + 85, 400, 34);
    Theme::styleInput(m_passInput);

    // Error status label
    m_statusLabel = new Fl_Box(40, startY + 125, 400, 20, "");
    m_statusLabel->labelcolor(Theme::COLOR_DANGER);
    m_statusLabel->labelfont(FL_HELVETICA_BOLD);
    m_statusLabel->labelsize(11);

    // Action Buttons
    m_loginBtn = new Fl_Button(40, startY + 155, 260, 38, "Sign In");
    Theme::stylePrimaryButton(m_loginBtn);
    m_loginBtn->callback(onLoginClicked, this);

    m_cancelBtn = new Fl_Button(315, startY + 155, 125, 38, "Exit");
    Theme::styleSecondaryButton(m_cancelBtn);
    m_cancelBtn->callback(onCancelClicked, this);

    // Separator line
    Fl_Box* sep = new Fl_Box(40, startY + 205, 400, 1);
    sep->box(FL_FLAT_BOX);
    sep->color(Theme::COLOR_BORDER);

    // Quick Login Demo Bar
    Fl_Box* quickLbl = new Fl_Box(40, startY + 215, 400, 18, "Quick Login (Testing Profiles):");
    quickLbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    quickLbl->labelcolor(Theme::COLOR_TEXT_MUTED);
    quickLbl->labelfont(FL_HELVETICA_BOLD);
    quickLbl->labelsize(11);

    m_quickAdminBtn = new Fl_Button(40, startY + 238, 125, 32, "Admin");
    Theme::styleSecondaryButton(m_quickAdminBtn);
    m_quickAdminBtn->callback(onQuickAdminClicked, this);

    m_quickHrBtn = new Fl_Button(177, startY + 238, 125, 32, "HR Manager");
    Theme::styleSecondaryButton(m_quickHrBtn);
    m_quickHrBtn->callback(onQuickHrClicked, this);

    m_quickEmpBtn = new Fl_Button(315, startY + 238, 125, 32, "Employee");
    Theme::styleSecondaryButton(m_quickEmpBtn);
    m_quickEmpBtn->callback(onQuickEmpClicked, this);

    end();
}

bool LoginDialog::execute() {
    show();
    while (shown()) {
        Fl::wait();
    }
    return m_authenticated;
}

void LoginDialog::performLogin(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        m_statusLabel->copy_label("Please enter both username and password.");
        m_statusLabel->redraw();
        return;
    }

    auto& auth = sec::AuthService::instance();
    if (auth.login(username, password)) {
        sec::Crypto::secureClear(const_cast<std::string&>(password));
        m_authenticated = true;
        hide();
    } else {
        m_statusLabel->copy_label("Invalid username or password. Please try again.");
        m_statusLabel->redraw();
    }
}

void LoginDialog::onLoginClicked(Fl_Widget* /*w*/, void* data) {
    LoginDialog* self = static_cast<LoginDialog*>(data);
    std::string user = self->m_userInput->value() ? self->m_userInput->value() : "";
    std::string pass = self->m_passInput->value() ? self->m_passInput->value() : "";
    self->performLogin(user, pass);
}

void LoginDialog::onCancelClicked(Fl_Widget* /*w*/, void* data) {
    LoginDialog* self = static_cast<LoginDialog*>(data);
    self->m_authenticated = false;
    self->hide();
}

void LoginDialog::onQuickAdminClicked(Fl_Widget* /*w*/, void* data) {
    LoginDialog* self = static_cast<LoginDialog*>(data);
    self->m_userInput->value("admin");
    self->m_passInput->value("admin123");
    self->performLogin("admin", "admin123");
}

void LoginDialog::onQuickHrClicked(Fl_Widget* /*w*/, void* data) {
    LoginDialog* self = static_cast<LoginDialog*>(data);
    self->m_userInput->value("hr_mgr");
    self->m_passInput->value("hr12345");
    self->performLogin("hr_mgr", "hr12345");
}

void LoginDialog::onQuickEmpClicked(Fl_Widget* /*w*/, void* data) {
    LoginDialog* self = static_cast<LoginDialog*>(data);
    self->m_userInput->value("emp_user");
    self->m_passInput->value("emp12345");
    self->performLogin("emp_user", "emp12345");
}

} // namespace hrms::gui
