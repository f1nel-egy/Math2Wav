.PHONY: tinyexpr_build miniaudio_build

build: tinyexpr_build miniaudio_build

tinyexpr_build:
	echo "  CC      $(BUILD_DIR)/tinyexpr.o"
	$(CC) $(CFLAGS) -c tinyexpr.c -o $(BUILD_DIR)/tinyexpr.o

miniaudio_build:
	echo "  CC      $(BUILD_DIR)/miniaudio.o"
	$(CC) $(CFLAGS) -c miniaudio.c -o $(BUILD_DIR)/miniaudio.o
