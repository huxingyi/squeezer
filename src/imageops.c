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

#include "imageops.h"
#include "lodepng.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct imageOpsImage{
  unsigned char *imageData;
  unsigned int width;
  unsigned int height;
};

int imageOpsInit(void) {
  return 0;
}

void imageOpsUninit(void) {
  // no-op
}

imageOpsImage *imageOpsOpen(const char *filename) {
  unsigned int err;
  imageOpsImage *img = (imageOpsImage *)calloc(1, sizeof(imageOpsImage));
  if (!img) {
    fprintf(stderr, "%s: calloc failed\n", __FUNCTION__);
    return 0;
  }
  err = lodepng_decode32_file(&img->imageData, &img->width, &img->height,
    filename);
  if (err) {
    fprintf(stderr, "%s: lodepng_decode32_file error: %s\n", __FUNCTION__,
      lodepng_error_text(err));
    free(img);
    return 0;
  }
  return img;
}

void imageOpsDestroy(imageOpsImage *img) {
  if (img->imageData) {
    free(img->imageData);
    img->imageData = 0;
  }
  free(img);
}

int imageOpsRotate(imageOpsImage *img, int degrees) {
  imageOpsImage *newImg;
  int x;
  int y;
  assert(90 == degrees);
  newImg = imageOpsCreate(img->height, img->width);
  if (!newImg) {
    fprintf(stderr, "%s: imageOpsCreate failed\n", __FUNCTION__);
    return -1;
  }
  for (x = 0; x < img->width; ++x) {
    for (y = 0; y < img->height; ++y) {
      int destX = y;
      int destY = x;
      memcpy(&newImg->imageData[(destY * newImg->width + destX) * 4],
        &img->imageData[(y * img->width + x) * 4], 4);
    }
  }
  free(img->imageData);
  img->imageData = newImg->imageData;
  img->width = newImg->width;
  img->height = newImg->height;
  free(newImg);
  return 0;
}

#define setBorderPixel()           \
  img->imageData[offset++] = 0xff; \
  img->imageData[offset++] = 0;    \
  img->imageData[offset++] = 0;    \
  img->imageData[offset++] = 0xff

int imageOpsAddBorder(imageOpsImage *img) {
  int x;
  int y;
  int offset = 0;
  for (x = 0, y = 0, offset = (x + y * img->width) * 4; x < img->width; ++x) {
    setBorderPixel();
  }
  for (x = 0, y = img->height - 1, offset = (x + y * img->width) * 4;
      x < img->width; ++x) {
    setBorderPixel();
  }
  for (y = 0, x = 0; y < img->height; ++y) {
    offset = (x + y * img->width) * 4;
    setBorderPixel();
  }
  for (y = 0, x = img->width - 1; y < img->height; ++y) {
    offset = (x + y * img->width) * 4;
    setBorderPixel();
  }
  return 0;
}

imageOpsImage *imageOpsCreate(int width, int height) {
  imageOpsImage *img = (imageOpsImage *)calloc(1, sizeof(imageOpsImage));
  if (!img) {
    fprintf(stderr, "%s: calloc failed\n", __FUNCTION__);
    return 0;
  }
  img->imageData = (unsigned char *)calloc(width * height, 4);
  if (!img->imageData) {
    fprintf(stderr, "%s: calloc failed\n", __FUNCTION__);
    free(img);
    return 0;
  }
  img->width = width;
  img->height = height;
  return img;
}

int imageOpsSave(imageOpsImage *img, const char *filename) {
  unsigned int err = lodepng_encode32_file(filename, img->imageData,
    img->width, img->height);
  if (err) {
    fprintf(stderr, "%s: lodepng_encode32_file error: %s\n", __FUNCTION__,
      lodepng_error_text(err));
    return -1;
  }
  return 0;
}

static void copyImageData(unsigned char *destImageData, int destLeft,
    int destTop, int destWidth, unsigned char *srcImageData, int srcLeft,
    int srcTop, int srcWidth, int copyWidth, int copyHeight) {
  int xSrc = srcLeft;
  int ySrc = srcTop;
  int shiftOnce = copyWidth * 4;
  for (ySrc = srcTop; ySrc < srcTop + copyHeight; ++ySrc) {
    int xDest = destLeft;
    int yDest = destTop + (ySrc - srcTop);
    memcpy(destImageData + (yDest * destWidth + xDest) * 4,
      srcImageData + (ySrc * srcWidth + xSrc) * 4, shiftOnce);
  }
}

int imageOpsTrim(imageOpsImage *img, int *cropLeft, int *cropTop) {
  int trimLeft = 0;
  int trimTop = 0;
  int trimRight = img->width - 1;
  int trimBottom = img->height - 1;
  int offset = 0;
  int x;
  int y;
  for (y = 0; y < img->height; ++y) {
    int isEmpty = 1;
    for (x = 0; x < img->width; ++x) {
      offset = (x + y * img->width) * 4;
      if (0 != img->imageData[offset + 3]) {
        isEmpty = 0;
        break;
      }
    }
    if (!isEmpty) {
      trimTop = y;
      break;
    }
  }
  for (y = img->height - 1; y >= 0; --y) {
    int isEmpty = 1;
    for (x = 0; x < img->width; ++x) {
      offset = (x + y * img->width) * 4;
      if (0 != img->imageData[offset + 3]) {
        isEmpty = 0;
        break;
      }
    }
    if (!isEmpty) {
      trimBottom = y;
      break;
    }
  }
  for (x = 0; x < img->width; ++x) {
    int isEmpty = 1;
    for (y = 0; y < img->height; ++y) {
      offset = (x + y * img->width) * 4;
      if (0 != img->imageData[offset + 3]) {
        isEmpty = 0;
        break;
      }
    }
    if (!isEmpty) {
      trimLeft = x;
      break;
    }
  }
  for (x = img->width - 1; x >= 0; --x) {
    int isEmpty = 1;
    for (y = 0; y < img->height; ++y) {
      offset = (x + y * img->width) * 4;
      if (0 != img->imageData[offset + 3]) {
        isEmpty = 0;
        break;
      }
    }
    if (!isEmpty) {
      trimRight = x;
      break;
    }
  }
  if (0 != trimLeft ||
      trimRight != img->width - 1 ||
      trimTop != 0 ||
      trimBottom != img->height - 1) {
    int newWidth = trimRight - trimLeft + 1;
    int newHeight = trimBottom - trimTop + 1;
    unsigned char *newImageData = (unsigned char *)calloc(newWidth * newHeight,
      4);
    if (!newImageData) {
      return -1;
    }
    copyImageData(newImageData, 0, 0, newWidth, img->imageData, trimLeft,
        trimTop, img->width, newWidth, newHeight);
    free(img->imageData);
    img->imageData = newImageData;
    img->width = newWidth;
    img->height = newHeight;
    if (cropLeft) {
      *cropLeft = trimLeft;
    }
    if (cropTop) {
      *cropTop = trimTop;
    }
  } else {
    if (cropLeft) {
      *cropLeft = 0;
    }
    if (cropTop) {
      *cropTop = 0;
    }
  }
  return 0;
}

int imageOpsGetWidth(imageOpsImage *img) {
  return img->width;
}

int imageOpsGetHeight(imageOpsImage *img) {
  return img->height;
}

int imageOpsComposite(imageOpsImage *dest, imageOpsImage *src,
    int left, int top) {
  copyImageData(dest->imageData, left, top, dest->width, src->imageData, 0, 0,
    src->width, src->width, src->height);
  return 0;
}
