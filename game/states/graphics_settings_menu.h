#pragma once

#include "game/states/settings_panel_page.h"
#include "game/states/option_row.h"

// A separate restart-only graphics preference transaction, never part of the
// display-mode confirmation or the Controls page's immediately active settings.
class GraphicsSettingsMenu final : public SettingsPanelPage
{
public:
    explicit GraphicsSettingsMenu(MenuContext* context, int opening_slot = -1);
    void init() override;
    void update_page(float dt) override;
private:
    void refresh_value();
    void set_status(const std::wstring& text, bool warning = false);
    void apply_settings();
    bool selection_ = false;
    std::unique_ptr<OptionRow> crt_;
};
