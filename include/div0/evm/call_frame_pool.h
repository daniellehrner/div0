#ifndef DIV0_EVM_CALL_FRAME_POOL_H
#define DIV0_EVM_CALL_FRAME_POOL_H

#include <array>
#include <cassert>

#include "call_frame.h"

namespace div0::evm {

class CallFramePool {
 public:
  CallFramePool() = default;
  ~CallFramePool() = default;

  // non copyable, non moveable
  CallFramePool(const CallFramePool& other) = delete;
  CallFramePool(CallFramePool&& other) = delete;
  CallFramePool& operator=(const CallFramePool& other) = delete;
  CallFramePool& operator=(CallFramePool&& other) = delete;

  CallFrame* rent_frame() {
    assert(frame_depth_ < 1024);

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index) - frame_depth_ is
    // bounds-checked above
    CallFrame& frame = frame_pool_[frame_depth_++];
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    frame.reset();

    return &frame;
  }

  void return_frame() {
    assert(frame_depth_ > 0);

    --frame_depth_;
  }

 private:
  alignas(64) std::array<CallFrame, 1024> frame_pool_{};
  size_t frame_depth_{0};
};

}  // namespace div0::evm

#endif  // DIV0_EVM_CALL_FRAME_POOL_H
