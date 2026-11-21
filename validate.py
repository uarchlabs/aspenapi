#! /bin/env python3
import json
from jsonschema import validate
from jsonschema.exceptions import ValidationError

# Define your JSON schema
#schema = {
#    "type": "object",
#    "properties": {
#        "name": {"type": "string"},
#        "age": {"type": "number"}
#    },
#    "required": ["name", "age"]
#}

schema = {
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "core": {
      "type": "object",
      "properties": {
        "instruction_set": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": {
              "mnemonic": { "type": "string" },
              "opc": { "type": "string" },
              "description": { "type": "string" },
              "encoding": {
                "type": "object",
                "properties": {
                  "fields": { "type": "string" },
                  "width": { "type": "integer" }
                },
                "required": ["fields", "width"]
              },
#              // ... other properties for each instruction
            },
            "required": ["mnemonic", "opc", "description", "encoding"]
          }
        },
#        // ... other properties of "core"
      },
      "required": ["instruction_set"]
    },
#    // ... other top-level properties
  },
  "required": ["core"]
}




# Load JSON data
with open('test/vx.json', 'r') as file:
    data = json.load(file)

# Validate JSON data
try:
    validate(instance=data, schema=schema)
    print("JSON data is valid.")
except ValidationError as e:
    print("JSON data is invalid.")
    #print(e)

