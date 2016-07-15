/* Copyright (c) huxingyi@msn.com All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
 
#ifndef IMAGE_OPS_H
#define IMAGE_OPS_H

typedef struct imageOpsImage imageOpsImage;

int imageOpsInit(void);
void imageOpsUninit(void);
imageOpsImage *imageOpsOpen(const char *filename);
int imageOpsRotate(imageOpsImage *img, int degrees);
int imageOpsAddBorder(imageOpsImage *img);
imageOpsImage *imageOpsCreate(int width, int height);
int imageOpsSave(imageOpsImage *img, const char *filename);
int imageOpsTrim(imageOpsImage *img, int *cropLeft, int *cropTop);
void imageOpsDestroy(imageOpsImage *img);
int imageOpsGetWidth(imageOpsImage *img);
int imageOpsGetHeight(imageOpsImage *img);
int imageOpsComposite(imageOpsImage *dest, imageOpsImage *src,
  int left, int top);

#endif
