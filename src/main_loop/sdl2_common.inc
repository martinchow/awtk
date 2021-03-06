#include <stdio.h>
#include "base/input_method.h"

static ret_t main_loop_sdl2_dispatch_text_input(main_loop_simple_t* loop, SDL_Event* sdl_event) {
  im_commit_event_t e;
  SDL_TextInputEvent* text_input_event = (SDL_TextInputEvent*)sdl_event;

  e.e = event_init(EVT_IM_COMMIT, NULL);
  e.text = text_input_event->text;

  return input_method_dispatch_to_widget(input_method(), &(e.e));
}

static ret_t main_loop_sdl2_dispatch_text_editing(main_loop_simple_t* loop, SDL_Event* sdl_event) {
  return RET_OK;
}

static ret_t main_loop_sdl2_dispatch_key_event(main_loop_simple_t* loop, SDL_Event* sdl_event) {
  key_event_t event;
  int type = sdl_event->type;
  widget_t* widget = loop->base.wm;

  event.e.time = time_now_ms();
  event.key = sdl_event->key.keysym.sym;
  switch (type) {
    case SDL_KEYDOWN: {
      event.e.type = EVT_KEY_DOWN;
      window_manager_dispatch_input_event(widget, (event_t*)&event);
      break;
    }
    case SDL_KEYUP: {
      event.e.type = EVT_KEY_UP;
      window_manager_dispatch_input_event(widget, (event_t*)&event);
      break;
    }
    default:
      break;
  }

  return RET_OK;
}

#define MIN_WHEEL_DELTA 12
static ret_t main_loop_sdl2_dispatch_wheel_event(main_loop_simple_t* loop, SDL_Event* sdl_event) {
  wheel_event_t event;
  widget_t* widget = loop->base.wm;

  event.e = event_init(EVT_WHEEL, widget);
  event.dx = sdl_event->wheel.x;
  event.dy = sdl_event->wheel.y;

  if (event.dy > 0) {
    event.dy = tk_max(MIN_WHEEL_DELTA, event.dy);
  } else if (event.dy < 0) {
    event.dy = tk_min(-MIN_WHEEL_DELTA, event.dy);
  }
  window_manager_dispatch_input_event(widget, (event_t*)&event);

  return RET_OK;
}

static ret_t main_loop_sdl2_dispatch_mouse_event(main_loop_simple_t* loop, SDL_Event* sdl_event) {
  pointer_event_t event;
  int type = sdl_event->type;
  widget_t* widget = loop->base.wm;

  event.e.time = time_now_ms();
  switch (type) {
    case SDL_MOUSEBUTTONDOWN: {
      loop->pressed = 1;
      event.e.type = EVT_POINTER_DOWN;
      event.x = sdl_event->button.x;
      event.y = sdl_event->button.y;
      event.button = sdl_event->button.button;
      event.pressed = loop->pressed;

      window_manager_dispatch_input_event(widget, (event_t*)&event);
      break;
    }
    case SDL_MOUSEBUTTONUP: {
      event.e.type = EVT_POINTER_UP;
      event.x = sdl_event->button.x;
      event.y = sdl_event->button.y;
      event.button = sdl_event->button.button;
      event.pressed = loop->pressed;

      window_manager_dispatch_input_event(widget, (event_t*)&event);
      loop->pressed = 0;
      break;
    }
    case SDL_MOUSEMOTION: {
      event.e.type = EVT_POINTER_MOVE;
      event.x = sdl_event->motion.x;
      event.y = sdl_event->motion.y;
      event.button = 0;
      event.pressed = loop->pressed;

      window_manager_dispatch_input_event(widget, (event_t*)&event);
      break;
    }
    default:
      break;
  }

  return RET_OK;
}

static ret_t main_loop_sdl2_dispatch_window_resize(main_loop_simple_t* loop, uint32_t win_id, int w,
                                                   int h) {
  lcd_resize(loop->base.canvas.lcd, w, h);
  return window_manager_resize(loop->base.wm, w, h);
}

static ret_t main_loop_sdl2_dispatch_window_event(main_loop_simple_t* loop, SDL_Event* event) {
  switch (event->window.event) {
    case SDL_WINDOWEVENT_SHOWN:
      log_debug("Window %d shown\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_HIDDEN:
      log_debug("Window %d hidden\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_EXPOSED:
      log_debug("Window %d exposed\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_MOVED:
      log_debug("Window %d moved to %d,%d\n", event->window.windowID, event->window.data1,
                event->window.data2);
      break;
    case SDL_WINDOWEVENT_RESIZED:
      log_debug("Window %d resized to %dx%d\n", event->window.windowID, event->window.data1,
                event->window.data2);
      break;
    case SDL_WINDOWEVENT_SIZE_CHANGED:
      log_debug("Window %d size changed to %dx%d\n", event->window.windowID, event->window.data1,
                event->window.data2);
      main_loop_sdl2_dispatch_window_resize(loop, event->window.windowID, event->window.data1,
                                            event->window.data2);
      break;
    case SDL_WINDOWEVENT_MINIMIZED:
      log_debug("Window %d minimized\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_MAXIMIZED:
      log_debug("Window %d maximized\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_RESTORED:
      log_debug("Window %d restored\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_ENTER:
      log_debug("Mouse entered window %d\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_LEAVE:
      log_debug("Mouse left window %d\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_FOCUS_GAINED:
      log_debug("Window %d gained keyboard focus\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_FOCUS_LOST:
      log_debug("Window %d lost keyboard focus\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_CLOSE:
      log_debug("Window %d closed\n", event->window.windowID);
      loop->user1 = NULL;
      loop->user2 = NULL;
      main_loop_quit(&(loop->base));
      break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
    case SDL_WINDOWEVENT_TAKE_FOCUS:
      log_debug("Window %d is offered a focus\n", event->window.windowID);
      break;
    case SDL_WINDOWEVENT_HIT_TEST:
      log_debug("Window %d has a special hit test\n", event->window.windowID);
      break;
#endif
    default:
      log_debug("Window %d got unknown event %d\n", event->window.windowID, event->window.event);
      break;
  }

  widget_invalidate_force(loop->base.wm);

  return RET_OK;
}

static ret_t main_loop_sdl2_dispatch(main_loop_simple_t* loop) {
  SDL_Event event;
  ret_t ret = RET_OK;

  while (SDL_PollEvent(&event) && loop->base.running) {
    switch (event.type) {
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        ret = main_loop_sdl2_dispatch_key_event(loop, &event);
        break;
      }
      case SDL_MOUSEMOTION:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP: {
        ret = main_loop_sdl2_dispatch_mouse_event(loop, &event);
        break;
      }
      case SDL_TEXTINPUT: {
        ret = main_loop_sdl2_dispatch_text_input(loop, &event);
        break;
      }
      case SDL_TEXTEDITING: {
        ret = main_loop_sdl2_dispatch_text_editing(loop, &event);
        break;
      }
      case SDL_QUIT: {
        ret = main_loop_quit(&(loop->base));
        break;
      }
      case SDL_MOUSEWHEEL: {
        ret = main_loop_sdl2_dispatch_wheel_event(loop, &event);
        break;
      }
      case SDL_WINDOWEVENT: {
        main_loop_sdl2_dispatch_window_event(loop, &event);
        break;
      }
    }
  }

  return ret;
}
