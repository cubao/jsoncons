from __future__ import annotations

import json

import jsoncons as m


def test_version():
    assert m.__version__ == "0.0.1"


def test_add():
    assert m.add(1, 2) == 3


def test_sub():
    assert m.subtract(1, 2) == -1


def test_repl():
    # https://github.com/danielaparker/jsoncons/blob/master/doc/ref/jmespath/jmespath.md
    data = {
        "people": [
            {"age": 20, "other": "foo", "name": "Bob"},
            {"age": 25, "other": "bar", "name": "Fred"},
            {"age": 30, "other": "baz", "name": "George"},
        ]
    }
    repl = m.JsonQueryRepl(json.dumps(data), debug=True)
    ret = repl.eval("people[?age > `20`].[name, age]")
    assert ret == '[["Fred",25],["George",30]]'
    print(json.loads(ret))
    repl.debug = False
    assert repl.eval("people[?age > `20`].[name, age]") == ret

    data = [
        {
            "home_state": "WA",
            "states": [
                {"name": "WA", "cities": ["Seattle", "Bellevue", "Olympia"]},
                {"name": "CA", "cities": ["Los Angeles", "San Francisco"]},
                {"name": "NY", "cities": ["New York City", "Albany"]},
            ],
        },
        {
            "home_state": "NY",
            "states": [
                {"name": "WA", "cities": ["Seattle", "Bellevue", "Olympia"]},
                {"name": "CA", "cities": ["Los Angeles", "San Francisco"]},
                {"name": "NY", "cities": ["New York City", "Albany"]},
            ],
        },
    ]
    repl = m.JsonQueryRepl(json.dumps(data), debug=True)
    ret = repl.eval(
        r"[*].[let $home_state = home_state in states[? name == $home_state].cities[]][]"
    )
    assert ret == '[["Seattle","Bellevue","Olympia"],["New York City","Albany"]]'


# pytest -vs tests/test_basic.py
