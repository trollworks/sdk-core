#pragma once

#include <type_traits>
#include <concepts>

#include "../entt/entt.hpp"

namespace tw {
  template <typename T>
  concept asset_trait = requires(T& asset) {
    typename T::resource_type;
    typename T::loader_type;
  };

  template <asset_trait A>
  class asset_manager {
    public:
      using cache_type = entt::resource_cache<
        typename A::resource_type,
        typename A::loader_type
      >;

      static cache_type& cache() {
        if (!entt::locator<cache_type>::has_value()) {
          entt::locator<cache_type>::emplace();
        }

        return entt::locator<cache_type>::value();
      }
  };
}
