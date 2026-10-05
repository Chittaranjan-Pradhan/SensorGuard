# Convenience wrapper around CMake and the kernel build.
BUILD ?= build

.PHONY: all test driver driver-clean clean demo
all:
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD) -j

test: all
	cd $(BUILD) && ctest --output-on-failure

driver:
	$(MAKE) -C driver

driver-clean:
	$(MAKE) -C driver clean

demo: all
	scripts/run_demo.sh $(BUILD)

clean: driver-clean
	rm -rf $(BUILD) demo.log demo.csv
