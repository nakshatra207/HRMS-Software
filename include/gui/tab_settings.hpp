#pragma once

#include "gui/gui_common.hpp"
#include "security/audit.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

namespace hrms::gui {

class SettingsTab : public Fl_Group {
public:
    SettingsTab(int x, int y, int w, int h);

    void refreshData();

private:
    Fl_Box* m_sysInfoBox{nullptr};
    Fl_Button* m_btnToggleAsm{nullptr};
    Fl_Box* m_asmStatusLabel{nullptr};

    Fl_Button* m_btnRunBenchmark{nullptr};
    DataTable* m_benchTable{nullptr};

    Fl_Button* m_btnRefreshAudit{nullptr};
    DataTable* m_auditTable{nullptr};

    void runBenchmark();
    void toggleAssembly();

    static void onToggleAsmClicked(Fl_Widget* w, void* data);
    static void onRunBenchmarkClicked(Fl_Widget* w, void* data);
    static void onRefreshAuditClicked(Fl_Widget* w, void* data);
};

} // namespace hrms::gui
