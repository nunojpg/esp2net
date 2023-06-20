# esp2net

Does the same as https://github.com/espressif/esptool/blob/master/esp_rfc2217_server.py, but in a very small binary, mostly for OpenWRT routers.

**This tools is now available as a package on OpenWRT!**

## Quick start

```
git clone https://github.com/nunojpg/esp2net.git
cd esp2net
git submodule update --init
mkdir build
cd build
cmake ..
./esp2net /dev/ttyUSB0 5001
```

Now you can flash with idf.py:

```
idf.py flash --port rfc2217://127.0.0.1:5001
```

You can also monitor the device directly with nc/ncat:

```
nc 127.0.0.1 5001
```

or

```
ncat 127.0.0.1 5001
```

