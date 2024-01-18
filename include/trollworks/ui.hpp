#pragma once

#include <type_traits>
#include <concepts>

#include "../entt/entt.hpp"

namespace tw::ui {
  class hooks;

  template <typename T>
  concept component_trait = requires(T& cmp, hooks& hooks) {
    { cmp(hooks) } -> std::same_as<void>;
  };

  class hooks {
    public:
      static hooks& main() {
        if (!entt::locator<hooks>::has_value()) {
          entt::locator<hooks>::emplace();
        }

        return entt::locator<hooks>::value();
      }

      void reset() {
        m_state_index = 0;
      }

      template <typename T>
      T& use_state(T initial_val) {
        entt::id_type key = m_state_index++;

        if (!m_state.ctx().contains<T>(key)) {
          m_state.ctx().emplace_as<T>(key, initial_val);
        }

        return m_state.ctx().get<T>(key);
      }

      template <component_trait Component, typename... Args>
      void render(entt::id_type key, Args&&... args) {
        if (!m_components.ctx().contains<hooks>(key)) {
          m_components.ctx().emplace_as<hooks>(key);
        }

        auto component = Component{std::forward<Args>(args)...};
        auto& child_hooks = m_components.ctx().get<hooks>(key);
        child_hooks.reset();
        component(child_hooks);
      }

    private:
      entt::id_type m_state_index{0};
      entt::registry m_state;
      entt::registry m_components;
  };

  template <component_trait Component, typename... Args>
  void h(entt::id_type key, Args&&... args) {
    hooks::main().reset();
    hooks::main().render<Component>(key, std::forward<Args>(args)...);
  }
}
