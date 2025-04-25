from __future__ import annotations

import json

import pytest

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

    data = {
        "results": [
            {"name": "test1", "uuid": "33bb9554-c616-42e6-a9c6-88d3bba4221c"},
            {"name": "test2", "uuid": "acde070d-8c4c-4f0d-9d8a-162843c10333"},
        ]
    }
    repl = m.JsonQueryRepl(json.dumps(data), debug=True)
    with pytest.raises(RuntimeError) as excinfo:
        repl.add_params("hostname", "localhost")
    assert "JSON syntax_error" in repr(excinfo)
    repl.add_params("hostname", json.dumps("localhost"))
    ret = repl.eval("results[*].[name, uuid, $hostname]")
    assert (
        ret
        == '[["test1","33bb9554-c616-42e6-a9c6-88d3bba4221c","localhost"],["test2","acde070d-8c4c-4f0d-9d8a-162843c10333","localhost"]]'
    )


def test_msgpack():
    print()


# pytest -vs tests/test_basic.py
