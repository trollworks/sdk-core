#include "doctest.h"

#include "../include/trollworks.hpp"

struct game_state {
  bool setup{false};
  bool teardown{false};
};

struct listener {
  game_state& gs;

  void on_setup(tw::controlflow& cf) {
    gs.setup = true;
  }

  void on_teardown() {
    gs.teardown = true;
  }

  void on_update(float dt, tw::controlflow& cf) {
    cf = tw::controlflow::exit;
  }
};

TEST_CASE("game_loop") {
  auto gs = game_state{};
  auto l = listener{.gs = gs};

  auto loop = tw::game_loop{}
    .on_setup<&listener::on_setup>(l)
    .on_teardown<&listener::on_teardown>(l)
    .on_update<&listener::on_update>(l);

  loop.run();
  CHECK(gs.setup);
  CHECK(gs.teardown);
}
