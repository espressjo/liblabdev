PREFIX = /usr/local

.PHONY: soem

submodule:
	git submodule update --init --recursive

soem: submodule
	mkdir -p ./soem/build
	cd ./soem/build && cmake ..
	cd ./soem/build && make -j$(nproc)

prometheuscpp:
	mkdir -p ./prometheus-cpp/build
	cd ./prometheus-cpp/build && cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF
	cd ./prometheus-cpp/build && cmake --build . --parallel 4
install:
	cd ./soem/build && cmake --install . --prefix $(PREFIX)
	cd ./prometheus-cpp/build && cmake --install .
all: soem prometheuscpp
clean:
	rm -rf ./soem/build
	rm -rf ./prometheus-cpp/build
