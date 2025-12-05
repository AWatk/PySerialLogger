/**
 * @file Communication.cpp
 * @brief Implementation of serial command protocol handler
 */

#include "Communication.h"

/**
 * @brief Constructor - initializes empty command registry
 */
Communication::Communication()
{
}

/**
 * @brief Initialize serial communication at specified baud rate
 * @param baud Serial baud rate (default: 115200)
 */
void Communication::begin(unsigned long baud) {
  Serial.begin(baud);
}

/**
 * @brief Register a command handler with metadata
 * @param cmd Command string (e.g., "led", "motor")
 * @param func Handler function pointer
 * @param cmdDetails JSON string describing command syntax
 * @return true if successful, false if command registry is full
 */
bool Communication::addCommand(const char* cmd, HandleFunc func, const char* cmdDetails) {
  if (cmdCount_ >= (int)kMaxCmds) return false;
  
  cmdNames_[cmdCount_] = cmd;
  cmdFuncs_[cmdCount_] = func;
  cmdDetails_[cmdCount_] = cmdDetails;
  ++cmdCount_;
  
  return true;
}

/**
 * @brief Main processing function - call this in loop()
 */
void Communication::processSerial() {
  recvWithStartEndMarkers();
}

/**
 * @brief State machine for receiving framed serial messages
 * 
 * Protocol format: <command,arg1,arg2,...>
 * 
 * Behavior:
 * 1. Wait for '<' character to begin frame
 * 2. Accumulate characters into rx_buf_
 * 3. Skip '\r' characters (carriage return)
 * 4. On '>' character: terminate frame and parse
 * 5. On buffer overflow: abort frame and reset
 */
void Communication::recvWithStartEndMarkers() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (!in_frame_) {
      if (c == '<') {
        in_frame_ = true;
        rx_idx_ = 0;
      }
      continue;
    }

    if (c == '>') {
      rx_buf_[min(rx_idx_, kMaxMsg - 1)] = '\0';
      in_frame_ = false;
      parseAndDispatch();
      rx_idx_ = 0;
      continue;
    }

    if (c == '\r') continue;

    if (rx_idx_ < kMaxMsg - 1) {
      rx_buf_[rx_idx_++] = c;
    } else {
      in_frame_ = false;
      rx_idx_ = 0;
    }
  }
}

/**
 * @brief Helper function to trim leading whitespace
 * @param p Pointer to string (modified to point past whitespace)
 */
static inline void trimLeading(char*& p) {
  while (*p == ' ' || *p == '\t') {
    ++p;
  }
}

/**
 * @brief Tokenize comma-separated string into array of pointers
 * @param buffer Input string (modified in-place: commas → null terminators)
 * @param tokens Output array of char* (NULL-terminated)
 * 
 * Converts: "blink,1.0,HIGH" 
 * Into: tokens = ["blink", "1.0", "HIGH", nullptr]
 */
void Communication::tokenize(char* buffer, char** tokens) {
  int idx = 0;
  char* p = buffer;
  
  trimLeading(p);
  tokens[idx++] = p;

  while (*p && idx < (int)kMaxArgs - 1) {
    if (*p == ',') {
      *p = '\0';
      char* nxt = p + 1;
      trimLeading(nxt);
      tokens[idx++] = nxt;
    }
    ++p;
  }

  tokens[idx] = nullptr;
}

/**
 * @brief Parse received message and dispatch to appropriate handler
 * 
 * Message format: "command,arg1,arg2,..."
 * - tokens[0] = command name
 * - tokens[1..N] = arguments
 * 
 * Built-in commands:
 * - "help": calls printHelpJson()
 * 
 * Registered commands are matched case-insensitively.
 * Handler receives pointer to arguments (tokens+1).
 */
void Communication::parseAndDispatch() {
  char* tokens[kMaxArgs];
  tokenize(rx_buf_, tokens);

  if (tokens[0] == nullptr || *tokens[0] == '\0') {
    Serial.println(F("ERR,EMPTY"));
    return;
  }

  if (strcasecmp(tokens[0], "help") == 0) {
    printHelpJson();
    return;
  }

  for (int i = 0; i < cmdCount_; ++i) {
    if (strcasecmp(tokens[0], cmdNames_[i]) == 0) {
      cmdFuncs_[i](tokens + 1);
      return;
    }
  }

  Serial.print(F("ERR,UNKNOWN_CMD,"));
  Serial.println(tokens[0]);
}

/**
 * @brief Output JSON documentation for all registered commands
 * 
 * Output format:
 * {"commands":{"cmd1":{...},"cmd2":{...}}}
 * 
 * Each command's metadata is a raw JSON object stored in cmdDetails_[i].
 */
void Communication::printHelpJson() {
  Serial.print(F("{\"commands\":{"));

  for (int i = 0; i < cmdCount_; ++i) {
    if (i) Serial.print(',');

    Serial.print('\"');
    Serial.print(cmdNames_[i]);
    Serial.print("\":");

    const char* details = cmdDetails_[i];
    if (details && *details) {
      Serial.print(details);
    }
  }
  
  Serial.println(F("}}"));
}