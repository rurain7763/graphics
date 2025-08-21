CC = clang++
CFLAGS = -std=c++17 -Wall -Wextra -g
DEFINES = -D_DEBUG
INCLUDES = -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/OpenEXR -I/opt/homebrew/include/Imath
SRCS = 	$(wildcard src/*.cpp) \
		$(wildcard src/Log/*.cpp) \
		$(wildcard src/Platform/Mac/*.cpp) \
		$(wildcard src/Graphics/Vulkan/*.cpp) \
		$(wildcard src/Event/*.cpp) \
		$(wildcard src/Time/*.cpp) \
		$(wildcard src/Image/*.cpp) \
		$(wildcard src/Model/*.cpp) \
		$(wildcard src/Input/*.cpp) \

RPATH = -Wl,-rpath,/usr/local/lib

LIBS = 	-lvulkan \
		-L/opt/homebrew/lib -lspdlog -lfmt -lglfw -lopenexr -lassimp
	   
build:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) $(SRCS) -o bin/out.exe $(LIBS) $(RPATH)

run:
	./bin/out.exe

clean:
	rm -f bin/out.exe