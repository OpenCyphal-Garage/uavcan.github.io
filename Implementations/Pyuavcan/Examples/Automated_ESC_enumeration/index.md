---
---

# Automated ESC enumeration

In this example we'll demonstrate how the automated ESC enumeration feature works from the inside.
Please read the UAVCAN specification if the concept of automated enumeration is not familiar to you.

## Writing the script

```python
{% include_relative automated_esc_enumeration.py %}
```

## Running the code

Connect ESC to the CAN bus
(it's better to use multiple ESC, otherwise the auto-enumeration procedure becomes rather pointless),
start the script, and follow its instructions.

For each ESC you will see the output similar to this:

```
=== PROVIDE ENUMERATION FEEDBACK ON ESC INDEX 0 NOW ===
=== e.g. turn the motor, press the button, etc, depending on your equipment ===
### Message from 124 to All  ts_mono=94836.894482  ts_real=1470647890.850445
value:
  empty:
    {}
parameter_name: 'esc_index' # [101, 115, 99, 95, 105, 110, 100, 101, 120]
Indication received from node 124
Stopping enumeration on node 124
Setting config param 'esc_index' to 0...
### Response from 124 to 10  ts_mono=94837.077415  ts_real=1470647891.033378
value:
  integer_value: 0
default_value:
  integer_value: 0
max_value:
  integer_value: 15
min_value:
  integer_value: 0
name: 'esc_index' # [101, 115, 99, 95, 105, 110, 100, 101, 120]
### Response from 124 to 10  ts_mono=94837.122424  ts_real=1470647891.078387
argument: 0
ok: true
Node 124 assigned ESC index 0
Enumerated so far: [124]
```

## A relevant video

The following video demonstrates how the automated enumeration works on the user level.
Although it was not performed with the script shown in this example, the core principles remain the same.

<iframe width="560" height="315" src="https://www.youtube.com/embed/4nSa8tvpbgQ" frameborder="0" allowfullscreen></iframe>
