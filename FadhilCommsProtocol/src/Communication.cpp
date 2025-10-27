#include "Communication.h"

void Communication::begin(unsigned long baud) {
  Serial.begin(baud);
}

bool Communication::addCommand(const char* cmd, HandleFunc func) {
  if (cmdCount_ >= (int)kMaxCmds) return false;
  cmdNames_[cmdCount_] = cmd;
  cmdFuncs_[cmdCount_] = func;
  ++cmdCount_;
  return true;
}

void Communication::processSerial() {
  recvWithStartEndMarkers();
}

void Communication::recvWithStartEndMarkers() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    // not currently reading a frame: wait for '<'
    if (!in_frame_) {
      if (c == '<') {
        in_frame_ = true;
        rx_idx_ = 0;
      }
      continue;
    }

    // currently in a frame
    if (c == '>') {
      // end of frame -> terminate string and parse
      rx_buf_[min(rx_idx_, kMaxMsg - 1)] = '\0';
      in_frame_ = false;
      parseAndDispatch();
      rx_idx_ = 0;
      continue;
    }

    if (c == '\r') continue; // ignore carriage return

    // store char if room
    if (rx_idx_ < kMaxMsg - 1) {
      rx_buf_[rx_idx_++] = c;
    } else {
      // overflow: abort frame
      in_frame_ = false;
      rx_idx_ = 0;
    }
  }
}

// helper to skip spaces before each token
static inline void trimLeading(char*& p) {
  while (*p == ' ' || *p == '\t') {
    ++p;
  }
}

// Turn "blink,1.0" into tokens = ["blink","1.0",nullptr]
void Communication::tokenize(char* buffer, char** tokens) {
  int idx = 0;
  char* p = buffer;
  trimLeading(p);
  tokens[idx++] = p;

  while (*p && idx < (int)kMaxArgs - 1) {
    if (*p == ',') {
      *p = '\0';                // end current token
      char* nxt = p + 1;        // next token begins after comma
      trimLeading(nxt);
      tokens[idx++] = nxt;
    }
    ++p;
  }

  tokens[idx] = nullptr;        // null-terminate pointer list
}

void Communication::parseAndDispatch() {
  char* tokens[kMaxArgs];
  tokenize(rx_buf_, tokens);

  // tokens[0] = command string like "led" or "blink"
  if (tokens[0] == nullptr || *tokens[0] == '\0') {
    Serial.println(F("ERR,EMPTY"));
    return;
  }

  // look up which command handler to call
  for (int i = 0; i < cmdCount_; ++i) {
    // case-insensitive match
    if (strcasecmp(tokens[0], cmdNames_[i]) == 0) {
      cmdFuncs_[i](tokens + 1); //REMOVED +1 BROKE CODE
      return;
    }
  }

  Serial.print(F("ERR,UNKNOWN_CMD,"));
  Serial.println(tokens[0]);
}
