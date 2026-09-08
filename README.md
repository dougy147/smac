`smac` is a MAC accounts scanner.

It's kind of a rewrite of [`mcbash`](https://github.com/dougy147/mcbash) with a GUI and Windows/Linux compatibility.

# Compile from source

TODO

## Windows

To know which DLLs are mandatory (include them next to .exe):

```console
$ objdump.exe -p ./release/windows/src/main-windows.exe | grep "DLL Name"
```

# SOURCES

- Curl source code (Linux):   https://curl.se/download/curl-8.21.0.tar.gz
- Curl source code (Windows): https://curl.se/windows/dl-8.21.0_6/curl-8.21.0_6-win64-mingw.zip

