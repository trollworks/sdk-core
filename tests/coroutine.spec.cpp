#include "doctest.h"

#include "../include/trollworks.hpp"

struct world {
  int x{0};
  int y{0};
};

tw::coroutine test_count(int& dest, int n) {
  for (int i = 0; i < n; ++i) {
    dest++;
    co_yield tw::coroutine::none{};
  }
}

tw::coroutine test_count2(world& dest, int a, int b) {
  co_yield test_count(dest.x, a);
  co_yield test_count(dest.y, b);
}

TEST_CASE("coroutine") {
  auto &coromgr = tw::coroutine_manager::main();
  auto w = world{};

  coromgr.start_coroutine(test_count2(w, 3, 5));

  while (!coromgr.empty()) {
    coromgr.update();
  }

  CHECK(w.x == 3);
  CHECK(w.y == 5);
}
