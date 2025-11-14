#ifndef COMMAND_DETAILS_H
#define COMMAND_DETAILS_H

// --- Enable ---
static const char* DETAILS_ENABLE = R"({
  "label": "Enable system",
  "variants": {
    "default": {
      "command": "<e>",
      "inputs": []
    }
  }
})";

// --- Disable ---
static const char* DETAILS_DISABLE = R"({
  "label": "Disable system",
  "variants": {
    "default": {
      "command": "<d>",
      "inputs": []
    }
  }
})";

// --- Setpoint ---
static const char* DETAILS_SETPOINT = R"({
  "label": "Set temperature setpoints",
  "variants": {
    "all": {
      "command": "<s,VAL>",
      "inputs": [ { "name": "VAL", "type": "float" } ]
    },
    "relative": {
      "command": "<s,+DELTA>",
      "inputs": [ { "name": "DELTA", "type": "float" } ]
    },
    "single": {
      "command": "<s,IDX,VAL>",
      "inputs": [
        { "name": "IDX", "type": "int" },
        { "name": "VAL", "type": "float" }
      ]
    }
  }
})";

// --- DAC single channel ---
static const char* DETAILS_DAC_ONE = R"({
  "label": "Set DAC channel voltage",
  "variants": {
    "default": {
      "command": "<v,idx,volts>",
      "inputs": [
        { "name": "idx", "type": "int" },
        { "name": "volts", "type": "float" }
      ]
    }
  }
})";

// --- DAC: write first N mapped channels ---
const char* DETAILS_DAC_ARRAY = R"({
  "label": "Set first N DAC channels",
  "variants": {
    "default": {
      "command": "<va,v1,v2,...>",
      "inputs": [
        { "name": "v1", "type": "float", "units": "V", "repeat": true  },
      ]
    }
  }
})";

// --- DAC: set all mapped channels to same voltage ---
const char* DETAILS_DAC_ALL = R"({
  "label": "Set all DAC channels to same voltage",
  "variants": {
    "default": {
      "command": "<vr,volts>",
      "inputs": [
        { "name": "volts", "type": "float", "units": "V" }
      ]
    }
  }
})";

// --- Telemetry: report temps + setpoints ---
const char* DETAILS_TEMPS = R"({
  "label": "Report temperatures and setpoints",
  "variants": {
    "default": {
      "command": "<t>",
      "inputs": []
    }
  }
})";

// --- DAC reset: load CLR value to outputs ---
const char* DETAILS_RESET = R"({
  "label": "Reset DAC to CLR value",
  "variants": {
    "default": {
      "command": "<r>",
      "inputs": []
    }
  }
})";


// ... add more DETAILS_* definitions for each command

#endif
