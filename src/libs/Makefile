.PHONY: tinyexpr_build math2wav_build

build: tinyexpr_build math2wav_build

tinyexpr_build:
	echo "  CC      $(BUILD_DIR)/tinyexpr.o"
	$(CC) $(CFLAGS) -c tinyexpr.c -o $(BUILD_DIR)/tinyexpr.o

math2wav_build:
	echo "  CC      $(BUILD_DIR)/math2wav.o"
	$(CC) $(CFLAGS) -c math2wav.c -o $(BUILD_DIR)/math2wav.o
