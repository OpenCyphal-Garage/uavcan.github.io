mkdir build
cd build
cmake ..
make
./pan_galactic_gargle_blaster 42 vcan0  # Args: <node-id>, <can-iface-name>
