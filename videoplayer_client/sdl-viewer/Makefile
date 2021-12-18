SRCS := $(shell find . -name "*.c"  -not -name "*_template.c")
TARGET=viewer
CFLAGS := -g -I./ffmpeg-src -O0 -w -std=c99 -Wno-implicit-function-declaration -D_GNU_SOURCE
LDFLAGS := -lm -L/usr/local/lib -L/usr/local/Cellar/sdl2/2.0.8/lib -lSDL2

OBJS := $(SRCS:%.c=%.o)

h264dec: $(OBJS)
	$(CC) -g -o $(TARGET) -I/usr/local/include -I/usr/local/Cellar/sdl2/2.0.8/include  $(OBJS) $(LDFLAGS)
	@echo ""
	@echo "+--------------------------------------------+"
	@echo "|      Finish compilation video-$(TARGET)          |"
	@echo "+--------------------------------------------+"
	@echo "|        Copyright(c) voipman@qq.com         |"
	@echo "+--------------------------------------------+"

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)
