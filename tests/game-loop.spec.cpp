#include "doctest.h"

#include "../include/trollworks.hpp"

struct game_state {
  bool setup{false};
  bool teardown{false};
};

struct listener {
  game_state& game_state;

  void on_setup(tw::controlflow& cf) {
    game_state.setup = true;
  }

  void on_teardown() {
    game_state.teardown = true;
  }

  void on_update(float dt, tw::controlflow& cf) {
    cf = tw::controlflow::exit;
  }
};

TEST_CASE("game_loop") {
  auto gs = game_state{};
  auto l = listener{.game_state = gs};

  auto loop = tw::game_loop{}
    .on_setup<&listener::on_setup>(l)
    .on_teardown<&listener::on_teardown>(l)
    .on_update<&listener::on_update>(l);

  loop.run();
  CHECK(gs.setup);
  CHECK(gs.teardown);
}
