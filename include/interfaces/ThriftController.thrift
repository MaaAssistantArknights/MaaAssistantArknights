namespace cpp ThriftController

struct Point {
  1: i32 x,
  2: i32 y,
}

struct ClickParam {
  1: Point point,
}

struct SwipeParam {
  1: Point point1,
  2: Point point2,
  3: i32 duration,
}

struct TouchParam {
  1: i32 contact,
  2: Point point,
}

enum InputEventType {
  TOUCH_DOWN = 0,
  TOUCH_UP = 1,
  TOUCH_MOVE = 2,
  TOUCH_RESET = 3,
  KEY_DOWN = 4,
  KEY_UP = 5,
  WAIT_MS = 6,
}

struct InputEvent {
  1: InputEventType type,
  2: TouchParam touch,
  3: i32 key,
  4: i32 wait_ms,
}

struct Size {
  1: i32 width,
  2: i32 height,
}

struct CustomImage {
  1: Size size,
  2: i32 type,
  3: binary data,
}

service ThriftController {
  bool connect(),

  bool set_option(1: string key, 2: string value),

  string get_uuid(),
  CustomImage screencap(),

  bool start_game(1: string activity),
  bool stop_game(1: string activity),

  bool click(1: ClickParam param),
  bool swipe(1: SwipeParam param),

  bool inject_input_events(1: list<InputEvent> events),

  bool press_key(1: i32 param),
  i64 get_support_features(),

  Size get_screen_res(),
}
