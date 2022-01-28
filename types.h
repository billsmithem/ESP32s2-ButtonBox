// required because of weird way arduino compiles code

typedef struct {
  char layout[100];
} Config;

typedef enum ButtonMode {
  Off,
  On,
  Pulse
} ButtonMode;

typedef struct Button {
  int buttonVal;
  bool isChars;
  char keys[20];
  bool control;
  bool shift;
  bool alt;
} Button;