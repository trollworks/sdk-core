#pragma once

#include <algorithm>
#include <exception>
#include <coroutine>
#include <vector>

#include "../entt/entt.hpp"

namespace tw {
  class coroutine {
    public:
      struct none {};
      struct promise_type;
      using handle_type = std::coroutine_handle<promise_type>;

      class promise_type {
        public:
          promise_type() = default;
          promise_type(const promise_type&) = delete;
          promise_type(promise_type&&) = delete;

          coroutine get_return_object() noexcept {
            return coroutine{*this};
          }

          std::suspend_always initial_suspend() noexcept {
            return {};
          }

          std::suspend_always final_suspend() noexcept {
            return {};
          }

          void unhandled_exception() noexcept {
            m_exc = std::current_exception();
          }

          void return_void() noexcept {}

          std::suspend_always yield_value(none&) noexcept {
            return {};
          }

          std::suspend_always yield_value(none&&) noexcept {
            return {};
          }

          auto yield_value(coroutine& from) noexcept {
            class awaitable {
              public:
                awaitable(promise_type* child) : m_child(child) {}

                bool await_ready() const noexcept {
                  return m_child == nullptr;
                }

                void await_suspend(handle_type) noexcept {}

                void await_resume() noexcept {
                  if (m_child != nullptr) {
                    m_child->throw_if_exception();
                  }
                }

              private:
                promise_type* m_child;
            };

            if (from.m_promise != nullptr) {
              m_root->m_parent = from.m_promise;
              from.m_promise->m_root = m_root;
              from.m_promise->m_parent = this;
              from.m_promise->resume();

              if (!from.m_promise->done()) {
                return awaitable{from.m_promise};
              }

              m_root->m_parent = this;
            }

            return awaitable{nullptr};
          }

          auto yield_value(coroutine&& from) noexcept {
            return yield_value(from);
          }

          template <typename T>
          std::suspend_never await_transform(T&&) = delete;

          void destroy() noexcept {
            handle_type::from_promise(*this).destroy();
          }

          void throw_if_exception() {
            if (m_exc) {
              std::rethrow_exception(m_exc);
            }
          }

          bool done() noexcept {
            return handle_type::from_promise(*this).done();
          }

          void poll() noexcept {
            m_parent->resume();

            while (m_parent != this && m_parent->done()) {
              m_parent = m_parent->m_parent;
              m_parent->resume();
            }
          }

        private:
          void resume() noexcept {
            handle_type::from_promise(*this).resume();
          }

        private:
          std::exception_ptr m_exc{nullptr};
          promise_type *m_root{this};
          promise_type *m_parent{this};
      };

    public:
      coroutine() = default;
      coroutine(promise_type& promise) : m_promise(&promise) {}
      coroutine(coroutine&& other) : m_promise(other.m_promise) {
        other.m_promise = nullptr;
      }
      coroutine(const coroutine&) = delete;
      coroutine& operator=(const coroutine&) = delete;

      coroutine& operator=(coroutine&& other) noexcept {
        if (this != &other) {
          if (m_promise != nullptr) {
            m_promise->destroy();
          }

          m_promise = other.m_promise;
          other.m_promise = nullptr;
        }

        return *this;
      }

      ~coroutine() {
        if (m_promise != nullptr) {
          m_promise->destroy();
        }
      }

      bool done() const {
        return m_promise == nullptr || m_promise->done();
      }

      void resume() {
        m_promise->poll();

        if (m_promise->done()) {
          auto* temp = m_promise;
          m_promise = nullptr;
          temp->throw_if_exception();
        }
      }

    private:
      promise_type *m_promise{nullptr};
  };

  class coroutine_manager {
    public:
      static coroutine_manager& main() {
        if (!entt::locator<coroutine_manager>::has_value()) {
          entt::locator<coroutine_manager>::emplace();
        }

        return entt::locator<coroutine_manager>::value();
      }

      void start_coroutine(coroutine&& coro) {
        m_coroutines.push_back(std::move(coro));
      }

      void update() {
        for (auto& coro : m_coroutines) {
          coro.resume();
        }

        m_coroutines.erase(
          std::remove_if(
            m_coroutines.begin(),
            m_coroutines.end(),
            [](const coroutine& coro) {
              return coro.done();
            }
          ),
          m_coroutines.end()
        );
      }

      bool empty() const {
        return m_coroutines.empty();
      }

    private:
      std::vector<coroutine> m_coroutines;
  };
}
