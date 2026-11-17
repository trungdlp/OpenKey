"""Compile the actual native sender bodies against OS mocks without posting input."""
from pathlib import Path
import sys


def extract(path, name):
    source = Path(path).read_text(encoding="utf-8-sig")
    start = source.index("void SendNewCharString(")
    body_start = source.index("{", start)
    depth = 1
    end = body_start + 1
    while depth:
        if end >= len(source):
            raise ValueError("Unterminated sender function")
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    result = source[start:end].replace("SendNewCharString", name, 1)
    assert result.count(name) == 1
    return result


if __name__ == "__main__":
    mac, windows, output = sys.argv[1:]
    Path(output).write_text(extract(mac, "SendMacString") + "\n" + extract(windows, "SendWindowsString") + "\n")
