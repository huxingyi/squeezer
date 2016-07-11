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

#include <wand/magick_wand.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "squeezer.h"
#include "maxrects.h"

typedef struct fileItem {
  struct fileItem *next;
  char filename[780];
  int width;
  int height;
} fileItem;

static void freeFileList(fileItem *list) {
  while (list) {
    fileItem *willDel = list;
    list = list->next;
    free(willDel);
  }
}

static MagickWand *createSpecificWand(const char *filename) {
  MagickWand *mw = NewMagickWand();
  if (!mw) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "NewMagickWand failed");
    return 0;
  }
  if (MagickTrue != MagickReadImage(mw, filename)) {
    fprintf(stderr, "%s: MagickReadImage %s failed\n", __FUNCTION__,
      filename);
    DestroyMagickWand(mw);
    return 0;
  }
  MagickTrimImage(mw, 0);
  return mw;
}

static fileItem *getFileListInDir(const char *dir, int *itemCount) {
  fileItem *fileList = 0;
  struct dirent *dp;
  int count = 0;
  DIR *dirp = opendir(dir);
  if (!dirp) {
    return 0;
  }
  while ((dp = readdir(dirp))) {
    MagickWand *mw;
    fileItem *newItem;
    if ('.' == dp->d_name[0]) {
      continue;
    }
    newItem = calloc(1, sizeof(fileItem));
    if (!newItem) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
      freeFileList(fileList);
      closedir(dirp);
      return 0;
    }
    sprintf(newItem->filename, "%s/%s", dir, dp->d_name);
    mw = createSpecificWand(newItem->filename);
    if (!mw) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__, "createSpecificWand failed");
      freeFileList(fileList);
      closedir(dirp);
      return 0;
    }
    newItem->width = MagickGetImageWidth(mw);
	  newItem->height = MagickGetImageHeight(mw);
    if (0 == newItem->width || 0 == newItem->height) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__,
        "0 == newItem->width || 0 == newItem->height");
      DestroyMagickWand(mw);
      freeFileList(fileList);
      closedir(dirp);
      return 0;
    }
    DestroyMagickWand(mw);
    newItem->next = fileList;
    fileList = newItem;
    ++count;
  }
  closedir(dirp);
  if (itemCount) {
    *itemCount = count;
  }
  return fileList;
}

typedef struct squeezerContext {
  fileItem *fileList;
  int itemCount;
  const char **filenameArray;
  maxRectsSize *inputs;
  maxRectsPosition *results;
  float bestOccupancy;
  maxRectsPosition *bestResults;
  MagickWand *binWand;
  PixelWand *transparentBackground;
  PixelWand *border;
  int binWidth;
  int binHeight;
} squeezerContext;

static void initSqueezerContext(squeezerContext *ctx) {
  memset(ctx, 0, sizeof(squeezerContext));
  MagickWandGenesis();
}

static void releaseSqueezerContext(squeezerContext *ctx) {
  if (ctx->binWand) {
    DestroyMagickWand(ctx->binWand);
    ctx->binWand = 0;
  }
  if (ctx->transparentBackground) {
    DestroyPixelWand(ctx->transparentBackground);
    ctx->transparentBackground = 0;
  }
  if (ctx->border) {
    DestroyPixelWand(ctx->border);
    ctx->border = 0;
  }
  MagickWandTerminus();
  if (ctx->fileList) {
    freeFileList(ctx->fileList);
    ctx->fileList = 0;
  }
  if (ctx->results) {
    free(ctx->results);
    ctx->results = 0;
  }
  if (ctx->bestResults) {
    free(ctx->bestResults);
    ctx->bestResults = 0;
  }
  if (ctx->filenameArray) {
    free(ctx->filenameArray);
    ctx->filenameArray = 0;
  }
}

static int outputInfo(squeezerContext *ctx, const char *outputInfoFilename) {
  int index;
  FILE *fp = fopen(outputInfoFilename, "w");
  if (!fp) {
    fprintf(stderr, "%s: fopen %s failed\n", __FUNCTION__,
      outputInfoFilename);
    return -1;
  }
  fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(fp, "<texture width=\"%d\" height=\"%d\">\n",
    ctx->binWidth, ctx->binHeight);
  for (index = 0; index < ctx->itemCount; ++index) {
    maxRectsSize *ipt = &ctx->inputs[index];
    maxRectsPosition *pos = &ctx->bestResults[index];
    const char *itemFilename = ctx->filenameArray[index];
    fprintf(fp,
      "    <sprite name=\"%s\" left=\"%d\" top=\"%d\" rotated=\"%s\" width=\"%d\" height=\"%d\"></sprite>\n",
      itemFilename, pos->left, pos->top, pos->rotated ? "true" : "false",
      ipt->width, ipt->height);
  }
  fprintf(fp, "</texture>\n");
  fclose(fp);
  return 0;
}

int squeezer(const char *dir, int binWidth, int binHeight, int allowRotations,
    const char *outputImageFilename, const char *outputInfoFilename,
    int border, int verbose) {
  int index;
  fileItem *loopItem;
  squeezerContext contextStruct;
  squeezerContext *ctx = &contextStruct;
  enum maxRectsFreeRectChoiceHeuristic methods[] = {
    rectBestShortSideFit, ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
    rectBestLongSideFit, ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
    rectBestAreaFit, ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
    rectBottomLeftRule, ///< -BL: Does the Tetris placement.
    rectContactPointRule ///< -CP: Choosest the placement where the rectangle touches other rects as much as possible.
  };

  if (verbose) {
    printf("preparing to squeezer\n");
  }

  initSqueezerContext(ctx);

  ctx->binWidth = binWidth;
  ctx->binHeight = binHeight;

  if (verbose) {
    printf("fetching file list from dir(%s)\n", dir);
  }

  ctx->fileList = getFileListInDir(dir, &ctx->itemCount);
  if (!ctx->fileList) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "getFileListInDir failed");
    releaseSqueezerContext(ctx);
    return -1;
  }

  if (verbose) {
    printf("alloc memory for squeezer\n");
  }

  ctx->filenameArray = (const char **)calloc(ctx->itemCount,
    sizeof(const char *));
  if (!ctx->filenameArray) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezerContext(ctx);
    return -1;
  }

  ctx->inputs = (maxRectsSize *)calloc(ctx->itemCount, sizeof(maxRectsSize));
  if (!ctx->inputs) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezerContext(ctx);
    return -1;
  }
  for (index = 0, loopItem = ctx->fileList; loopItem;
      loopItem = loopItem->next, ++index) {
    maxRectsSize *ipt = &ctx->inputs[index];
    ipt->width = loopItem->width;
    ipt->height = loopItem->height;
    ctx->filenameArray[index] = loopItem->filename;
  }

  ctx->results = (maxRectsPosition *)calloc(ctx->itemCount,
    sizeof(maxRectsPosition));
  if (!ctx->results) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezerContext(ctx);
    return -1;
  }

  ctx->bestResults = (maxRectsPosition *)calloc(ctx->itemCount,
    sizeof(maxRectsPosition));
  if (!ctx->results) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezerContext(ctx);
    return -1;
  }

  for (index = 0; index < sizeof(methods) / sizeof(methods[0]); ++index) {
    float occupancy = 0;
    enum maxRectsFreeRectChoiceHeuristic method = methods[index];
    if (verbose) {
      printf("calculating occupancy using method #%d\n", method);
    }
    if (0 != maxRects(binWidth, binHeight, ctx->itemCount, ctx->inputs,
        method, allowRotations, ctx->results, &occupancy)) {
      fprintf(stderr, "%s: maxRects method #%d failed\n", __FUNCTION__,
        method);
      continue;
    }
    if (verbose) {
      printf("occupancy #%d %.02f\n",
        method, occupancy);
    }
    if (occupancy > ctx->bestOccupancy) {
      ctx->bestOccupancy = occupancy;
      memcpy(ctx->bestResults, ctx->results,
        sizeof(maxRectsPosition) * ctx->itemCount);
    }
  }

  if (ctx->bestOccupancy <= 0) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "bestOccupancy <= 0");
    releaseSqueezerContext(ctx);
    return -1;
  }

  if (verbose) {
    printf("creating bin image\n");
  }

  ctx->transparentBackground = NewPixelWand();
  if (!ctx->transparentBackground) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "NewPixelWand failed");
    releaseSqueezerContext(ctx);
    return -1;
  }
  PixelSetColor(ctx->transparentBackground, "none");

  ctx->border = NewPixelWand();
  if (!ctx->border) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "NewPixelWand failed");
    releaseSqueezerContext(ctx);
    return -1;
  }
  PixelSetColor(ctx->border, "red");

  ctx->binWand = NewMagickWand();
  if (!ctx->binWand) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "NewMagickWand failed");
    releaseSqueezerContext(ctx);
    return -1;
  }
  MagickNewImage(ctx->binWand, binWidth, binHeight, ctx->transparentBackground);
  for (index = 0; index < ctx->itemCount; ++index) {
    MagickWand *itemWand;
    maxRectsSize *ipt = &ctx->inputs[index];
    maxRectsPosition *pos = &ctx->bestResults[index];
    const char *itemFilename = ctx->filenameArray[index];
    if (verbose) {
      printf("openninng image(%s)\n", itemFilename);
    }
    itemWand = createSpecificWand(itemFilename);
    if (!itemWand) {
      fprintf(stderr, "%s: createSpecificWand %s failed\n", __FUNCTION__,
        itemFilename);
      releaseSqueezerContext(ctx);
      return -1;
    }
    assert(ipt->width == MagickGetImageWidth(itemWand));
    assert(ipt->height == MagickGetImageHeight(itemWand));
    if (border) {
      MagickBorderImage(itemWand, ctx->border, 1, 1);
    }
    if (pos->rotated) {
      if (verbose) {
        printf("rotating image(%s)\n", itemFilename);
      }
      MagickRotateImage(itemWand, ctx->transparentBackground, 90);
    }
    if (verbose) {
      printf("coping image(%s) to bin left:%d top:%d width:%d height:%d\n",
        itemFilename, pos->left, pos->top, ipt->width, ipt->height);
    }
    if (MagickTrue != MagickCompositeImage(ctx->binWand, itemWand,
        OverCompositeOp, pos->left, pos->top)) {
      fprintf(stderr, "%s: MagickCompositeImage %s failed\n", __FUNCTION__,
        itemFilename);
      DestroyMagickWand(itemWand);
      releaseSqueezerContext(ctx);
      return -1;
    }
    DestroyMagickWand(itemWand);
  }

  if (verbose) {
    printf("outputing bin(%s)\n", outputImageFilename);
  }

  if (MagickTrue != MagickWriteImage(ctx->binWand, outputImageFilename)) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "MagickWriteImage failed");
    releaseSqueezerContext(ctx);
    return -1;
  }

  if (verbose) {
    printf("outputing info(%s)\n", outputInfoFilename);
  }

  if (0 != outputInfo(ctx, outputInfoFilename)) {
    fprintf(stderr, "%s: outputInfo %s failed\n", __FUNCTION__,
      outputInfoFilename);
    releaseSqueezerContext(ctx);
    return -1;
  }

  releaseSqueezerContext(ctx);

  if (verbose) {
    printf("done\n");
  }

  return 0;
}
