#!/usr/bin/env python
# -*- coding:utf-8 -*-
import os
import sys
import glob
from pathlib import Path
import json

def usage():
    print("%s src_dir tiles_in_row json_string" % (sys.argv[0]))
    print("json_string = [[prefix, x1, y1, x2, y1], ...]")
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
    if len(sys.argv) != 4:
        usage()

    src_path = sys.argv[1]
    row_size = int(sys.argv[2])
    json_string = sys.argv[3]
    regions = [{"prefix": region[0], "top_left": [region[1], region[2]], "bottom_right": [region[3], region[4]]} for region in json.loads(json_string)]
    print("Renaming images...\n")
    counters = {}
    for region in regions:
        prefix = region["prefix"]
        i = counters.get(prefix, 0)
        before = i
        for filepath in glob.iglob(os.path.join(src_path, "*.png")):
            i = rename(filepath, prefix, row_size, region["top_left"], region["bottom_right"], i)
        counters[prefix] = i
        count = i - before
        if count > 0:
            print("%d images prefixed with %s" % (count, prefix))
    deleted = 0
    for filepath in glob.iglob(os.path.join(src_path, "*.png")):
        p = Path(filepath)
        try:
            file_number = int(p.stem)
            p.unlink()
            deleted += 1
        except:
            pass
    total = 0
    print("")
    for k,v in counters.items():
        print("%d\t%s" % (v,k))
        total += v
    print("\n%d images renamed, %d images removed." % (total,deleted))
