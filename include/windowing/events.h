#ifndef FLUIDSIM_INCLUDE_WINDOWING_EVENTS_H_
#define FLUIDSIM_INCLUDE_WINDOWING_EVENTS_H_

#include <cstdint>

namespace chove::windowing {

struct WindowExtent {
  uint32_t width;
  uint32_t height;
};

struct WindowPosition {
  int x;
  int y;
};

enum class MouseButton : std::uint8_t {
  kLeft,
  kRight,
  kMiddle,
  kButton4,
  kButton5,
  kButton6,
  kButton7,
  kButton8,
};

enum class KeyCode : std::int16_t {
  kUnknown = -1,
  kSpace = 32,
  kApostrophe = 39,
  kComma = 44,
  kMinus = 45,
  kPeriod = 46,
  kSlash = 47,
  k0 = 48,
  k1 = 49,
  k2 = 50,
  k3 = 51,
  k4 = 52,
  k5 = 53,
  k6 = 54,
  k7 = 55,
  k8 = 56,
  k9 = 57,
  kSemicolon = 59,
  kEqual = 61,
  kA = 65,
  kB = 66,
  kC = 67,
  kD = 68,
  kE = 69,
  kF = 70,
  kG = 71,
  kH = 72,
  kI = 73,
  kJ = 74,
  kK = 75,
  kL = 76,
  kM = 77,
  kN = 78,
  kO = 79,
  kP = 80,
  kQ = 81,
  kR = 82,
  kS = 83,
  kT = 84,
  kU = 85,
  kV = 86,
  kW = 87,
  kX = 88,
  kY = 89,
  kZ = 90,
  kLeftBracket = 91,
  kBackslash = 92,
  kRightBracket = 93,
  kGraveAccent = 96,
  kEscape = 256,
  kEnter = 257,
  kTab = 258,
  kBackspace = 259,
  kInsert = 260,
  kDelete = 261,
  kRight = 262,
  kLeft = 263,
  kDown = 264,
  kUp = 265,
  kPageUp = 266,
  kPageDown = 267,
  kHome = 268,
  kEnd = 269,
  kCapsLock = 280,
  kScrollLock = 281,
  kNumLock = 282,
  kPrintScreen = 283,
  kLeftShift = 340,
  kLeftControl = 341,
  kLeftAlt = 342,
  kLeftSuper = 343,
  kRightShift = 344,
  kRightControl = 345,
  kRightAlt = 346,
  kRightSuper = 347,
};

enum class EventType : std::uint8_t {
  kWindowClose,
  kWindowResize,
  kKeyPressed,
  kKeyReleased,
  kMouseMoved,
  kMouseButtonPressed,
  kMouseButtonReleased,
};

class Event {
 public:
  [[nodiscard]] virtual EventType type() const = 0;
  virtual ~Event() = default;
};

class WindowCloseEvent : public Event {
 public:
  [[nodiscard]] EventType type() const override;
};

class WindowResizeEvent : public Event {
 public:
  explicit WindowResizeEvent(WindowExtent new_extent);

  [[nodiscard]] EventType type() const override;

  [[nodiscard]] uint32_t new_width() const;
  [[nodiscard]] uint32_t new_height() const;
  [[nodiscard]] WindowExtent new_extent() const;
 private:
  WindowExtent new_extent_;
};

class KeyPressedEvent : public Event {
 public:
  explicit KeyPressedEvent(KeyCode key_code);

  [[nodiscard]] EventType type() const override;

  [[nodiscard]] KeyCode key_code() const;
 private:
  KeyCode key_code_;
};

class KeyReleasedEvent : public Event {
 public:
  explicit KeyReleasedEvent(KeyCode key_code);

  [[nodiscard]] EventType type() const override;

  [[nodiscard]] KeyCode key_code() const;
 private:
  KeyCode key_code_;
};

class MouseButtonPressedEvent : public Event {
 public:
  explicit MouseButtonPressedEvent(MouseButton button);

  [[nodiscard]] EventType type() const override;

  [[nodiscard]] MouseButton button() const;
 private:
  MouseButton button_;
};

class MouseButtonReleasedEvent : public Event {
 public:
  explicit MouseButtonReleasedEvent(MouseButton button);

  [[nodiscard]] EventType type() const override;

  [[nodiscard]] MouseButton button() const;
 private:
  MouseButton button_;
};

} // namespace chove::windowing

#endif //FLUIDSIM_INCLUDE_WINDOWING_EVENTS_H_
