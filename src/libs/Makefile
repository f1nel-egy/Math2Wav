.PHONY: math2wav_build ssf_build

build: math2wav_build ssf_build

math2wav_build:
	echo "  CC      $(BUILD_DIR)/math2wav.o"
	$(CC) $(CFLAGS) -c math2wav.c -o $(BUILD_DIR)/math2wav.o

ssf_build:
	echo "  CC      $(BUILD_DIR)/ssf.o"
	$(CC) $(CFLAGS) -c ssf.c -o $(BUILD_DIR)/ssf.o
