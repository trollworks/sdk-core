#pragma once

#include <optional>
#include <chrono>
#include <thread>
#include <ranges>
#include <vector>

#include <type_traits>
#include <concepts>

#include "../entt/entt.hpp"

#include "./controlflow.hpp"
#include "./coroutine.hpp"
#include "./messaging.hpp"
#include "./jobs.hpp"

namespace tw {
  template <typename B>
  concept backend_trait = requires(B& backend, controlflow& cf) {
    { backend.setup(cf) } -> std::same_as<void>;
    { backend.teardown() } -> std::same_as<void>;
    { backend.poll_events(cf) } -> std::same_as<void>;
    { backend.render() } -> std::same_as<void>;
  };

  class game_loop {
    private:
      using cb_setup_type    = entt::delegate<void(controlflow&)>;
      using cb_teardown_type = entt::delegate<void()>;

      using cb_frame_begin_type = entt::delegate<void(controlflow&)>;
      using cb_frame_end_type   = entt::delegate<void(controlflow&)>;

      using cb_fixed_update_type = entt::delegate<void(float, controlflow&)>;
      using cb_update_type       = entt::delegate<void(float, controlflow&)>;
      using cb_late_update_type  = entt::delegate<void(float, controlflow&)>;

      using cb_render_type = entt::delegate<void()>;

      std::vector<cb_setup_type> m_sig_setup;
      std::vector<cb_teardown_type> m_sig_teardown;

      std::vector<cb_frame_begin_type> m_sig_frame_begin;
      std::vector<cb_frame_end_type> m_sig_frame_end;

      std::vector<cb_fixed_update_type> m_sig_fixed_update;
      std::vector<cb_update_type> m_sig_update;
      std::vector<cb_late_update_type> m_sig_late_update;

      std::vector<cb_render_type> m_sig_render;

    public:
      game_loop() = default;
      game_loop(const game_loop&) = delete;

      game_loop& with_fps(float fps) {
        if (fps < 0.0f) {
          throw std::invalid_argument("expected fps >= 0");
        }

        m_fps = fps;
        return *this;
      }

      game_loop& with_ups(float ups) {
        if (ups <= 0.0f || (m_fps > 0.0f && ups > m_fps)) {
          throw std::invalid_argument("expected 0 < ups <= fps");
        }

        m_ups = ups;
        return *this;
      }

      template <backend_trait B>
      game_loop& with_backend(B& backend) {
        on_setup<&B::setup>(backend);
        on_teardown<&B::teardown>(backend);
        on_frame_begin<&B::poll_events>(backend);
        on_render<&B::render>(backend);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_setup(Type&&... args) {
        auto delegate = cb_setup_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_setup.push_back(delegate);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_teardown(Type&&... args) {
        auto delegate = cb_teardown_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_teardown.push_back(delegate);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_frame_begin(Type&&... args) {
        auto delegate = cb_frame_begin_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_frame_begin.push_back(delegate);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_frame_end(Type&&... args) {
        auto delegate = cb_frame_end_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_frame_end.push_back(delegate);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_fixed_update(Type&&... args) {
        auto delegate = cb_fixed_update_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_fixed_update.push_back(delegate);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_update(Type&&... args) {
        auto delegate = cb_update_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_update.push_back(delegate);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_late_update(Type&&... args) {
        auto delegate = cb_late_update_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_late_update.push_back(delegate);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_render(Type&&... args) {
        auto delegate = cb_render_type{};
        delegate.template connect<Candidate>(std::forward<Type>(args)...);
        m_sig_render.push_back(delegate);
        return *this;
      }

      void run() {
        auto cf = controlflow::running;

        publish(m_sig_setup, cf);

        auto last_time = std::chrono::high_resolution_clock::now();
        auto lag = 0.0f;

        while (cf == controlflow::running) {
          auto current_time = std::chrono::high_resolution_clock::now();
          auto elapsed_time = current_time - last_time;
          auto delta_time = std::chrono::duration<float>(elapsed_time).count();

          lag += delta_time;
          last_time = current_time;

          publish(m_sig_frame_begin, cf);

          auto fixed_delta_time = 1.0f / m_ups;
          while (lag >= fixed_delta_time) {
            publish(m_sig_fixed_update, fixed_delta_time, cf);
            lag -= fixed_delta_time;
          }

          publish(m_sig_update, delta_time, cf);
          coroutine_manager::main().update();
          publish(m_sig_late_update, delta_time, cf);
          job_manager::main().update(delta_time, &cf);
          message_bus::main().update();

          publish(m_sig_render);

          publish(m_sig_frame_end, cf);

          auto frame_end = std::chrono::high_resolution_clock::now();
          auto frame_duration = frame_end - current_time;
          auto frame_time = std::chrono::duration<float>(frame_duration).count();

          if (m_fps > 0.0f) {
            auto max_time = 1.0f / m_fps;

            if (frame_time < max_time) {
              auto wait = static_cast<int>((max_time - frame_time) * 1000.0f);
              std::this_thread::sleep_for(std::chrono::milliseconds(wait));
            }
          }
        }

        publish(std::ranges::reverse_view{m_sig_teardown});
      }

    private:
      template <typename Signal, typename... Args>
      void publish(Signal s, Args&&... args) {
        for (auto& delegate : s) {
          delegate(std::forward<Args>(args)...);
        }
      }

    private:
      float m_fps{0.0f};
      float m_ups{50.0f};
  };
}
