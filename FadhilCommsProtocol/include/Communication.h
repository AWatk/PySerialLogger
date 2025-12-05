/**
 * @file Communication.h
 * @brief Serial communication protocol handler for embedded command processing
 * 
 * This class implements a frame-based serial protocol using '<' and '>' delimiters.
 * Commands are formatted as: <command,arg1,arg2,...,argN>
 * 
 * Features:
 * - Frame-based parsing with start '<' and end '>' markers
 * - CSV-style argument tokenization
 * - Command registry with metadata for auto-documentation
 * - Built-in help system (JSON format)
 * - Case-insensitive command matching
 * 
 * Example usage:
 *   Communication comm;
 *   comm.begin(115200);
 *   comm.addCommand("led", &handleLED, DETAILS_LED);
 *   
 *   void loop() {
 *     comm.processSerial();
 *   }
 * 
 * @author Vanderbilt RASL
 */

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>

constexpr size_t kMaxMsg  = 128;   ///< Maximum message length between < and > markers
constexpr size_t kMaxArgs = 24;    ///< Maximum number of comma-separated arguments
constexpr size_t kMaxCmds = 16;    ///< Maximum number of registered commands

/**
 * @brief Function pointer type for command handlers
 * 
 * Handler functions receive a NULL-terminated array of argument strings.
 * - inputs[0] = first argument after command name
 * - inputs[1] = second argument
 * - ...
 * - inputs[N] = nullptr (terminator)
 * 
 * Example:
 *   Command: <led,1,HIGH>
 *   Handler receives: inputs = ["1", "HIGH", nullptr]
 */
using HandleFunc = void (*)(char** inputs);

/**
 * @class Communication
 * @brief Manages serial command parsing, dispatching, and help documentation
 */
class Communication {
public:
  /**
   * @brief Default constructor
   * 
   * Initializes internal state. The built-in "help" command is always available.
   */
  Communication();

  /**
   * @brief Initialize serial communication
   * @param baud Baud rate for serial port (default: 115200)
   * 
   * Must be called in setup() before processSerial() is used.
   */
  void begin(unsigned long baud = 115200);

  /**
   * @brief Process incoming serial data
   * 
   * Call this repeatedly in loop(). Checks for complete frames and dispatches
   * commands when a full message is received between '<' and '>' markers.
   */
  void processSerial();

  /**
   * @brief Register a new command handler
   * @param cmd Command string to match (case-insensitive), e.g., "led", "motor"
   * @param func Function pointer to handler that processes this command
   * @param cmdDetails JSON metadata string describing command syntax (see CommandDetails.h)
   * @return true if command registered successfully, false if registry is full
   * 
   * Example:
   *   comm.addCommand("blink", &handleBlink, DETAILS_BLINK);
   * 
   * The cmdDetails string should be a raw string literal containing valid JSON
   * describing the command's arguments and variants for auto-documentation.
   */
  bool addCommand(const char* cmd, HandleFunc func, const char* cmdDetails);

private:
  /**
   * @brief Receive serial data and assemble frames between '<' and '>' markers
   * 
   * State machine that:
   * - Waits for '<' to start frame capture
   * - Accumulates characters into rx_buf_
   * - Ignores carriage returns '\r'
   * - Terminates and parses frame when '>' is received
   * - Handles buffer overflow by aborting frame
   */
  void recvWithStartEndMarkers();

  /**
   * @brief Parse the received frame and dispatch to appropriate handler
   * 
   * - Tokenizes rx_buf_ by commas
   * - First token is the command name
   * - Remaining tokens are arguments passed to handler
   * - Handles built-in "help" command
   * - Outputs error messages for empty or unknown commands
   */
  void parseAndDispatch();

  /**
   * @brief Split a comma-separated string into token array
   * @param buffer Input string to tokenize (modified in-place)
   * @param tokens Output array of char* pointers (NULL-terminated)
   * 
   * Converts "cmd,arg1,arg2" into tokens = ["cmd", "arg1", "arg2", nullptr]
   * Leading whitespace before each token is trimmed.
   * Commas are replaced with null terminators.
   */
  void tokenize(char* buffer, char** tokens);

  /**
   * @brief Output JSON help documentation for all registered commands
   * 
   * Triggered by "<help>" command. Outputs format:
   * {"commands":{"cmd1":{...},"cmd2":{...}}}
   * 
   * Each command's metadata comes from the cmdDetails string provided
   * during addCommand().
   */
  void printHelpJson();

  const char* cmdNames_[kMaxCmds];    ///< Registered command names
  HandleFunc  cmdFuncs_[kMaxCmds];    ///< Corresponding handler functions
  const char* cmdDetails_[kMaxCmds];  ///< JSON metadata for each command
  int         cmdCount_ = 0;          ///< Number of registered commands

  char   rx_buf_[kMaxMsg];   ///< Buffer for incoming message between < and >
  size_t rx_idx_ = 0;        ///< Current write position in rx_buf_
  bool   in_frame_ = false;  ///< True when currently receiving a frame
};

#endif