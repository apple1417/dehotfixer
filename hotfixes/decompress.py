#!/usr/bin/env python3
import json
import struct
from pathlib import Path
from typing import Any


def extract_compressed_hotfix(path: Path) -> dict[str, Any]:
    """
    Extracts the raw hotfix data out of a compressed hotfix file.

    Args:
        path: The path of the compressed hotfix.
    Returns:
        The decompressed hotfix json.
    """
    params: list[dict[str, str]] = []

    with path.open("rb") as file:
        (num_hotfixes,) = struct.unpack("<I", file.read(4))
        for _ in range(num_hotfixes):
            (key_len,) = struct.unpack("<I", file.read(4))
            key = file.read(key_len * 2).decode("utf-16le")
            (value_len,) = struct.unpack("<I", file.read(4))
            value = file.read(value_len * 2).decode("utf-16le")
            params.append({"key": key, "value": value})

    return {
        "service_name": "Micropatch",
        "configuration_group": "Oak_Crossplay_Default",
        "configuration_version": "1.314.550",
        "parameters": params,
    }


if __name__ == "__main__":
    import argparse

    def _existing_file_parser(arg: str) -> Path:
        path = Path(arg)
        if path.is_file():
            return path
        raise argparse.ArgumentTypeError(f"'{arg}' is not a file")

    parser = argparse.ArgumentParser(description="Decompresses a single compressed hotfix file.")
    parser.add_argument(
        "compressed",
        type=_existing_file_parser,
        help="The compressed hotfix file to decompress.",
    )
    parser.add_argument("output", type=Path, help="The path to write the decompressed file to.")

    args = parser.parse_args()
    with args.output.open("w") as file:
        json.dump(extract_compressed_hotfix(args.compressed), file, indent=2)
