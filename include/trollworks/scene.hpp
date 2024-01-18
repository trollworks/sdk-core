#pragma once

#include <type_traits>
#include <concepts>
#include <memory>

#include "../entt/entt.hpp"

namespace tw {
  class scene {
    public:
      virtual ~scene() = default;

      virtual void load(entt::registry& registry) = 0;
      virtual void unload(entt::registry& registry) = 0;
  };

  template <typename S>
  concept scene_trait = std::derived_from<S, scene>;

  class scene_manager {
    public:
      static scene_manager& main() {
        if (!entt::locator<scene_manager>::has_value()) {
          entt::locator<scene_manager>::emplace();
        }

        return entt::locator<scene_manager>::value();
      }

      template <scene_trait S>
      void load(S scene) {
        if (m_scene) {
          m_scene->unload(m_registry);
        }

        m_scene = std::make_unique<S>(std::move(scene));
        m_scene->load(m_registry);
      }

      const entt::registry& registry() const {
        return m_registry;
      }

      entt::registry& registry() {
        return m_registry;
      }

    private:
      std::unique_ptr<scene> m_scene{nullptr};
      entt::registry m_registry;
  };
}
