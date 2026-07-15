package main

import (
	"encoding/hex"
	"fmt"
	"os"

	"github.com/veraison/cmw"
)

func main() {
	if len(os.Args) < 2 {
		fatal("usage: interop <roundtrip-cbor|roundtrip-json|decode-kind> <payload>")
	}

	switch os.Args[1] {
	case "roundtrip-cbor":
		input, err := hex.DecodeString(stripSpaces(os.Args[2]))
		check(err)
		decoded, err := cmw.Decode(input)
		check(err)
		check(decoded.Valid())
		output, err := decoded.MarshalCBOR()
		check(err)
		fmt.Print(hex.EncodeToString(output))
	case "roundtrip-json":
		input := []byte(os.Args[2])
		decoded, err := cmw.Decode(input)
		check(err)
		check(decoded.Valid())
		output, err := decoded.MarshalJSON()
		check(err)
		fmt.Print(string(output))
	case "decode-kind":
		input, err := hex.DecodeString(stripSpaces(os.Args[2]))
		check(err)
		decoded, err := cmw.Decode(input)
		check(err)
		fmt.Print(decoded.GetKind().String())
	default:
		fatal("unknown mode: " + os.Args[1])
	}
}

func stripSpaces(s string) string {
	out := make([]byte, 0, len(s))
	for i := 0; i < len(s); i++ {
		if s[i] != ' ' && s[i] != '\n' && s[i] != '\t' {
			out = append(out, s[i])
		}
	}
	return string(out)
}

func check(err error) {
	if err != nil {
		fatal(err.Error())
	}
}

func fatal(msg string) {
	fmt.Fprintln(os.Stderr, msg)
	os.Exit(1)
}
