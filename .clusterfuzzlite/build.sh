#!/bin/bash -eu

# build fuzzers

apt-get update && apt-get install -y --no-install-recommends libcmocka-dev

pushd fuzzing
cmake -DBOLOS_SDK=../BOLOS_SDK -Bbuild -H.
make -C build
mv ./build/fuzz_tx "${OUT}"
popd
