# Trollworks SDK Core

Trollworks is an (unfinished) game engine in C++ I've been working on for a
while.

This repository contains the basis of the SDK, as a header-only C++23 library.
It is built around [EnTT](https://github.com/skypjack/entt), an ECS library.

This library provides:

 - a game loop abstraction (with fixed updates, updates and late updates)
 - some EnTT utilities as singletons:
    - asset manager
    - event dispatcher
    - scene manager
    - job manager
 - a UI component framework inspired by React to help organize immediate-mode UI code
 - coroutines inspired by Unity coroutines

## Installation

Clone the repository in your project (don't forget to pull the submodules) and
add the `include/` folder to your include paths.

## Usage

### Game loop

```cpp
#include <trollworks.hpp>

struct game_state {
  // ...
};

struct listener {
  game_state& gs;

  void on_setup(tw::controlflow& cf) {
    // create window and opengl context, load resources
  }

  void on_teardown() {
    // free resources, opengl context and window
  }

  void on_frame_begin(tw::controlflow& cf) {
    // process window events and inputs
  }

  void on_fixed_update(float delta_time, tw::controlflow& cf) {
    // physics updates
  }

  void on_update(float delta_time, tw::controlflow& cf) {
    // game logic
  }

  void on_late_update(float delta_time, tw::controlflow& cf) {
    // more game logic
  }

  void on_render() {
    // render graphics
  }

  void on_frame_end(tw::controlflow& cf) {
    // ...
  }
};

int main() {
  auto gs = game_state{};
  auto l = listener{.gs = gs};

  auto loop = tw::game_loop{}
    .with_fps(60)
    .with_ups(50)
    .on_setup<&listener::on_setup>(l)
    .on_teardown<&listener::on_teardown>(l)
    .on_frame_begin<&listener::on_frame_begin>(l)
    .on_frame_end<&listener::on_frame_end>(l)
    .on_update<&listener::on_update>(l)
    .on_fixed_update<&listener::on_fixed_update>(l)
    .on_late_update<&listener::on_late_update>(l)
    .on_render<&listener::on_render>(l);

  loop.run();
  return 0;
}
```

**NB:** A concept `backend_trait` is also provided to facilitate pluging in a
specific window/event/render system (SDL, raylib, glfw, ...):

```cpp
struct sdl_backend {
  SDL_Window *window;
  SDL_Renderer *renderer;

  void setup(tw::controlflow& cf) {
    // create window, opengl context, ...
  }

  void teardown() {
    // free opengl context and window
  }

  void poll_events(tw::controlflow& cf) {
    // process event, for example with SDL:
    SDL_Event evt;

    while (SDL_PollEvents(&evt)) {
      switch (evt.type) {
        case SDL_QUIT:
          cf = tw::controlflow::exit;
          break;

        default:
          // ...
          break;
      }
    }
  }

  void render() {
    // get current scene's entity registry
    auto& registry = tw::scene_manager::main().registry();

    SDL_RenderClear(renderer);

    // iterate over your entities to draw them

    // render UI with imgui, or nuklear, or other

    SDL_RenderPresent(renderer);
  }
};

int main() {
  auto back = sdl_backend{};

  auto loop = tw::game_loop{}
    .with_fps(60)
    .with_ups(50)
    .with_backend(back);

  loop.run();
  return 0;
}
```

### Coroutines

First, create your coroutine function:

```cpp
tw::coroutine count(int n) {
  for (auto i = 0; i < n; i++) {
    co_yield tw::coroutine::none{};
  }
}
```

Then, in your game loop:

```cpp
tw::coroutine_manager::main().start_coroutine(count(5));
```

Coroutines are run after the update hook and before the late update hook.

Coroutines can also be chained, like in Unity:

```cpp
tw::coroutine count2(int a, int b) {
  co_yield count(a);
  co_yield count(b);
}
```

### Scene management

A scene is a class providing 2 methods (`load` and `unload`):

```cpp
class my_scene final : public tw::scene {
  public:
    virtual void load(entt::registry& registry) override {
      // create entities and components
    }

    virtual void unload(entt::registry& registry) override {
      // destroy entities and components
    }
};
```

Then:

```cpp
tw::scene_manager::main().load(my_scene{});
```

### Assets

The asset manager provides a singleton per asset type. The singleton is simply a
resource cache from EnTT, for more information consult
 [this page](https://github.com/skypjack/entt/wiki/Crash-Course:-resource-management).

```cpp
struct my_asset {
  // ...

  struct loader_type {
    using result_type = std::shared_ptr<my_asset>;

    result_type operator()(/* ... */) const {
      // ...
    }
  };
};

auto& cache = tw::asset_manager<my_asset>::cache();
```

### Messaging

The message bus is simply a singleton returning an `entt::dispatcher`. For more
information, please consult
[this page](https://github.com/skypjack/entt/wiki/Crash-Course:-events,-signals-and-everything-in-between#event-dispatcher).

```cpp
auto& dispatcher = tw::message_bus::main();
```

Queued messages are dispatched after the late update hook an before rendering.

### Jobs

The job manager is simply a singleton returing an `entt::basic_scheduler<float>`.
For more information, please consult
[this page](https://github.com/skypjack/entt/wiki/Crash-Course:-cooperative-scheduler).

### UI framework

```cpp
using namespace entt::literals;

struct foo {
  void operator()(tw::ui::hooks& h) {
    auto& local_state = h.use_state<int>(0);

    // gui code
  }
};

struct bar {
  void operator()(tw::ui::hooks& h) {
    auto& local_state = h.use_state<float>(0.0f);

    // gui code
  }
};

struct root {
  bool condition;

  void operator()(tw::ui::hooks& h) {
    h.render<foo>("a"_hs);

    if (condition) {
      h.render<foo>("b"_hs);
    }

    h.render<bar>("c"_hs);
  }
};
```

Then in your render hook:

```cpp
tw::ui::h<root>("root"_hs, root{.condition = true});
```

The ids given to the UI hooks must be unique within the component, not globally.

## License

This project is released under the terms of the [MIT License](./LICENSE.txt).
