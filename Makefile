all: prepare main-linux main-windows

main-linux: main.c
	cc -o ./build/main-linux main.c -I./3rd/curl-8.21.0/include 3rd/curl-8.21.0/lib/.libs/libcurl.a -lz -lcrypto -lssl -ldl -lpthread -lbrotlidec -lzstd -lnghttp2 -lpsl -lidn2 -lcares -DCURL_STATICLIB

main-windows: main.c
	x86_64-w64-mingw32-gcc -I./3rd/curl-8.21.0_6-win64-mingw/include main.c -o ./build/main-windows.exe -L./3rd/curl-8.21.0_6-win64-mingw/lib -lcurl
	cp ./3rd/curl-8.21.0_6-win64-mingw/bin/libcurl-x64.dll ./build/
	# Automatically make a zip package (main-windows.exe needs a curl DLL)
	mkdir -p ./build/smac-windows-pkg
	cp ./build/main-windows.exe ./build/smac-windows-pkg/
	cp ./3rd/curl-8.21.0_6-win64-mingw/bin/libcurl-x64.dll ./build/smac-windows-pkg/
	zip -r ./build/smac-windows-pkg.zip ./build/smac-windows-pkg
	rm -r ./build/smac-windows-pkg

prepare:
	rm -r ./build 2>/dev/null
	mkdir -p ./build
