#pragma once

#include <chrono>
#include <unordered_map>

#include <json.hpp>
#include <type_safe/bounded_type.hpp>
#include <type_safe/strong_typedef.hpp>

#include "util/enum.hpp"
#include "util/locked.hpp"

#include "core/props/props.hpp"
#include "core/service.hpp"
#include "core/ui/screen.hpp"
#include "services/application.hpp"

#include "itc/action_queue.hpp"

namespace otto::services {

  BETTER_ENUM(SourceEnum, std::int8_t, sequencer, internal, external)

  BETTER_ENUM(ChannelEnum,
              std::int8_t,
              sampler0 = 0,
              sampler1 = 1,
              sampler2 = 2,
              sampler3 = 3,
              sampler4 = 4,
              sampler5 = 5,
              sampler6 = 6,
              sampler7 = 7,
              sampler8 = 8,
              sampler9 = 9,
              internal,
              external)

  SourceEnum source_of(ChannelEnum) noexcept;

  BETTER_ENUM(ScreenEnum,
              std::int8_t,
              sends,
              routing,
              fx1,
              fx1_selector,
              fx2,
              fx2_selector,
              looper,
              arp,
              arp_selector,
              voices,
              master,
              sequencer,
              sampler,
              sampler_envelope,
              synth,
              synth_selector,
              synth_envelope,
              settings,
              external,
              twist1,
              twist2)

  BETTER_ENUM(KeyMode, std::int8_t, midi, seq);

  struct UIManager : core::Service {
    /// The UI state
    ///
    /// This will dictate which state-leds light up, and which channel is currently selected etc.
    struct State {
      core::props::Property<SourceEnum> active_source = SourceEnum::internal;
      core::props::Property<ChannelEnum> active_channel = ChannelEnum::internal;
      core::props::Property<ScreenEnum> current_screen = ScreenEnum::synth;
      core::props::Property<KeyMode> key_mode = KeyMode::midi;
      core::props::Property<int> octave = {0, core::props::limits(-4, 4)};

      DECL_REFLECTION(State, active_channel, current_screen, key_mode, octave);
    };

    using ScreenSelector = std::function<core::ui::Screen&()>;

    UIManager();

    /// The main ui loop
    ///
    /// This sets up all the device specific graphics, and calls @ref draw_frame 60 times pr second,
    /// until @ref Application::running() is false, or the graphics are exitted by the user. It is
    /// also responsible for listening to keyevents, and calling @ref keypress and @ref keyrelease
    /// as apropriate.
    ///
    /// On some platforms (OSX), all OpenGL calls must be made from the main
    /// thread, therefore this function is called from `main()`.
    virtual void main_ui_loop() = 0;

    /// Select an engine
    void display(ScreenEnum screen);

    core::ui::Screen& current_screen();

    static UIManager& current() noexcept
    {
      return Application::current().ui_manager;
    }

    void register_screen_selector(ScreenEnum, ScreenSelector);

    State state;

    struct {
      util::Signal<core::ui::vg::Canvas&> on_draw;
    } signals;

    ch::Timeline& timeline() noexcept
    {
      return timeline_;
    }

    /// Push-only access to the action queue
    ///
    /// This queue is consumed at the start of each buffer.
    itc::PushOnlyActionQueue& action_queue() noexcept
    {
      return action_queue_;
    }

    /// Make an {@ref ActionQueueHelper} for the audio action queue
    template<typename... Recievers>
    auto make_aqh(Recievers&...) noexcept;

  protected:
    /// Draws the current screen and overlays.
    void draw_frame(core::ui::vg::Canvas& ctx);

    /// Display a screen.
    ///
    /// Calls @ref Screen::on_hide for the old screen, and then @ref Screen::on_show
    /// for the new screen
    void display(core::ui::Screen& screen);

  private:
    struct EmptyScreen : core::ui::Screen {
      void draw(core::ui::vg::Canvas& ctx) {}
    } empty_screen;

    core::ui::Screen* cur_screen = &empty_screen;

    util::enum_map<ScreenEnum, ScreenSelector> screen_selectors_;

    unsigned _frame_count = 0;

    chrono::time_point last_frame = chrono::clock::now();
    ch::Timeline timeline_;
    itc::ActionQueue action_queue_;
  };

  // IMPLEMENTATION //

  template<typename... Recievers>
  auto UIManager::make_aqh(Recievers&... recievers) noexcept
  {
    return itc::ActionQueueHelper(action_queue_, recievers...);
  }

} // namespace otto::services
