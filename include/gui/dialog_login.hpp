#pragma once

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <string>

namespace hrms::gui {

class LoginDialog : public Fl_Window {
public:
    LoginDialog();

    /**
     * Shows modal login dialog and blocks until user logs in or cancels.
     * Returns true on successful authentication.
     */
    bool execute();

private:
    Fl_Input* m_userInput{nullptr};
    Fl_Secret_Input* m_passInput{nullptr};
    Fl_Box* m_statusLabel{nullptr};
    Fl_Button* m_loginBtn{nullptr};
    Fl_Button* m_cancelBtn{nullptr};

    Fl_Button* m_quickAdminBtn{nullptr};
    Fl_Button* m_quickHrBtn{nullptr};
    Fl_Button* m_quickEmpBtn{nullptr};

    bool m_authenticated{false};

    void performLogin(const std::string& username, const std::string& password);

    static void onLoginClicked(Fl_Widget* w, void* data);
    static void onCancelClicked(Fl_Widget* w, void* data);
    static void onQuickAdminClicked(Fl_Widget* w, void* data);
    static void onQuickHrClicked(Fl_Widget* w, void* data);
    static void onQuickEmpClicked(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
