#pragma once

#include "core/engine/engine.hpp"
#include "itc/itc.hpp"

namespace otto::engines::chorus {

  using namespace core::props;

  struct Screen;
  struct Audio;

  using Sender = core::engine::EngineSender<Audio, Screen>;

  struct Props {
    Sender sender;

    Sender::Prop<struct delay_tag, float> delay = {sender, 0.8, limits(0, 1), step_size(0.01)};
    Sender::Prop<struct rate_tag, float> rate = {sender, 0, limits(0, 2), step_size(0.02)};
    Sender::Prop<struct feedback_tag, float> feedback = {sender, 0, limits(-0.99, 0.99), step_size(0.02)};
    Sender::Prop<struct depth_tag, float> depth = {sender, 0.5, limits(0, 1), step_size(0.01)};
    DECL_REFLECTION(Props, delay, depth, feedback, rate);
  };

  struct Chorus : core::engine::EffectEngine<Chorus> {
    static constexpr util::string_ref name = "Chorus";
    Chorus();

    void encoder(core::input::EncoderEvent e) override;

    core::ui::ScreenAndInput screen() override;

  private:
    itc::Shared<float>::Storage shared_phase_;

  public:
    const std::unique_ptr<Audio> audio;

  private:
    const std::unique_ptr<Screen> screen_;

    Sender sender_ = {*audio, *screen_};
  public:
    Props props{sender_};
  };

} // namespace otto::engines::chorus

#include "audio.hpp"
#include "screen.hpp"
