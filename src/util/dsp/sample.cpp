#include "sample.hpp"

#include "util/assert.hpp"

namespace otto::dsp {

  Sample::Sample(gsl::span<float> audio_data, float speed_modifier) noexcept
    : audio_data_(audio_data), end_point_(audio_data_.size()), speed_modifier(speed_modifier)
  {}

  int Sample::size() const noexcept
  {
    return end_point_ - start_point_;
  }

  Sample::iterator Sample::begin() const noexcept
  {
    float stride = playback_speed_ * speed_modifier;
    if (stride < 0) {
      return {*this, end_point() - 1};
    }
    return {*this, start_point()};
  }

  Sample::iterator Sample::end() const noexcept
  {
    float stride = playback_speed_ * speed_modifier;
    if (stride < 0) {
      return {*this, start_point() - 1};
    }
    return {*this, end_point()};
  }

  int Sample::start_point() const noexcept
  {
    return start_point_;
  }

  int Sample::end_point() const noexcept
  {
    return end_point_;
  }

  // int Sample::loop_start() const noexcept
  // {
  //   if (loop_start_ < 0) return start_point();
  //   return loop_start_;
  // }
  //
  // int Sample::loop_end() const noexcept
  // {
  //   if (loop_end_ < 0) return end_point();
  //   return loop_end_;
  // }

  int Sample::fade_in_time() const noexcept
  {
    return fade_in_time_;
  }

  int Sample::fade_out_time() const noexcept
  {
    return fade_out_time_;
  }

  float Sample::playback_speed() const noexcept
  {
    return playback_speed_;
  }

  int Sample::start_point(int val) noexcept
  {
    start_point_ = val;
    if (end_point_ < val) end_point(val);
    return start_point_;
  }

  int Sample::end_point(int val) noexcept
  {
    val = std::clamp(val, 0, (int) audio_data_.size());
    end_point_ = val;
    if (start_point() > val) start_point(val);
    return end_point_;
  }

  // int Sample::loop_start(int val) noexcept
  // {
  //   val = std::clamp(val, start_point(), loop_end() - 1);
  //   loop_start_ = val;
  //   return loop_start_;
  // }
  //
  // int Sample::loop_end(int val) noexcept
  // {
  //   val = std::clamp(val, loop_start() + 1, end_point());
  //   loop_end_ = val;
  //   return loop_end_;
  // }

  int Sample::fade_in_time(int val) noexcept
  {
    fade_in_time_ = val;
    return fade_in_time_;
  }

  int Sample::fade_out_time(int val) noexcept
  {
    fade_out_time_ = val;
    return fade_out_time_;
  }

  float Sample::playback_speed(float val) noexcept
  {
    playback_speed_ = val;
    return playback_speed_;
  }

  Sample::iterator::iterator(const Sample& sample, int index) noexcept : sample_(&sample), index_(index) {}

  Sample::iterator::iterator(const iterator& rhs) noexcept = default;

  void Sample::iterator::advance(std::ptrdiff_t d) noexcept
  {
    float int_part;
    error_ = std::modf(error_ + playback_speed() * d, &int_part);
    if (error_ < 0) {
      int_part -= 1;
      error_ += 1;
    }
    for (int i = 0; i != std::abs(int_part); i++) {
      index_ += int_part / std::abs(int_part);
      // if (signed_index() == loop_end() && do_loop) {
      // index_ = std::abs(loop_start());
      // }
    }
    auto abs_start = sign() * start_point();
    auto abs_end = sign() * end_point();
    index_ = std::clamp(index_, std::min(abs_start, abs_end), std::max(abs_start, abs_end));
  }

  float Sample::iterator::dereference() const noexcept
  {
    if (sample_ == nullptr || start_point() == end_point() || signed_index() < start_point() ||
        signed_index() >= end_point())
      return 0.f;
    OTTO_ASSERT(index_ >= 0 && index_ < sample_->audio_data_.size());
    float res = sample_->audio_data_[index_];
    if (fade_in_time() > 0) {
      float fade_amnt = (signed_index() - start_point()) / float(fade_in_time());
      if (fade_amnt < 1) res *= fade_amnt;
    }
    if (fade_out_time() > 0) {
      float fade_amnt = (end_point() - signed_index()) / float(fade_out_time());
      if (fade_amnt < 1) res *= fade_amnt;
    }
    return res;
  }

  bool Sample::iterator::equal(const Sample::iterator& rhs) const noexcept
  {
    return index_ == rhs.index_ && (signed_index() == end_point() || error_ == rhs.error_);
  }

  std::ptrdiff_t Sample::iterator::operator-(const Sample::iterator& rhs) const noexcept
  {
    return signed_index() - rhs.signed_index();
  }

  int Sample::iterator::index() const noexcept
  {
    return index_;
  }

  float Sample::iterator::playback_speed() const noexcept
  {
    if (sample_ == nullptr) return 0;
    return sample_->playback_speed_ * sample_->speed_modifier;
  }

  int Sample::iterator::start_point() const noexcept
  {
    if (sample_ == nullptr) return 0;
    if (playback_speed() < 0) return -(sample_->end_point() - 1);
    return sample_->start_point();
  }

  int Sample::iterator::end_point() const noexcept
  {
    if (sample_ == nullptr) return 0;
    if (playback_speed() < 0) return -(sample_->start_point() - 1);
    return sample_->end_point();
  }

  // int Sample::iterator::loop_start() const noexcept
  // {
  //   if (sample_ == nullptr) return 0;
  //   if (playback_speed() < 0) return -sample_->loop_end();
  //   return sample_->loop_start();
  // }
  // int Sample::iterator::loop_end() const noexcept
  // {
  //   if (sample_ == nullptr) return 0;
  //   if (playback_speed() < 0) return -sample_->loop_start();
  //   return sample_->loop_end();
  // }

  int Sample::iterator::signed_index() const noexcept
  {
    if (sample_ == nullptr) return 0;
    if (playback_speed() < 0) return -index_;
    return index_;
  }

  int Sample::iterator::fade_in_time() const noexcept
  {
    if (sample_ == nullptr) return 0;
    if (playback_speed() < 0) return sample_->fade_out_time();
    return sample_->fade_in_time();
  }

  int Sample::iterator::fade_out_time() const noexcept
  {
    if (sample_ == nullptr) return 0;
    if (playback_speed() < 0) return sample_->fade_in_time();
    return sample_->fade_out_time();
  }

  int Sample::iterator::sign() const noexcept
  {
    if (playback_speed() < 0) return -1;
    return 1;
  }

} // namespace otto::dsp
