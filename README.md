# libcmw

C++ implementation of [RFC 9999](https://www.rfc-editor.org/rfc/rfc9999.html) (RATS Conceptual Message Wrapper).

This API is (heavily) inspired by the [Go implementation](https://github.com/veraison/cmw).

## Features

- Supported CMW types: Record, Tag (CBOR-only), and Collection
- JSON and CBOR serialization and deserialization

Note that the implementation does not currently support JWS/COSE signing.

## Dependencies

* [QCBOR](https://github.com/laurencelundblade/QCBOR)
* [cJSON](https://github.com/DaveGamble/cJSON)

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Example

```cpp
#include "cmw/cmw.hpp"

// Create a JSON record CMW
auto record = cmw::CMW::create_monad(
    "application/vnd.example.rats-conceptual-msg",
    std::vector<uint8_t>{0x23, 0x47, 0xda, 0x55});
auto json = record.value().marshal_json();

// Decode any CMW (auto-detects JSON/CBOR)
auto decoded = cmw::decode(*json);
```

## Interoperability Tests

The test suite uses the same vectors as the Go implementation (`tests/testdata/`) and the examples from the spec.

A series of round-trip tests verifies that CMWs produced by this library can be consumed by the Go library and vice versa.
To run the interop tests do:
```bash
cd tests
./interop_go.sh
```
Note that `interop_go.sh` script assumes that the clone of the veraison/cmw repo sits alongside the libcmw repo on the local file system.
If this is not the case, the location of the go repo can be modified using the `GO_REF` environment variable.
