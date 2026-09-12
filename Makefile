EXE_LINUX=main-linux
EXE_WINDOWS=main-windows.exe # ensure it is correct in CMakeList.txt!

linux: prepare smac-gui.cpp
	g++ -std=c++17 -fPIC smac-gui.cpp -o ./release/linux/$(EXE_LINUX) \
		$$(pkg-config --cflags Qt6Core Qt6Gui Qt6Widgets Qt6UiTools) \
		$$(pkg-config --libs   Qt6Core Qt6Gui Qt6Widgets Qt6UiTools) \
		-lcurl
	cp interface.ui ./release/linux/ 2>/dev/null

windows:
	# [IMPORTANT] IF YOU ALREADY ARE IN MINGW64 SHELL, SIMPLY
	#             RUN     make windows-mingw64
	C:/msys64/mingw64/bin/make.exe windows-mingw64

windows-mingw64: prepare smac-gui.cpp CMakeLists.txt
	# [IMPORTANT] YOU MUST EXECUTE THIS VIA `msys2/mingw64` SHELL
	#             ELSE YOU WILL HAVE TROUBLES WITH STATIC COMPILE
	cd ./build && \
		cmake -DCMAKE_PREFIX_PATH=C:/msys64/mingw64/qt6-static .. && \
		cmake --build .
	cp ./3rd/curl-8.21.0_6-win64-mingw/bin/libcurl-x64.dll ./release/windows/src/
	mv ./build/$(EXE_WINDOWS) ./release/windows/src/
	mv ./build/*.dll ./release/windows/src/
	cp interface.ui ./release/windows/src/ 2>/dev/null
	cp icon.ico ./release/windows/src/
	cp ./tools/create_relative_shortcut.bat ./release/windows/
	cd ./release/windows/ && \
		./create_relative_shortcut.bat
	rm ./release/windows/create_relative_shortcut.bat

windows-sendbuild:
	rsync -hurtPl . win11:Desktop/main-sendbuild \
		--exclude={.git*,3rd/curl-8.21.0{,*zip,*gz},release,build}
	ssh win11 "cd ./Desktop/main-sendbuild; make windows"

linux-cli: generator smac.c
	cc -o smac-cli smac.c \
		./3rd/curl-8.21.0/lib/.libs/libcurl.a \
		-lssl -lcrypto -ldl -lm -lz -DCURL_STATICLIB -lbrotlicommon -lbrotlidec -lzstd -lcares -lpsl -lidn2 -lnghttp2

prepare: generator
	#@rm -rf ./build 2>/dev/null
	@mkdir -p ./build
	@mkdir -p ./release/{linux,windows/{,src}}

generator: 
	cc -o ./src/generator ./src/threaded_decl_generator.c
	./src/generator
	rm ./src/generator

