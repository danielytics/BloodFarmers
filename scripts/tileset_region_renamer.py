#!/usr/bin/env python
# -*- coding:utf-8 -*-
import os
import sys
import glob
from pathlib import Path

def usage():
    print('%s src_dir prefix tiles_in_row x1 y1 x2 y2' % (sys.argv[0]))
    sys.exit(1)

def rename(filepath, prefix, row_size, top_left, bottom_right, n):
    p = Path(filepath)
    file_number = None
    try:
        file_number = int(p.stem)
    except:
        return n
    y = int(file_number / row_size)
    x = int(file_number - (y * row_size))
    if x >= top_left[0] and x <= bottom_right[0] and y >= top_left[1] and y <= bottom_right[1]:
        p.rename(Path(p.parent, "{}-{}".format(prefix, n) + p.suffix))
        n += 1
    return n

if __name__ == "__main__":
    if len(sys.argv) != 8:
        usage()

    src_path = sys.argv[1]
    prefix = sys.argv[2]
    row_size = int(sys.argv[3])
    top_left, bottom_right = (int(sys.argv[4]), int(sys.argv[5]),), (int(sys.argv[6]), int(sys.argv[7]),)
    i = 0
    for filepath in glob.iglob(os.path.join(src_path, '*.png')):
        i = rename(filepath, prefix, row_size, top_left, bottom_right, i)
    print("%d images renamed." % (i))
