#!/usr/bin/env python
# -*- coding:utf-8 -*-
import os
import sys
import glob
from PIL import Image

def Usage():
    print('%s src_path dst_path width height' % (sys.argv[0]))
    sys.exit(1)

def crop(dst_path, src_img, w, h, first_n):
    im = Image.open(src_img)
    im_w, im_h = im.size
    print('Splitting image "%s" (width: %d height: %d)' % (src_img, im_w, im_h))
    w_num, h_num = int(im_w/w), int(im_h/h)

    n = first_n
    for wi in range(0, w_num):
        for hi in range(0, h_num):
            box = (wi*w, hi*h, (wi+1)*w, (hi+1)*h)
            piece = im.crop(box)
            tmp_img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
            tmp_img.paste(piece)
            img_path = os.path.join(dst_path, "%d.png" % (n))
            tmp_img.save(img_path, 'PNG')
            n += 1
    return n


if __name__ == "__main__":
    if len(sys.argv) != 5:
        Usage()

    src_path = sys.argv[1]
    dst_path = sys.argv[2]
    w, h = int(sys.argv[3]), int(sys.argv[4])
    i = 0
    for filepath in glob.iglob(os.path.join(src_path, '*.png')):
        i = crop(dst_path, filepath, w, h, i)
    print("%d images created." % (i))