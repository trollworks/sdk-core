#include "doctest.h"

#include "../include/trollworks.hpp"

struct an_event {
  int value;
};

struct world {
  int value{0};
};

struct listener {
  world& w;

  void on_event(const an_event& e) {
    w.value = e.value;
  }
};

TEST_CASE("message bus") {
  auto &bus = tw::message_bus::main();
  auto w = world{};
  auto l = listener{w};

  bus.sink<an_event>().connect<&listener::on_event>(l);

  bus.trigger(an_event{42});
  CHECK(w.value == 42);

  bus.enqueue(an_event{24});
  CHECK(w.value == 42);
  bus.update();
  CHECK(w.value == 24);
}
