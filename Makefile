SRC_DIR = src
MAKEFLAGS += -s

.PHONY: clean build run

all: clean .WAIT build .WAIT run

build:
	$(MAKE) -C $(SRC_DIR) build

run:
	$(MAKE) -C $(SRC_DIR) run

clean:
	$(MAKE) -C $(SRC_DIR) clean
