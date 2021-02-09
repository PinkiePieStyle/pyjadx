# PyJadx

PyJadx provides Python binding for [Jadx decompiler](https://github.com/skylot/jadx)

As Jadx is written in Java, it use the JNI interface through [jni.hpp](https://github.com/mapbox/jni.hpp) to make the bridge Java / C++ then it uses [pybind11](https://github.com/pybind/pybind11)
to create the bridge C++ / Python.


<p align="center" >
<img width="50%" src="https://github.com/romainthomas/pyjadx/blob/master/.github/archi.png"/><br />
</p>


## Getting Start

```bash
  $ git clone https://github.com/romainthomas/pyjadx.git
  $ cd pyjadx
  $ python ./setup.py install --user
```


## Windows only
Get https://github.com/dlfcn-win32/dlfcn-win32
Copy dlfcn.c + dlfcn.h to pyjadx/src

You can also build the documentation with:

```bash
  $ python ./setup.py build_sphinx
```

The generated doc will be located in ``doc/_build/html``

## Example

```python
   import pyjadx

   jadx = pyjadx.Jadx()
   app = jadx.load("my.apk")

   for cls in app.classes:
     print(cls.code)
```

See API reference [here](http://pyjadx.romainthomas.fr)

## Credits

This application uses Open Source components. You can find the source code of their open source projects along with license information below.
We acknowledge and are grateful to these developers for their contributions to open source.

Jadx:

```
https://github.com/skylot/jadx

Copyright 2018 by Skylot

Licensed under the Apache 2.0 License
```


PyBind11:

```
https://github.com/pybind/pybind11

Wenzel Jakob - http://rgl.epfl.ch/people/wjakob

License (BSD) https://github.com/pybind/pybind11/blob/master/LICENSE
```

jni.hpp

```
https://github.com/mapbox/jni.hpp

mapbox - https://www.mapbox.com/

License: https://github.com/mapbox/jni.hpp/blob/master/LICENSE.txt
```

## Authors

Romain Thomas `@rh0main <https://twitter.com/rh0main>`_ - `Quarkslab <https://www.quarkslab.com>`_

