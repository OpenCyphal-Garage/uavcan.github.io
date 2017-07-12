---
---

# Examples

This page collects some interesting or instructional use cases of the UAVCAN GUI Tool.

Since the GUI Tool is built on top of Pyuavcan,
you should also [check out its examples](/Implementations/Pyuavcan/Examples/) -
they are mostly compatible with the GUI Tool's embedded IPython console,
although some minor modifications may be required.

## Generating audiovisual feedback

Some nodes support UAVCAN messages from the namespace `uavcan.indication.*`
that allow one to generate arbitrary indications on remote UAVCAN nodes.
Here’s how this feature can be invoked from the interactive console of the UAVCAN GUI Tool:

```python
# Beeping at 1kHz for 0.1 seconds
broadcast(uavcan.equipment.indication.BeepCommand(frequency=1000, duration=0.1))

# Beeping at 1kHz for 0.1 seconds every 0.5 seconds for 5 seconds
broadcast(uavcan.equipment.indication.BeepCommand(frequency=1000, duration=0.1), interval=0.5, duration=5)

# Turning all RGB LEDs of index 0 magenta
broadcast(uavcan.equipment.indication.LightsCommand(commands=[
    uavcan.equipment.indication.SingleLightCommand(color=uavcan.equipment.indication.RGB565(red=31,
                                                                                            green=0,
                                                                                            blue=31))]))
```
