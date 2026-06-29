#!/bin/bash -eu

export BOLOS_SDK="${BOLOS_SDK:-${SRC}/app-xrp/BOLOS_SDK}"

# Build fuzzers using the ClusterFuzzLite / OSS-Fuzz toolchain.
pushd fuzzing
cmake -S . -B build -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug \
            -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
            -DBOLOS_SDK="${BOLOS_SDK}"

# Package per-fuzzer seed corpora as <fuzzer>_seed_corpus.zip in $OUT.
# Each subdirectory of corpus/ is treated as one fuzzer's seed set, matching
# the libFuzzer convention used by ClusterFuzzLite.
for dir in corpus/*; do
    if [ -d "${dir}" ]; then
        fuzzer_name=$(basename "${dir}")
        zip_name="${fuzzer_name}_seed_corpus.zip"
        echo "Zipping corpus from ${dir} into ${zip_name}"

        (cd "${dir}" && zip -q -r "${zip_name}" .)

        mv "${dir}/${zip_name}" "${OUT}"
    fi
done

cmake --build build
mv ./build/fuzz_* "${OUT}"
popd
