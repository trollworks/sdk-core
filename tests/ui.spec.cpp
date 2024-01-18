#include "doctest.h"

#include "../include/trollworks.hpp"

using namespace entt::literals;

struct world {
  int foo{0};
  float bar{0.0f};
};

static world w = world{};

struct foo {
  void operator()(tw::ui::hooks& h) {
    auto& i = h.use_state<int>(42);
    w.foo = i;
    i = 24;
  }
};

struct bar {
  void operator()(tw::ui::hooks& h) {
    auto& f = h.use_state<float>(42.0f);
    w.bar = f;
    f = 24.0f;
  }
};

struct root {
  void operator()(tw::ui::hooks& h) {
    h.render<foo>(0);
    if (w.foo == 42) {
      h.render<foo>(1);
    }
    h.render<bar>(2);
  }
};

TEST_CASE("ui component tree") {
  tw::ui::h<root>(0);
  CHECK(w.foo == 42);
  CHECK(w.bar == 42.0f);

  tw::ui::h<root>(0);
  CHECK(w.foo == 24);
  CHECK(w.bar == 24.0f);
}
