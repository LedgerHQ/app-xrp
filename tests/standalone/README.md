## Functional tests

```console
./speculos.py --log-level automation:DEBUG --automation file:$HOME/app-xrp/tests/automation.json ~/app-xrp/bin/app.elf &

export LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999
pytest-3 -v -s
```

Test a specific raw transaction:

```
export LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999
pytest-3 -v -s -k 09-check-create/02-issued.raw
```

## Unit tests

Build:

```console
cmake -Btests/build -Htests/
make -C tests/build/
```

Run:

```console
make -C tests/build/ test
```

Arguments can be given to `ctest`. For instance, to make the output of the test
`test_tx` verbose:

```console
make -C tests/build/ test ARGS='-V -R test_tx'
```

## Fuzzing


```console
export ASAN_SYMBOLIZER_PATH=/usr/lib/llvm-9/bin/llvm-symbolizer
./tests/build/fuzz_tx >/dev/null
```

```console
mkdir -p /tmp/corpus
find tests/testcases/ -name '*.raw' -exec cp '{}' /tmp/corpus/ ';'
./tests/build/fuzz_tx /tmp/corpus >/dev/null
```
