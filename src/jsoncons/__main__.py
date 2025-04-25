import os
from . import add, subtract

if __name__ == "__main__":
    os.umask(0)
    import fire
    fire.core.Display = lambda lines, out: print(*lines, file=out)
    fire.Fire({
        "add": add,
        "subtract": subtract,
    })