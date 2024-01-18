#pragma once

#include <optional>
#include <chrono>
#include <thread>

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
      using sigh_setup_type = entt::sigh<void(controlflow&)>;
      using sigh_teardown_type = entt::sigh<void()>;

      using sigh_frame_begin_type = entt::sigh<void(controlflow&)>;
      using sigh_frame_end_type = entt::sigh<void(controlflow&)>;

      using sigh_fixed_update_type = entt::sigh<void(float, controlflow&)>;
      using sigh_update_type = entt::sigh<void(float, controlflow&)>;
      using sigh_late_update_type = entt::sigh<void(float, controlflow&)>;

      using sigh_render_type = entt::sigh<void()>;

      sigh_setup_type m_sig_setup;
      sigh_teardown_type m_sig_teardown;

      sigh_frame_begin_type m_sig_frame_begin;
      sigh_frame_end_type m_sig_frame_end;

      sigh_fixed_update_type m_sig_fixed_update;
      sigh_update_type m_sig_update;
      sigh_late_update_type m_sig_late_update;

      sigh_render_type m_sig_render;

      entt::sink<sigh_setup_type> m_sink_setup{m_sig_setup};
      entt::sink<sigh_teardown_type> m_sink_teardown{m_sig_teardown};

      entt::sink<sigh_frame_begin_type> m_sink_frame_begin{m_sig_frame_begin};
      entt::sink<sigh_frame_end_type> m_sink_frame_end{m_sig_frame_end};

      entt::sink<sigh_fixed_update_type> m_sink_fixed_update{m_sig_fixed_update};
      entt::sink<sigh_update_type> m_sink_update{m_sig_update};
      entt::sink<sigh_late_update_type> m_sink_late_update{m_sig_late_update};

      entt::sink<sigh_render_type> m_sink_render{m_sig_render};

    public:
      game_loop() = default;

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
        m_sink_setup.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_teardown(Type&&... args) {
        m_sink_teardown.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_frame_begin(Type&&... args) {
        m_sink_frame_begin.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_frame_end(Type&&... args) {
        m_sink_frame_end.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_fixed_update(Type&&... args) {
        m_sink_fixed_update.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_update(Type&&... args) {
        m_sink_update.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_late_update(Type&&... args) {
        m_sink_late_update.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      template <auto Candidate, typename... Type>
      game_loop& on_render(Type&&... args) {
        m_sink_render.template connect<Candidate>(std::forward<Type>(args)...);
        return *this;
      }

      void run() {
        auto cf = controlflow::running;

        m_sig_setup.publish(cf);

        auto last_time = std::chrono::high_resolution_clock::now();
        auto lag = 0.0f;

        while (cf == controlflow::running) {
          auto current_time = std::chrono::high_resolution_clock::now();
          auto elapsed_time = current_time - last_time;
          auto delta_time = std::chrono::duration<float>(elapsed_time).count();

          lag += delta_time;
          last_time = current_time;

          m_sig_frame_begin.publish(cf);

          auto fixed_delta_time = 1.0f / m_ups;
          while (lag >= fixed_delta_time) {
            m_sig_fixed_update.publish(fixed_delta_time, cf);
            lag -= fixed_delta_time;
          }

          m_sig_update.publish(delta_time, cf);
          coroutine_manager::main().update();
          m_sig_late_update.publish(delta_time, cf);
          job_manager::main().update(delta_time, &cf);
          message_bus::main().update();

          m_sig_render.publish();

          m_sig_frame_end.publish(cf);

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

        m_sig_teardown.publish();
      }

    private:
      float m_fps{0.0f};
      float m_ups{50.0f};
  };
}
