#include "doctest.h"

#include "../include/trollworks.hpp"

struct world {
  int loaded{0};
  int unloaded{0};
};

class scene final : public tw::scene {
  public:
    scene(int value) : m_value(value) {}

    virtual void load(entt::registry& registry) override {
      registry.ctx().get<world&>().loaded = m_value;
    }

    virtual void unload(entt::registry& registry) override {
      registry.ctx().get<world&>().unloaded = m_value;
    }

  private:
    int m_value;
};

TEST_CASE("scene manager") {
  auto& mgr = tw::scene_manager::main();
  auto w = world{};
  mgr.registry().ctx().emplace<world&>(w);

  mgr.load(scene{42});
  CHECK(w.loaded == 42);

  mgr.load(scene{24});
  CHECK(w.unloaded == 42);
  CHECK(w.loaded == 24);
}
