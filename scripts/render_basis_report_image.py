#!/usr/bin/env python3

import json
import pathlib
import struct
import sys
import zlib


FONT = {
    " ": ["000", "000", "000", "000", "000"],
    "-": ["000", "000", "111", "000", "000"],
    ".": ["000", "000", "000", "000", "010"],
    "/": ["001", "001", "010", "100", "100"],
    ":": ["000", "010", "000", "010", "000"],
    "%": ["101", "001", "010", "100", "101"],
    "0": ["111", "101", "101", "101", "111"],
    "1": ["010", "110", "010", "010", "111"],
    "2": ["111", "001", "111", "100", "111"],
    "3": ["111", "001", "111", "001", "111"],
    "4": ["101", "101", "111", "001", "001"],
    "5": ["111", "100", "111", "001", "111"],
    "6": ["111", "100", "111", "101", "111"],
    "7": ["111", "001", "001", "001", "001"],
    "8": ["111", "101", "111", "101", "111"],
    "9": ["111", "101", "111", "001", "111"],
    "A": ["111", "101", "111", "101", "101"],
    "B": ["110", "101", "110", "101", "110"],
    "C": ["111", "100", "100", "100", "111"],
    "D": ["110", "101", "101", "101", "110"],
    "E": ["111", "100", "110", "100", "111"],
    "F": ["111", "100", "110", "100", "100"],
    "G": ["111", "100", "101", "101", "111"],
    "H": ["101", "101", "111", "101", "101"],
    "I": ["111", "010", "010", "010", "111"],
    "J": ["001", "001", "001", "101", "111"],
    "K": ["101", "101", "110", "101", "101"],
    "L": ["100", "100", "100", "100", "111"],
    "M": ["101", "111", "111", "101", "101"],
    "N": ["101", "111", "111", "111", "101"],
    "O": ["111", "101", "101", "101", "111"],
    "P": ["111", "101", "111", "100", "100"],
    "Q": ["111", "101", "101", "111", "001"],
    "R": ["111", "101", "111", "110", "101"],
    "S": ["111", "100", "111", "001", "111"],
    "T": ["111", "010", "010", "010", "010"],
    "U": ["101", "101", "101", "101", "111"],
    "V": ["101", "101", "101", "101", "010"],
    "W": ["101", "101", "111", "111", "101"],
    "X": ["101", "101", "010", "101", "101"],
    "Y": ["101", "101", "010", "010", "010"],
    "Z": ["111", "001", "010", "100", "111"],
}

BG = (255, 255, 255, 255)
TEXT = (35, 47, 62, 255)
SUBTEXT = (98, 110, 123, 255)
HEADER_BG = (44, 91, 140, 255)
HEADER_TEXT = (255, 255, 255, 255)
GROUP_BG = (226, 238, 248, 255)
ALT_ROW = (245, 248, 252, 255)
WARNING = (196, 43, 28, 255)
BORDER = (214, 223, 232, 255)


def make_canvas(width: int, height: int):
    return [bytearray(BG * width) for _ in range(height)]


def set_pixel(pixels, x, y, color):
    if x < 0 or y < 0 or y >= len(pixels) or x >= len(pixels[0]) // 4:
        return
    offset = x * 4
    pixels[y][offset : offset + 4] = bytes(color)


def fill_rect(pixels, x, y, width, height, color):
    for yy in range(max(0, y), min(len(pixels), y + height)):
        row = pixels[yy]
        start = max(0, x) * 4
        end = min(len(row) // 4, x + width) * 4
        row[start:end] = bytes(color) * ((end - start) // 4)


def draw_hline(pixels, x, y, width, color):
    fill_rect(pixels, x, y, width, 1, color)


def glyph_for(char):
    return FONT.get(char.upper(), FONT[" "])


def text_width(text, scale):
    if not text:
        return 0
    return len(text) * ((3 * scale) + scale) - scale


def draw_text(pixels, x, y, text, color, scale=2, align="left", box_width=None):
    if box_width is not None:
        width = text_width(text, scale)
        if align == "right":
            x = x + box_width - width
        elif align == "center":
            x = x + (box_width - width) // 2

    cursor_x = x
    for char in text:
        glyph = glyph_for(char)
        for row_index, row in enumerate(glyph):
            for col_index, bit in enumerate(row):
                if bit != "1":
                    continue
                for yy in range(scale):
                    for xx in range(scale):
                        set_pixel(
                            pixels,
                            cursor_x + col_index * scale + xx,
                            y + row_index * scale + yy,
                            color,
                        )
        cursor_x += (3 * scale) + scale


def png_chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)


def write_png(path: pathlib.Path, pixels):
    height = len(pixels)
    width = len(pixels[0]) // 4 if height else 0
    raw = b"".join(b"\x00" + bytes(row) for row in pixels)
    compressed = zlib.compress(raw, 9)
    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += png_chunk(b"IDAT", compressed)
    png += png_chunk(b"IEND", b"")
    path.write_bytes(png)


def build_row_cells(row):
    return [
        row["instrument_id"],
        row["price_text"],
        row["change_text"],
        row["change_percent_text"],
        row["basis_text"],
        row["annual_rate_text"],
        row["remaining_days_text"],
        row["warning_text"],
    ]


def main() -> int:
    if len(sys.argv) != 3:
        return 2

    input_path = pathlib.Path(sys.argv[1])
    output_path = pathlib.Path(sys.argv[2])
    document = json.loads(input_path.read_text(encoding="utf-8"))

    columns = document["columns"]
    groups = document["groups"]
    col_widths = [102, 78, 78, 78, 78, 92, 56, 92]
    margin = 18
    title_height = 22
    subtitle_height = 18
    group_header_height = 20
    table_header_height = 20
    row_height = 20
    gap = 10

    width = margin * 2 + sum(col_widths)
    height = margin + title_height + subtitle_height + gap
    for group in groups:
        height += group_header_height + table_header_height
        height += max(1, len(group["rows"])) * row_height
        height += gap
    height += margin

    pixels = make_canvas(width, height)

    y = margin
    draw_text(pixels, margin, y, document["title"], TEXT, scale=2)
    y += title_height
    draw_text(pixels, margin, y, document["subtitle"], SUBTEXT, scale=2)
    y += subtitle_height + gap

    for group in groups:
        fill_rect(pixels, margin, y, sum(col_widths), group_header_height, GROUP_BG)
        draw_text(pixels, margin + 6, y + 4, group["name"], TEXT, scale=2)
        y += group_header_height

        fill_rect(pixels, margin, y, sum(col_widths), table_header_height, HEADER_BG)
        x = margin
        for column, col_width in zip(columns, col_widths):
            draw_text(pixels, x + 4, y + 5, column, HEADER_TEXT, scale=1)
            x += col_width
        y += table_header_height

        rows = group["rows"] if group["rows"] else [{
            "instrument_id": "-",
            "price_text": "N/A",
            "change_text": "N/A",
            "change_percent_text": "N/A",
            "basis_text": "N/A",
            "annual_rate_text": "N/A",
            "remaining_days_text": "N/A",
            "warning_text": "-",
            "warning_negative": False,
        }]

        for row_index, row in enumerate(rows):
            if row_index % 2 == 1:
                fill_rect(pixels, margin, y, sum(col_widths), row_height, ALT_ROW)

            x = margin
            for cell_index, (cell, col_width) in enumerate(zip(build_row_cells(row), col_widths)):
                color = WARNING if (cell_index == 7 and row.get("warning_negative")) else TEXT
                align = "left" if cell_index == 0 else "right"
                draw_text(pixels, x + 4, y + 5, cell, color, scale=1, align=align, box_width=col_width - 8)
                x += col_width

            draw_hline(pixels, margin, y + row_height - 1, sum(col_widths), BORDER)
            y += row_height

        y += gap

    output_path.parent.mkdir(parents=True, exist_ok=True)
    write_png(output_path, pixels)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
