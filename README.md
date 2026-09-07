`smac` is a MAC accounts scanner for IPTV.

I first started to write it with GTK library for GUI (check the branch `gtk` of this repository).
Now I am experimenting with Qt.

# NOTE

If that does not work, ensure the current directory has a copy of "interface.ui". 
This is while we read the interface from a file.
Maybe someday we will cook it into the source code.


# Compile from source

## Linux

```console
$ mkdir ./3rd && cd ./3rd
$ wget https://curl.se/download/curl-8.21.0.tar.gz
$ tar xvf ./curl-8.21.0.tar.gz
$ cd -
$ make main-linux
```

## Windows

```console
$ mkdir ./3rd && cd ./3rd
$ wget https://curl.se/windows/dl-8.21.0_6/curl-8.21.0_6-win64-mingw.zip
$ unzip ./curl-8.21.0_6-win64-mingw.zip
$ cd -
$ make main-windows
```

To know which DLLs are mandatory (include them next to .exe):

```console
$ objdump.exe -p ./release/windows/src/main-windows.exe | grep "DLL Name"
```

# Sources

- Curl source code (Linux):   https://curl.se/download/curl-8.21.0.tar.gz
- Curl source code (Windows): https://curl.se/windows/dl-8.21.0_6/curl-8.21.0_6-win64-mingw.zip

