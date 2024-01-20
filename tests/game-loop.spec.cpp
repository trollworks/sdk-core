#include "doctest.h"

#include "../include/trollworks.hpp"

struct game_state {
  int setup{0};
  int teardown{0};
};

struct listener {
  game_state& gs;
  int val{0};

  void on_setup(tw::controlflow& cf) {
    gs.setup = val;
  }

  void on_teardown() {
    gs.teardown = val;
  }

  void on_update(float dt, tw::controlflow& cf) {
    cf = tw::controlflow::exit;
  }
};

TEST_CASE("game_loop") {
  auto gs = game_state{};
  auto l1 = listener{.gs = gs, .val = 1};
  auto l2 = listener{.gs = gs, .val = 2};

  auto loop = tw::game_loop{}
    .on_setup<&listener::on_setup>(l1)
    .on_teardown<&listener::on_teardown>(l1)
    .on_update<&listener::on_update>(l1)
    .on_setup<&listener::on_setup>(l2)
    .on_teardown<&listener::on_teardown>(l2);

  loop.run();
  CHECK(gs.setup == 2);
  CHECK(gs.teardown == 1);
}
