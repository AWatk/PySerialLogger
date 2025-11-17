#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>

constexpr size_t kMaxMsg  = 128;   // max raw message length inside <...>
constexpr size_t kMaxArgs = 24;    // max number of tokens
constexpr size_t kMaxCmds = 16;    // max registered commands

// Every handler gets NULL-terminated tokens: inputs[0], inputs[1], ..., nullptr
using HandleFunc = void (*)(char** inputs);

class Communication {
public:
  // Communication() = default;
  Communication();
  //include help thing that lists all respective methods
  void begin(unsigned long baud = 115200);
  void processSerial();                         // call this in loop()

  // cmd is the string to match against inputs[0], e.g. "led", "blink"
  bool addCommand(const char* cmd, HandleFunc func, const char* cmdDetails);
  //void commands();
  //void help(); // built in command, tied to "h" or "help"

private:
  void recvWithStartEndMarkers();               // assemble rx_buf_ between < >
  void parseAndDispatch();                      // tokenize then dispatch handler
  void tokenize(char* buffer, char** tokens);   // split by ',' into char* array
  void printHelpJson(); 

  // registry of known commands
  const char* cmdNames_[kMaxCmds];
  HandleFunc  cmdFuncs_[kMaxCmds];
  const char* cmdDetails_[kMaxCmds];
  int         cmdCount_ = 0;

  const char* helpString_ = "This is the help string\0";

  // receive state machine
  char   rx_buf_[kMaxMsg];
  size_t rx_idx_ = 0;
  bool   in_frame_ = false;
};

#endif


