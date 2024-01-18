#pragma once

#include "../entt/entt.hpp"

namespace tw {
  class message_bus {
    public:
      static entt::dispatcher& main() {
        if (!entt::locator<entt::dispatcher>::has_value()) {
          entt::locator<entt::dispatcher>::emplace();
        }

        return entt::locator<entt::dispatcher>::value();
      }
  };
}
