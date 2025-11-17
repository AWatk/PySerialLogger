#ifndef COMMAND_DETAILS_H
#define COMMAND_DETAILS_H

// --- Enable ---
static const char* DETAILS_ENABLE = R"({
  "label": "Enable system",
  "command": "e",
  "default": {
    "name": "default",
    "inputs": []
  },
  "variants": []
})";

// --- Disable ---
static const char* DETAILS_DISABLE = R"({
  "label": "Disable system",
  "command": "d",
  "default": {
    "name": "default",
    "inputs": []
  },
  "variants": []
})";

// --- Setpoint ---
static const char* DETAILS_SETPOINT = R"({
  "label": "Set temperature setpoints",
  "command": "s",
  "default": {
    "name": "single",
    "inputs": [
      { "name": "IDX", "type": "int" },
      { "name": "VAL", "type": "float" }
    ]
  },
  "variants": [
    {
      "name": "all",
      "inputs": [
        { "name": "VAL", "type": "float" }
      ]
    }
  ]
})";

// --- DAC: single channel ---
static const char* DETAILS_DAC_ONE = R"({
  "label": "Set DAC channel voltage",
  "command": "v",
  "default": {
    "name": "single",
    "inputs": [
      { "name": "IDX", "type": "int" },
      { "name": "VOLTS", "type": "float" }
    ]
  },
  "variants": []
})";

// --- DAC: write first N mapped channels ---
static const char* DETAILS_DAC_ARRAY = R"({
  "label": "Set first N DAC channels",
  "command": "va",
  "default": {
    "name": "array",
    "inputs": [
      { "name": "V", "type": "float", "repeat": true }
    ]
  },
  "variants": []
})";

// --- DAC: set all mapped channels to same voltage ---
static const char* DETAILS_DAC_ALL = R"({
  "label": "Set all DAC channels to same voltage",
  "command": "vr",
  "default": {
    "name": "uniform",
    "inputs": [
      { "name": "VOLTS", "type": "float" }
    ]
  },
  "variants": []
})";

// --- Telemetry: temps + setpoints ---
static const char* DETAILS_TEMPS = R"({
  "label": "Report temperatures and setpoints",
  "command": "t",
  "default": {
    "name": "default",
    "inputs": []
  },
  "variants": []
})";

// --- DAC Reset ---
static const char* DETAILS_RESET = R"({
  "label": "Reset DAC to CLR value",
  "command": "r",
  "default": {
    "name": "default",
    "inputs": []
  },
  "variants": []
})";

#endif
