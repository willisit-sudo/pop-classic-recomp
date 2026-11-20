// Prince of Persia Classic host application.
#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <rex/app/title_app.h>
#include <rex/hid/binding.h>
#include <rex/hid/input_system.h>
#include <rex/logging/log_category.h>
#include <rex/path_settings.h>
#include <rex/settings_registry.h>
#include <rex/ui/ui_system.h>

#include <imgui.h>

#include "generated/default/pop_classic_init.h"

namespace {

constexpr auto kActions = std::to_array<std::pair<const char*, const char*>>({
    {"Move up", "hid.keybind_lstick_up"},
    {"Move down", "hid.keybind_lstick_down"},
    {"Move left", "hid.keybind_lstick_left"},
    {"Move right", "hid.keybind_lstick_right"},
    {"Jump / confirm (A)", "hid.keybind_a"},
    {"Back (B)", "hid.keybind_b"},
    {"X button", "hid.keybind_x"},
    {"Y button", "hid.keybind_y"},
    {"Pause / Start", "hid.keybind_start"},
});

struct WindowSize {
  int width;
  int height;
  const char* label;
};

constexpr WindowSize kWindowSizes[] = {
    {800, 450, "800 x 450"}, {960, 540, "960 x 540"},
    {1280, 720, "1280 x 720"}, {1600, 900, "1600 x 900"},
    {1920, 1080, "1920 x 1080"},
};

bool IsDeckGamingMode() {
#ifdef __linux__
  const char* deck = std::getenv("SteamDeck");
  const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
  const char* session = std::getenv("XDG_SESSION_DESKTOP");
  return deck && std::string_view(deck) == "1" &&
         ((desktop && std::string_view(desktop) == "gamescope") ||
          (session && std::string_view(session) == "gamescope"));
#else
  return false;
#endif
}

class PlayerSettings final : public rex::ui::DialogBase {
 public:
  PlayerSettings(rex::ui::IUISystem& ui, rex::hid::IInputSystem& input,
                 bool& open)
      : ui_(ui), input_(input), open_(open) {}

  bool WantsExclusiveInput() const override { return true; }
  void OnClose() override { open_ = false; }

  bool Draw(ImGuiIO&) override {
    if (first_draw_) {
      REXLOG_DEBUG("Player settings panel opened");
      first_draw_ = false;
    }

    ImGui::SetNextWindowSize(ImVec2(480, 400), ImGuiCond_FirstUseEver);
    bool visible = capture_binding_ || !ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    if (ImGui::Begin("Prince of Persia Classic | Settings", &visible)) {
      if (ImGui::BeginTabBar("Settings tabs")) {
        if (ImGui::BeginTabItem("Controls")) {
          DrawControls();
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Display")) {
          DrawDisplay();
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }

      ImGui::Separator();
      if (ImGui::Button("Save settings")) {
        const auto result = REX_SETTINGS().Save(REX_PATHS().config_file.Get());
        status_ = result ? "Settings saved." : result.error().message;
      }
      ImGui::SameLine();
      if (ImGui::Button("Close")) {
        visible = false;
      }
      if (!status_.empty()) {
        ImGui::TextWrapped("%s", status_.c_str());
      }
    }
    ImGui::End();
    return visible;
  }

 private:
  void UpdateSetting(std::string_view name, std::string_view value) {
    const auto result = REX_SETTINGS().SetText(name, value);
    status_ = result ? "" : result.error().message;
  }

  void DrawControls() {
    bool keyboard = input_.GetSettings().mnk_mode.Get();
    if (ImGui::Checkbox("Enable keyboard controls", &keyboard)) {
      UpdateSetting("hid.mnk_mode", keyboard ? "true" : "false");
    }
    auto& bindings = input_.GetKeyBindings();
    CaptureBinding(bindings);

    ImGui::TextWrapped("Select a binding and press a new key. Esc cancels.");
    if (ImGui::BeginTable("Keyboard bindings", 2, ImGuiTableFlags_RowBg)) {
      for (const auto& [action, setting] : kActions) {
        const auto binding = bindings.FindBySetting(setting);
        if (!binding) {
          continue;
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(action);
        ImGui::TableNextColumn();
        ImGui::PushID(setting);
        std::string label;
        for (const auto& source : bindings.GetSources(*binding)) {
          if (!label.empty()) {
            label += " / ";
          }
          label += input_.InputSourceToString(source);
        }
        if (label.empty()) {
          label = "Unbound";
        }
        if (capture_binding_ == *binding) {
          label = "Press a key...";
        }
        label += "###binding";
        if (ImGui::Button(label.c_str(), ImVec2(-1, 0))) {
          capture_binding_ = *binding;
          status_.clear();
        }
        ImGui::PopID();
      }
      ImGui::EndTable();
    }

    if (ImGui::Button("Reset keyboard bindings")) {
      capture_binding_.reset();
      bool reset = true;
      for (const auto& [action, setting] : kActions) {
        if (const auto binding = bindings.FindBySetting(setting)) {
          reset = bindings.Reset(*binding) && reset;
        }
      }
      status_ = reset ? "Default keys restored." : "Some keys could not be reset.";
    }
    ImGui::TextWrapped("Steam Deck: remap the physical buttons in Steam > Controller Layout.");
  }


  void CaptureBinding(rex::hid::IBindingSet& bindings) {
    if (!capture_binding_) {
      return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
      capture_binding_.reset();
      return;
    }

    for (int code = ImGuiKey_Tab; code <= ImGuiKey_KeypadEqual; ++code) {
      const auto key = static_cast<ImGuiKey>(code);
      if ((key >= ImGuiKey_LeftCtrl && key <= ImGuiKey_Menu) ||
          (key >= ImGuiKey_F1 && key <= ImGuiKey_F24) ||
          !ImGui::IsKeyPressed(key, false)) {
        continue;
      }

      std::string chord;
      const auto& io = ImGui::GetIO();
      if (io.KeyCtrl) chord += "Ctrl+";
      if (io.KeyShift) chord += "Shift+";
      if (io.KeyAlt) chord += "Alt+";
      switch (key) {
        case ImGuiKey_LeftArrow: chord += "Left"; break;
        case ImGuiKey_RightArrow: chord += "Right"; break;
        case ImGuiKey_UpArrow: chord += "Up"; break;
        case ImGuiKey_DownArrow: chord += "Down"; break;
        case ImGuiKey_Enter: chord += "Return"; break;
        case ImGuiKey_Apostrophe: chord += "Quote"; break;
        default: chord += ImGui::GetKeyName(key); break;
      }

      const auto source = input_.ParseInputSource(chord);
      if (!source) {
        status_ = "That key cannot be assigned.";
        return;
      }
      if (bindings.FindConflict(*capture_binding_, *source)) {
        status_ = "That key is already assigned to another control.";
        return;
      }
      if (bindings.SetSources(*capture_binding_, std::span(&*source, 1))) {
        UpdateSetting("hid.mnk_mode", "true");
      } else {
        status_ = "Could not change that binding.";
      }
      capture_binding_.reset();
      return;
    }
  }

  void DrawDisplay() {
    const auto& settings = ui_.GetSettings();
    const bool fullscreen = settings.fullscreen.Get();
    int mode = fullscreen ? 1 : 0;
    if (ImGui::Combo("Display mode", &mode, "Windowed\0Fullscreen\0")) {
      UpdateSetting("ui.fullscreen", mode == 1 ? "true" : "false");
    }

    const auto width = settings.window_width.Get();
    const auto height = settings.window_height.Get();
    const std::string current = width && height
        ? std::to_string(width) + " x " + std::to_string(height) : "Automatic";
    ImGui::BeginDisabled(mode == 1);
    if (ImGui::BeginCombo("Window size", current.c_str())) {
      for (const auto& size : kWindowSizes) {
        if (ImGui::Selectable(size.label, width == size.width && height == size.height)) {
          UpdateSetting("ui.window_width", std::to_string(size.width));
          UpdateSetting("ui.window_height", std::to_string(size.height));
        }
      }
      ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    ImGui::TextWrapped("Fullscreen scales the game to the display.");
  }

  rex::ui::IUISystem& ui_;
  rex::hid::IInputSystem& input_;
  bool& open_;
  bool first_draw_ = true;
  std::optional<size_t> capture_binding_;
  std::string status_;
};

class PopClassicApp final : public rex::app::TitleApp, private rex::ui::IUIEventListener {
 public:
  PopClassicApp() : TitleApp("pop_classic", kPPCImageConfig) {}

 private:
  rex::Result<void> OnConfigure(rex::SettingsRegistry& settings) override {
    if (auto result = TitleApp::OnConfigure(settings); !result) {
      return result;
    }
    return settings.SetText(rex::SettingLayer::kTitle, "ui.fullscreen",
                            IsDeckGamingMode() ? "true" : "false");
  }

  rex::Result<void> OnInit() override {
    if (auto result = TitleApp::OnInit(); !result) {
      return result;
    }
    if (GetUI().HasWindow()) {
      input_listener_ = GetUI().AddEventListener(*this);
      if (open_at_startup_) {
        OpenSettings();
      }
    }
    return rex::Ok();
  }

  void OnShutdown() override {
    input_listener_ = {};
    TitleApp::OnShutdown();
  }

  void OnUIEvent(const rex::ui::UIEvent&) override {}

  void OnInputEvent(rex::InputEvent& event) override {
    if (event.type != rex::InputEvent::Type::kKeyDown ||
        event.key != rex::VirtualKey::kF2 || event.repeat) {
      return;
    }
    event.handled = true;
    OpenSettings();
  }

  void OpenSettings() {
    if (settings_open_) {
      return;
    }
    settings_open_ = true;
    GetUI().AddDialog(std::make_unique<PlayerSettings>(GetUI(), GetInput(), settings_open_));
  }

  std::span<const rex::CLIAction> GetActions() const override {
    static constexpr rex::CLIAction actions[] = {{"settings", "Open player settings at launch"}};
    return actions;
  }

  rex::Result<void> OnAction(const rex::CLIActionHit& hit) override {
    if (hit.action.name == "settings") {
      open_at_startup_ = true;
      return rex::Ok();
    }
    return TitleApp::OnAction(hit);
  }

  bool settings_open_ = false;
  bool open_at_startup_ = false;
  rex::ScopeGuard input_listener_;
};

}  // namespace

REX_TITLE_APP(PopClassicApp)
