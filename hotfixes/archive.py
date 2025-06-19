#!/usr/bin/env python3
# ruff: noqa: D102, D103
import argparse
import io
import json
import re
import struct
import tarfile
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path

RE_ARCHIVE_EVENT = re.compile(r"_-(?!(_\d\d){3})_(.+?)\.json")
RE_ARCHIVE_TIME_ONLY = re.compile(r"(\d{4}(_\d\d){2}(_-(_\d\d){3})?).json")

name_overrides: dict[str, str] = {}


def get_friendly_name(path: Path) -> str:
    if path.name in name_overrides:
        return name_overrides[path.name]

    match = RE_ARCHIVE_EVENT.search(path.name)
    if match:
        event = match.group(2).replace("_", " ")
        return " ".join(x.title() for x in event.split())

    match = RE_ARCHIVE_TIME_ONLY.search(path.name)
    if match:
        if match.group(3) is None:
            return match.group(1).replace("_", "-")
        time = datetime.strptime(match.group(1), r"%Y_%m_%d_-_%H_%M_%S")  # noqa: DTZ007
        date_str = time.strftime("%Y_%m_%d")
        pm_str = time.strftime("%p").lower()
        return f"{date_str} {time.hour % 12}{pm_str}"

    return path.stem


@dataclass
class HotfixInfo:
    path: Path
    friendly_name: str = field(init=False)

    def __post_init__(self) -> None:
        self.friendly_name = get_friendly_name(self.path)

    def compress(self) -> io.BytesIO:
        binary = io.BytesIO()

        with self.path.open() as file:
            params = json.load(file)["parameters"]

            binary.write(struct.pack("<I", len(params)))

            for hf in params:
                # Explicitly saying le removes the BOM
                key_bites = hf["key"].encode("utf-16le")
                value_bites = hf["value"].encode("utf-16le")

                binary.write(
                    struct.pack("<I", len(key_bites) // 2)
                    + key_bites
                    + struct.pack("<I", len(value_bites) // 2)
                    + value_bites,
                )

        return binary


def get_ordered_mods(mod_paths: list[Path]) -> list[HotfixInfo]:
    return sorted((HotfixInfo(mod) for mod in mod_paths), key=lambda h: h.friendly_name)


def get_ordered_hotfixes(point_in_time: Path, filter_path: Path | None) -> list[HotfixInfo]:
    filtered_names: list[str] | None = None
    if filter_path is not None:
        with filter_path.open() as file:
            filtered_names = json.load(file)

    return sorted(
        (
            HotfixInfo(hf)
            for hf in point_in_time.iterdir()
            if hf.is_file()
            and hf.suffix == ".json"
            and (filtered_names is None or hf.name in filtered_names)
        ),
        reverse=True,
        key=lambda h: h.path,
    )


if __name__ == "__main__":

    def _existing_dir_parser(arg: str) -> Path:
        path = Path(arg)
        if path.is_dir():
            return path
        raise argparse.ArgumentTypeError(f"'{arg}' is not a directory")

    def _existing_file_parser(arg: str) -> Path:
        path = Path(arg)
        if path.is_file():
            return path
        raise argparse.ArgumentTypeError(f"'{arg}' is not a file")

    parser = argparse.ArgumentParser(
        description="Combines hotfixes from one of the archive repos into our archive format.",
    )
    parser.add_argument("output", type=Path, help="The location to put the combined archive file.")
    parser.add_argument(
        "point_in_time",
        type=_existing_dir_parser,
        help="The hotfix archive repo's point-in-time folder.",
    )
    parser.add_argument(
        "-n",
        "--names",
        type=_existing_file_parser,
        help="A json file mapping file names to friendly names to store instead.",
    )
    parser.add_argument(
        "-f",
        "--filter",
        type=_existing_file_parser,
        help="A json file holding a whitelist of filenames from the point-in-time folder.",
    )
    parser.add_argument(
        "-m",
        "--mod",
        type=_existing_file_parser,
        action="append",
        default=[],
        help="A modded hotfix file to include. May be specified multiple times.",
    )

    args = parser.parse_args()

    if args.names is not None:
        with args.names.open() as file:
            name_overrides = json.load(file)

    mod_hotfixes = get_ordered_mods(args.mod)
    vanilla_hotfixes = get_ordered_hotfixes(args.point_in_time, args.filter)
    all_hotfixes = mod_hotfixes + vanilla_hotfixes

    with tarfile.open(args.output, "w:gz") as tar:
        for idx, hf in enumerate(all_hotfixes):
            data = hf.compress()

            info = tar.gettarinfo(hf.path, arcname=f"{idx:03};{hf.friendly_name}")
            info.size = data.tell()
            data.seek(0)
            tar.addfile(info, data)

            data.close()
