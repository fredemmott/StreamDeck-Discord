/*
 * Copyright (c) 2018-present, Frederick Emmott.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

#include <asio.hpp>

template<typename T>
struct AwaitablePromise {
  public:
    AwaitablePromise(
      asio::io_context& ctx
    ): p(
      new Impl {
        .timer = asio::steady_timer(ctx, std::chrono::steady_clock::time_point::max()),
      }
    ) {
    }

    void resolve(T data) noexcept {
      p->data = data;
      p->timer.cancel();
    }

    asio::awaitable<T> async_wait() {
      asio::error_code ec;
      co_await p->timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
      assert(ec == asio::error::operation_aborted);
      co_return result();
    }

    T result() const {
      return p->data;
    }
  private:
    struct Impl {
      asio::steady_timer timer;
      T data;
    };
    std::shared_ptr<Impl> p;
};
