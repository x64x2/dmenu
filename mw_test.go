package meow

import (
	"strings"
	"testing"
)

var body = `
{
    "general1": {
        "meow-score": 10,
        "key-test": "branch general1",
        "key-test1": "branch1 general1",
		"g1": {
			"a1": "A1"
		},
		"g2": ["a1", "b1"]
    },
    "general2": {
        "meow-score": 9,
        "key-test": "branch general2",
		"key-test1": "branch1 general2"
    },
    "general3": {
        "meow-score": 11,
        "key-test": "branch general3",
		"key-test1": "branch1 general3"
    },
    "chess": {
        "key-test": "branch chess",
        "slice-string": [
            "a",
            "b"
        ],
        "map-string": {
            "a": "A",
            "b": "B"
        },
        "slice-int": [
            1,
            2
        ],
        "map-int": {
            "a": "1",
            "b": "2"
        },
        "string": "S",
        "int": 1,
		"g1": {
			"a1": "A"
		},
		"g2": ["a", "b"]
    }
}

`

func TestMeow(t *testing.T) {

	reader := strings.NewReader(body)
	meow, err := LoadReader("chess", reader)

	if err != nil {
		t.Errorf("Error: %s", err)
	}

	found := meow.ValueString("string")
	if meow.ValueString("string") != "S" {
		t.Errorf("Error: found `%s`\n\n%v", found, meow.meow)
	}
	found = meow.ValueString("key-test")
	if found != "branch chess" {
		t.Errorf("Error: found `%s`\n\n%v", found, meow.meow)
	}
	found = meow.ValueString("key-test1")
	if found != "branch1 general3" {
		t.Errorf("Error: found `%s`\n\n%v", found, meow.meow)
	}

	ifound := meow.ValueInt("int")
	if ifound != int(1) {
		t.Errorf("Error: found `%v`\n\n%v", ifound, meow.meow)
	}

	mfound := meow.ValueStringMap("g1")
	if mfound["a1"] != "A" {
		t.Errorf("Error: found `%v`\n\n%v", mfound["a1"], meow.meow)
	}

	sfound := meow.ValueStringSlice("g2")
	if sfound[1] != "b" {
		t.Errorf("Error: found `%v`\n\n%v", sfound[1], meow.meow)
	}

	found = meow.ValueString("g1", "a1")
	if found != "A" {
		t.Errorf("Error: found `%s`\n\n%v", found, meow.meow)
	}

}
