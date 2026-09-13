SMAC_VERSION=0.0.1
EXE_CLI=smac
EXE_GUI=smac-gui

linux: linux-cli linux-gui

windows: windows-cli windows-gui

linux-gui: prepare-build generator smac-gui.cpp
	g++ -std=c++17 -fPIC smac-gui.cpp -o ./release/linux/$(EXE_GUI) \
		$$(pkg-config --cflags Qt6Core Qt6Gui Qt6Widgets Qt6UiTools) \
		$$(pkg-config --libs   Qt6Core Qt6Gui Qt6Widgets Qt6UiTools) \
		-lcurl
	cp interface.ui ./release/linux/ 2>/dev/null

linux-cli: prepare-build generator smac.c
	cc -o ./release/linux/$(EXE_CLI) smac.c \
		-lcurl
# TODO: static build
#./3rd/curl-8.21.0/lib/.libs/libcurl.a \
#-lssl -lcrypto -ldl -lm -lz -DCURL_STATICLIB -lbrotlicommon -lbrotlidec -lzstd -lcares -lpsl -lidn2 -lnghttp2

windows-gui:
	# [NOTE] IF YOU ALREADY ARE IN MINGW64 SHELL, JUST RUN `make windows-mingw64`
	C:/msys64/mingw64/bin/make.exe windows-gui-mingw64

windows-gui-mingw64: prepare-build generator smac-gui.cpp windows/CMakeLists.txt
	# [NOTE] YOU MUST EXECUTE THIS VIA `msys2/mingw64` SHELL
	cd ./build && \
		cmake -DCMAKE_PREFIX_PATH=C:/msys64/mingw64/qt6-static ../windows && \
		cmake --build .
	cp ./3rd/curl-8.21.0_6-win64-mingw/bin/libcurl-x64.dll ./release/windows/src/
	mv ./build/smac-gui.exe ./release/windows/src/
	mv ./build/*.dll ./release/windows/src/
	cp interface.ui ./release/windows/src/ 2>/dev/null
	cp ./assets/icon.ico ./release/windows/src/
	cp ./windows/create_relative_shortcut.bat ./release/windows/
	cd ./release/windows/ && \
		./create_relative_shortcut.bat
	rm ./release/windows/create_relative_shortcut.bat

windows-cli: prepare-build generator smac.c
	cc -o ./release/windows/src/$(EXE_CLI).exe smac.c \
		-lcurl

generator: # this is to generate functions for multi-threads
	cc -o ./src/generator ./src/threaded_decl_generator.c
	./src/generator
	rm ./src/generator

prepare-build:
	@rm -rf ./build 2>/dev/null
	@mkdir -p ./build
	@mkdir -p ./release/{linux,windows/{,src}}

pack-releases: pack-linux pack-windows

pack-linux:
	cd ./release && \
		tar cvf smac-linux-$(SMAC_VERSION).tar.gz ./linux

pack-windows:
	cd ./release && \
		zip -r smac-windows-$(SMAC_VERSION).zip ./windows

# personal build case from linux machine
windows-send-build:
	rsync -hurtPl . win11:Desktop/smac-windows \
		--exclude={.git*,3rd/curl-8.21.0{,*zip,*gz},release,build}
	ssh win11 "cd ./Desktop/smac-windows; make windows"

windows-receive-release:
	rsync -hurtPl win11:Desktop/smac-windows/release/windows \
		./release

windows-full-build: windows-send-build windows-receive-release pack-windows

linux-full-build: linux pack-linux

full-build: linux-full-build windows-full-build
