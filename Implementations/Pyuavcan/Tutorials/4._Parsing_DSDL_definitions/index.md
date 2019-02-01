---
---

# Parsing DSDL definitions

Create a file named `namespace_a/Foo.uavcan` with the following content:

```python
uavcan.Timestamp timestamp
uint8 MY_FAVORITE_CHAR = '\x42'
```

Create another file named `namespace_b/Bar.uavcan` with the following content:

```python
float16 x
float16[<8] y
---
uint6 JUST_ENOUGH = 42
float64 answer
namespace_a.Foo foo
```

Place the standard DSDL definitions into the directory `dsdl/`
(just clone or symlink the DSDL repository into the current directory).

The resulting directory structure should look like this:

```python
namespace_a/
    Foo.uavcan
namespace_b/
    Bar.uavcan
dsdl/
    uavcan/
        ...
...
```

Put this code into a file in the current directory:

```python
#
# Pyuavcan DSDL parser demo.
# Read the package docstrings for details.
#

# pyuavcan supports Python 2.7 and Python 3.x
from __future__ import print_function
from uavcan import dsdl

# Root namespace directories:
NAMESPACE_DIRS = ['namespace_a/', 'namespace_b/']

# Where to look for the referenced types (standard UAVCAN types):
SEARCH_DIRS = ['dsdl/uavcan']

# Run the DSDL parser:
types = dsdl.parse_namespaces(NAMESPACE_DIRS, SEARCH_DIRS)

# Print what we got
ATTRIBUTE_INDENT = ' ' * 3

def print_attributes(fields, constants):
    for a in fields:
        print(ATTRIBUTE_INDENT, a.type.full_name, a.name, end='  # ')
        # Add a bit of relevant information for this type:
        if a.type.category == a.type.CATEGORY_COMPOUND:
            print('DSDL signature: 0x%08X' % a.type.get_dsdl_signature())
        if a.type.category == a.type.CATEGORY_ARRAY:
            print('Max array size: %d' % a.type.max_size)
        if a.type.category == a.type.CATEGORY_PRIMITIVE:
            print('Bit length: %d' % a.type.bitlen)
    for a in constants:
        print(ATTRIBUTE_INDENT, a.type.full_name, a.name, '=', a.value, '#', a.init_expression)

for t in types:
    print(t.full_name)
    if t.kind == t.KIND_MESSAGE:
        print_attributes(t.fields, t.constants)
    elif t.kind == t.KIND_SERVICE:
        print_attributes(t.request_fields, t.request_constants)
        print(ATTRIBUTE_INDENT, '---')
        print_attributes(t.response_fields, t.response_constants)
```

Make sure the Pyuavcan package is installed, then run the script.
The output should look like this:

```
namespace_a.Foo
    uavcan.Timestamp timestamp  # DSDL signature: 0xF776DD8697A8DB73
    saturated uint8 MY_FAVORITE_CHAR = 66 # '\x42'
namespace_b.Bar
    saturated float16 x  # Bit length: 16
    saturated float16[<=7] y  # Max array size: 7
    ---
    saturated float64 answer  # Bit length: 64
    namespace_a.Foo foo  # DSDL signature: 0xFE327FCBE8C6F7D
    saturated uint6 JUST_ENOUGH = 42 # 42
```

For a real world usage example it's recommended to refer to Libuavcan,
which implements a DSDL compiler based on the DSDL parsing module from Pyuavcan.
