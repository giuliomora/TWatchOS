"""Genera pk_fonts.c: font pixel (1 bit per pixel) in formato LVGL fmt_txt.

Uso (dalla cartella del progetto):  python tools/fontgen/gen.py .
I disegni dei caratteri sono in glyphs.py ('#' = pixel acceso).
"""
import sys
from glyphs import SEG, TEXT, SMALL, SPRITE

# name, glyphs, scale, rows above baseline, line rows, descent rows,
# space cols, monospace digits?, fallback, comment
FONTS = [
    ("pk_font_small", SMALL, 2, 5, 7, 1, 2, False, "lv_font_montserrat_14",
     "maiuscoletto 3x5, pixel da 2 (etichette piccole)"),
    ("pk_font_text", TEXT, 2, 7, 10, 2, 3, False, "lv_font_montserrat_16",
     "testo 5x7, pixel da 2 (testo normale)"),
    ("pk_font_big", TEXT, 3, 7, 10, 2, 3, False, "lv_font_montserrat_24",
     "testo 5x7, pixel da 3 (testo grande)"),
    ("pk_digits_s", SEG, 3, 9, 10, 1, 5, True, "pk_font_text",
     "cifre LCD 4x9, blocchi da 3"),
    ("pk_digits_m", SEG, 5, 9, 10, 1, 5, True, "pk_font_big",
     "cifre LCD 4x9, blocchi da 5 (cronometro)"),
    ("pk_digits_l", SEG, 8, 9, 10, 1, 5, True, "pk_font_big",
     "cifre LCD 4x9, blocchi da 8 (quadrante)"),
    ("pk_sprite", SPRITE, 2, 21, 21, 0, 1, False, "pk_font_text",
     "segnaposto del quadrante (gufetto 33x21), pixel da 2"),
]


def glyph_box(rows):
    """Colonne e righe usate: (c0, c1, r0, r1) oppure None se vuoto."""
    cells = [(r, c) for r, line in enumerate(rows) for c, ch in enumerate(line) if ch == '#']
    if not cells:
        return None
    rs = [r for r, _ in cells]
    cs = [c for _, c in cells]
    return min(cs), max(cs), min(rs), max(rs)


def build(name, glyphs, s, above, line_rows, desc_rows, space_cols, mono, fallback, comment):
    codes = sorted(ord(c) for c in glyphs)
    bitmap = []
    dscs = []  # (index, adv16, w, h, ox, oy)
    widths = {}
    for code in codes:
        rows = glyphs[chr(code)]
        box = glyph_box(rows)
        if box is None:
            adv = space_cols * s
            dscs.append((len(bitmap), adv * 16, 0, 0, 0, 0))
            widths[chr(code)] = adv
            continue
        c0, c1, r0, r1 = box
        if mono:
            # cifre e spazio larghe uguali: il tempo non "balla"
            c0 = 0
            cols = max(len(r) for r in rows)
            c1 = max(c1, cols - 1)
        w = (c1 - c0 + 1)
        h = (r1 - r0 + 1)
        bits = []
        for r in range(r0, r1 + 1):
            line = rows[r] if r < len(rows) else ""
            rowbits = []
            for c in range(c0, c1 + 1):
                on = c < len(line) and line[c] == '#'
                rowbits += [1 if on else 0] * s
            for _ in range(s):
                bits += rowbits
        while len(bits) % 8:
            bits.append(0)
        start = len(bitmap)
        for i in range(0, len(bits), 8):
            v = 0
            for b in bits[i:i + 8]:
                v = (v << 1) | b
            bitmap.append(v)
        adv = (w + 1) * s
        oy = (above - 1 - r1) * s
        dscs.append((start, adv * 16, w * s, h * s, 0, oy))
        widths[chr(code)] = adv

    # intervalli di codici consecutivi -> una cmap ciascuno
    ranges = []
    for i, code in enumerate(codes):
        if ranges and code == ranges[-1][0] + ranges[-1][1]:
            ranges[-1][1] += 1
        else:
            ranges.append([code, 1, i + 1])

    out = []
    out.append(f"/* {name}: {comment} */")
    out.append(f"static const uint8_t {name}_bitmap[] = {{")
    for i in range(0, len(bitmap), 16):
        out.append("    " + ", ".join(f"0x{b:02x}" for b in bitmap[i:i + 16]) + ",")
    if not bitmap:
        out.append("    0x00,")
    out.append("};")
    out.append(f"static const lv_font_fmt_txt_glyph_dsc_t {name}_glyphs[] = {{")
    out.append("    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},")
    for code, d in zip(codes, dscs):
        ch = chr(code)
        label = "space" if ch == ' ' else ("backslash" if ch == '\\' else ch)
        out.append(f"    {{.bitmap_index = {d[0]}, .adv_w = {d[1]}, .box_w = {d[2]}, .box_h = {d[3]}, "
                   f".ofs_x = {d[4]}, .ofs_y = {d[5]}}}, /* {label} */")
    out.append("};")
    out.append(f"static const lv_font_fmt_txt_cmap_t {name}_cmaps[] = {{")
    for start, length, gid in ranges:
        out.append(f"    {{.range_start = {start}, .range_length = {length}, .glyph_id_start = {gid}, "
                   f".unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, "
                   f".type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY}},")
    out.append("};")
    out.append(f"static const lv_font_fmt_txt_dsc_t {name}_dsc = {{")
    out.append(f"    .glyph_bitmap = {name}_bitmap,")
    out.append(f"    .glyph_dsc = {name}_glyphs,")
    out.append(f"    .cmaps = {name}_cmaps,")
    out.append("    .kern_dsc = NULL,")
    out.append("    .kern_scale = 0,")
    out.append(f"    .cmap_num = {len(ranges)},")
    out.append("    .bpp = 1,")
    out.append("    .kern_classes = 0,")
    out.append("    .bitmap_format = 0,")
    out.append("};")
    out.append(f"const lv_font_t {name} = {{")
    out.append("    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,")
    out.append("    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,")
    out.append(f"    .line_height = {line_rows * s},")
    out.append(f"    .base_line = {desc_rows * s},")
    out.append("    .subpx = LV_FONT_SUBPX_NONE,")
    out.append("    .underline_position = -1,")
    out.append("    .underline_thickness = 1,")
    out.append(f"    .dsc = &{name}_dsc,")
    out.append(f"    .fallback = &{fallback},  /* simboli (batteria, play...) e caratteri mancanti */")
    out.append("    .user_data = NULL,")
    out.append("};")
    return "\n".join(out), len(bitmap), widths


HEADER = """/* =============================================================
 *  pk_fonts.c - Font pixel del tema "Pokekron" (vedi theme.h)
 *
 *  FILE GENERATO da tools/fontgen/gen.py: non modificarlo a mano.
 *  Per cambiare un carattere modifica tools/fontgen/glyphs.py
 *  e rigenera il file.
 *  I disegni dei caratteri sono definiti come pixel art e salvati
 *  a 1 bit per pixel (in tutto pochi KB, niente immagini).
 *  Simboli e caratteri mancanti vengono presi dal font "fallback".
 * ============================================================= */
#include <lvgl.h>

/* I font Montserrat usati come riserva (gia' attivi nel progetto) */
LV_FONT_DECLARE(lv_font_montserrat_14)
LV_FONT_DECLARE(lv_font_montserrat_16)
LV_FONT_DECLARE(lv_font_montserrat_24)

/* Dichiarati prima: le cifre li usano come riserva */
extern const lv_font_t pk_font_text;
extern const lv_font_t pk_font_big;
"""

WIDTHS = {}

if __name__ == "__main__":
    outdir = sys.argv[1] if len(sys.argv) > 1 else "."
    parts = [HEADER]
    total = 0
    for f in FONTS:
        code, size, widths = build(*f)
        WIDTHS[f[0]] = widths
        parts.append(code)
        total += size
        print(f"{f[0]}: {size} byte di bitmap")
    print(f"Totale bitmap: {total} byte")
    with open(f"{outdir}/pk_fonts.c", "w", newline="\n") as fh:
        fh.write("\n\n".join(parts) + "\n")
