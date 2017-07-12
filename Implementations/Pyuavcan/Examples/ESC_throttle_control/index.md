---
---

# ESC throttle control

This example script demonstrates how to control ESC throttle setpoint using the Pyuavcan library.
Before running the script, make sure that no other node on the bus issues contradictory ESC commands concurrently.

## Writing the script

One code sample is worth 1024 words:

```python
{% include_relative sine_throttle_output.py %}
```

## Running the script

Save the above code somewhere and run it.
The connected ESC will be changing their RPM in a sine pattern, slowly accelerating and decelerating.
The script will periodically print output similar to this:

```
### Message from 124 to All  ts_mono=19376.645693  ts_real=1470440665.872391
error_count: 0
voltage: 13.2812
current: 1.3379
temperature: 313.15
rpm: 1514
power_rating_pct: 13
esc_index: 3
```

## Using the UAVCAN GUI Tool

It is also possible to run the above script (with minor modifications)
directly from the interactive console of the [UAVCAN GUI Tool](/GUI_Tool),
because the UAVCAN GUI Tool is built on top of Pyuavcan.
In that case you won’t need to create a new node yourself in the script - just use the application’s own node,
it is accessible from the interactive console.
For details, please read the documentation of the UAVCAN GUI Tool.

{% include lightbox.html url="/Implementations/Pyuavcan/Examples/ESC_throttle_control/uavcan_gui_tool.png" title="UAVCAN GUI Tool running this example script" %}
