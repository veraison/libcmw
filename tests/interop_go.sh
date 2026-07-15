#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GO_REF="${GO_REF:-$ROOT/../cmw}"
BUILD="${BUILD:-$ROOT/build}"
GO_BIN="${GO_BIN:-$(command -v go || true)}"

if [[ -z "$GO_BIN" && -x /usr/local/go/bin/go ]]; then
  GO_BIN=/usr/local/go/bin/go
fi

if [[ -z "$GO_BIN" ]]; then
  echo "Go compiler not found in PATH" >&2
  exit 1
fi

if [[ ! -d "$GO_REF" ]]; then
  echo "Go reference repo not found at $GO_REF" >&2
  exit 1
fi

if [[ ! -x "$BUILD/tests/cmw_interop" ]]; then
  echo "C++ interop tool not built at $BUILD/tests/cmw_interop" >&2
  echo "Run: cmake --build $BUILD" >&2
  exit 1
fi

export PATH="$(dirname "$GO_BIN"):$PATH"

go_roundtrip_cbor() {
  (cd "$GO_REF" && GOWORK=off "$GO_BIN" run "$ROOT/tests/interop/main.go" roundtrip-cbor "$1")
}

go_roundtrip_json() {
  (cd "$GO_REF" && GOWORK=off "$GO_BIN" run "$ROOT/tests/interop/main.go" roundtrip-json "$1")
}

cxx_roundtrip_cbor() {
  "$BUILD/tests/cmw_interop" roundtrip-cbor "$1"
}

cxx_roundtrip_json() {
  "$BUILD/tests/cmw_interop" roundtrip-json "$1"
}

assert_equal() {
  local name="$1"
  local expected="$2"
  local actual="$3"
  if [[ "$expected" != "$actual" ]]; then
    echo "FAIL: $name" >&2
    echo " expected: $expected" >&2
    echo "   actual: $actual" >&2
    exit 1
  fi
  echo "OK: $name"
}

# Spec examples (RFC 9999 section 5)
JSON_RECORD='["application/vnd.example.rats-conceptual-msg", "I0faVQ"]'
CBOR_RECORD_HEX='82 19 fde7 44 2347da55'
CBOR_TAG_HEX='da 6374ffe6 44 2347da55'

# Go -> C++ -> Go (CBOR)
go_cbor_out="$(go_roundtrip_cbor "$CBOR_RECORD_HEX")"
cxx_cbor_out="$(cxx_roundtrip_cbor "$go_cbor_out")"
go_cbor_roundtrip="$(go_roundtrip_cbor "$cxx_cbor_out")"
assert_equal "Go->C++->Go CBOR record" "$go_cbor_out" "$go_cbor_roundtrip"

# C++ -> Go -> C++ (CBOR)
cxx_cbor_out2="$(cxx_roundtrip_cbor "$CBOR_RECORD_HEX")"
go_cbor_out2="$(go_roundtrip_cbor "$cxx_cbor_out2")"
cxx_cbor_roundtrip="$(cxx_roundtrip_cbor "$go_cbor_out2")"
assert_equal "C++->Go->C++ CBOR record" "$cxx_cbor_out2" "$cxx_cbor_roundtrip"

# Go -> C++ -> Go (CBOR tag)
go_tag_out="$(go_roundtrip_cbor "$CBOR_TAG_HEX")"
cxx_tag_out="$(cxx_roundtrip_cbor "$go_tag_out")"
go_tag_roundtrip="$(go_roundtrip_cbor "$cxx_tag_out")"
assert_equal "Go->C++->Go CBOR tag" "$go_tag_out" "$go_tag_roundtrip"

# Go -> C++ -> Go (JSON record)
go_json_out="$(go_roundtrip_json "$JSON_RECORD")"
cxx_json_out="$(cxx_roundtrip_json "$go_json_out")"
go_json_roundtrip="$(go_roundtrip_json "$cxx_json_out")"
assert_equal "Go->C++->Go JSON record" "$go_json_out" "$go_json_roundtrip"

# C++ -> Go -> C++ (JSON record)
cxx_json_out2="$(cxx_roundtrip_json "$JSON_RECORD")"
go_json_out2="$(go_roundtrip_json "$cxx_json_out2")"
cxx_json_roundtrip="$(cxx_roundtrip_json "$go_json_out2")"
assert_equal "C++->Go->C++ JSON record" "$cxx_json_out2" "$cxx_json_roundtrip"

# Go test vectors from testdata/
COLLECTION_JSON="$(cat "$ROOT/tests/testdata/collection-ok.json" | tr -d '\n')"
go_collection_json="$(go_roundtrip_json "$COLLECTION_JSON")"
cxx_collection_json="$(cxx_roundtrip_json "$go_collection_json")"
go_collection_roundtrip="$(go_roundtrip_json "$cxx_collection_json")"
assert_equal "Go->C++->Go JSON collection" "$go_collection_json" "$go_collection_roundtrip"

COLLECTION_CBOR_HEX="$(xxd -p "$ROOT/tests/testdata/collection-cbor-ok.cbor" | tr -d '\n')"
go_collection_cbor="$(go_roundtrip_cbor "$COLLECTION_CBOR_HEX")"
cxx_collection_cbor="$(cxx_roundtrip_cbor "$go_collection_cbor")"
go_collection_cbor_roundtrip="$(go_roundtrip_cbor "$cxx_collection_cbor")"
assert_equal "Go->C++->Go CBOR collection" "$go_collection_cbor" "$go_collection_cbor_roundtrip"

COLLECTION_CBOR_2_HEX="$(xxd -p "$ROOT/tests/testdata/collection-cbor-ok-2.cbor" | tr -d '\n')"
cxx_collection2="$(cxx_roundtrip_cbor "$COLLECTION_CBOR_2_HEX")"
go_collection2="$(go_roundtrip_cbor "$cxx_collection2")"
cxx_collection2_roundtrip="$(cxx_roundtrip_cbor "$go_collection2")"
assert_equal "C++->Go->C++ CBOR collection" "$cxx_collection2" "$cxx_collection2_roundtrip"

echo
echo "All cross-language interop checks passed."
