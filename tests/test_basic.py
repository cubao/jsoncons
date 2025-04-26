from __future__ import annotations

import json

import pytest

import pybind11_jsoncons as m


def test_version():
    assert m.__version__ == "0.1.0"


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
    # https://msgpack.org/index.html
    data = m.msgpack_encode('{"compact":"true",         "schema":0}')
    assert isinstance(data, bytes)
    print("msgpack #bytes", len(data))
    data = m.msgpack_decode(data)
    assert isinstance(data, str)
    assert data == '{"compact":"true","schema":0}'


def test_json_query():
    """
    https://jmespath.org/tutorial.html
    """
    people = [
        {"age": 5, "other": "too young", "name": "Baby"},
        {"age": 20, "other": "foo", "name": "Bob"},
        {"age": 25, "other": "bar", "name": "Fred"},
        {"age": 30, "other": "baz", "name": "George"},
    ]

    repl = m.JsonQueryRepl(json.dumps(people[0]), debug=True)
    assert json.loads(repl.eval("age")) == 5
    assert json.loads(repl.eval("name")) == "Baby"
    assert not json.loads(repl.eval("age >= `18`"))

    jql = m.JsonQuery()
    with pytest.raises(RuntimeError) as excinfo:
        jql.setup_predicate("[*].[")
    assert "Syntax error" in repr(excinfo)

    jql.setup_predicate("age >= `18`")
    jql.setup_transforms(["name", "age"])
    for p in people:
        print(p, jql.process(m.msgpack_encode(json.dumps(p))))
    export = jql.export()
    data = m.msgpack_decode(export)
    assert json.loads(data) == [["Bob", 20], ["Fred", 25], ["George", 30]]


def test_json_type():
    obj = m.Json().from_json('{"compact":"true",         "schema":0}')
    assert obj.to_json() == '{"compact":"true","schema":0}'
    obj2 = m.Json().from_msgpack(obj.to_msgpack())
    assert obj.to_msgpack() == obj2.to_msgpack()


def test_json_query_json():
    people = [
        {"age": 5, "other": "too young", "name": "Baby"},
        {"age": 20, "other": "foo", "name": "Bob"},
        {"age": 25, "other": "bar", "name": "Fred"},
        {"age": 30, "other": "baz", "name": "George"},
    ]

    jql = m.JsonQuery()
    jql.setup_predicate("age >= `18`")
    jql.setup_transforms(["name", "age"])
    for p in people:
        j = m.Json().from_json(json.dumps(p))
        print(p, jql.process_json(j))
    export = jql.export_json()
    assert json.loads(export.to_json()) == [["Bob", 20], ["Fred", 25], ["George", 30]]


# pytest -vs tests/test_basic.py
