Dependencies
===
rely on prometheus-cpp. Follow the steps to installl prometheus-cpp;

cd ./prometheus-cpp

# fetch third-party dependencies
git submodule init
git submodule update

mkdir _build
cd _build

# run cmake
cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF

# build
cmake --build . --parallel 4

# run tests
ctest -V

# install the libraries and headers
cmake --install .

