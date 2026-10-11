#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build tab/touch icons from the existing Sloop sail mark (requires Pillow)."""
from pathlib import Path
import math
import xml.etree.ElementTree as ET
from PIL import Image, ImageDraw

WEB = Path(__file__).resolve().parents[1] / 'web'
NS = 'http://www.w3.org/2000/svg'
ET.register_namespace('', NS)
logo = ET.parse(WEB / 'sloop-smk-logo.svg').getroot()
icon = ET.Element(f'{{{NS}}}svg', {'viewBox': '0 0 260 260'})
ET.SubElement(icon, f'{{{NS}}}rect', {
    'width': '260', 'height': '260', 'rx': '44', 'fill': '#17191c'})
mark = list(logo)[:7]  # Ring, mast, four color bands, hull; omit the wordmark.
for element in mark:
    if 'stroke-width' in element.attrib:
        element.set('stroke-width', '14')
    icon.append(element)
(WEB / 'favicon.svg').write_text(ET.tostring(icon, encoding='unicode') + '\n', encoding='utf-8')

# Supersample the same simple geometry for browsers/bookmarks needing PNG/ICO.
scale = 4
image = Image.new('RGBA', (260 * scale, 260 * scale))
draw = ImageDraw.Draw(image)
box = lambda coords: tuple(round(c * scale) for c in coords)
draw.rounded_rectangle(box((0, 0, 260, 260)), radius=44 * scale, fill='#17191c')
for element in mark:
    tag = element.tag.rsplit('}', 1)[-1]
    a = element.attrib
    if tag == 'path':
        assert a['d'] == 'M59.29 200.71 A100.00 100.00 0 1 1 200.71 200.71'
        width = float(a['stroke-width'])
        points = [((130 + 100 * math.cos(math.radians(deg))) * scale,
                   (130 + 100 * math.sin(math.radians(deg))) * scale)
                  for deg in range(135, 406)]
        draw.arc(box((30-width/2, 30-width/2, 230+width/2, 230+width/2)),
                 135, 405, fill=a['stroke'], width=round(width * scale))
        for x, y in (points[0], points[-1]):
            r = width * scale / 2
            draw.ellipse((x-r, y-r, x+r, y+r), fill=a['stroke'])
    elif tag == 'rect':
        x, y, w, h = (float(a[k]) for k in ('x', 'y', 'width', 'height'))
        draw.rounded_rectangle(box((x, y, x+w, y+h)), radius=float(a['rx']) * scale, fill=a['fill'])
    elif tag == 'polygon':
        draw.polygon([box(tuple(map(float, point.split(',')))) for point in a['points'].split()], fill=a['fill'])
    else:
        raise ValueError('Unexpected sail primitive: ' + tag)
for name, size in [('favicon-32.png', 32), ('apple-touch-icon.png', 180)]:
    image.resize((size, size), Image.Resampling.LANCZOS).save(WEB / name)
image.resize((64, 64), Image.Resampling.LANCZOS).save(
    WEB / 'favicon.ico', sizes=[(16, 16), (32, 32), (48, 48), (64, 64)])
print('Generated Sloop sail SVG, ICO, 32px PNG and 180px touch icon')
