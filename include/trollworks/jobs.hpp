#pragma once

#include "../entt/entt.hpp"

namespace tw {
  template <typename T>
  using job = entt::process<T, float>;

  class job_manager {
    public:
      using scheduler = entt::basic_scheduler<float>;

      static scheduler& main() {
        if (!entt::locator<scheduler>::has_value()) {
          entt::locator<scheduler>::emplace();
        }

        return entt::locator<scheduler>::value();
      }
  };
}
