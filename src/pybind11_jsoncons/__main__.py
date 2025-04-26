from __future__ import annotations

import os
import sys

from . import msgpack_decode, msgpack_encode


def __msgpack_decode():
    data = sys.stdin.read()
    decoded = msgpack_decode(data)
    sys.stdout.write(decoded)


def __msgpack_encode():
    data = sys.stdin.read()
    encoded = msgpack_encode(data)
    # print(type(encoded), len(encoded))
    sys.stdout.buffer.write(encoded)


if __name__ == "__main__":
    os.umask(0)
    import fire

    fire.core.Display = lambda lines, out: print(*lines, file=out)
    fire.Fire(
        {
            "msgpack_decode": __msgpack_decode,
            "msgpack_encode": __msgpack_encode,
        }
    )
