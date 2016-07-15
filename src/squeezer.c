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

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "squeezer.h"
#include "maxrects.h"
#include "imageops.h"

typedef struct fileItem {
  struct fileItem *next;
  char filename[780];
  char shortName[780];
  int width;
  int height;
  int originWidth;
  int originHeight;
} fileItem;

static void freeFileList(fileItem *list) {
  while (list) {
    fileItem *willDel = list;
    list = list->next;
    free(willDel);
  }
}

static imageOpsImage *createSpecificImage(const char *filename,
    int *offsetLeft, int *offsetTop, int *originWidth, int *originHeight) {
  imageOpsImage *img = imageOpsOpen(filename);
  if (!img) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "imageOpsOpen failed");
    return 0;
  }
  if (originWidth) {
    *originWidth = imageOpsGetWidth(img);
  }
  if (originHeight) {
    *originHeight = imageOpsGetHeight(img);
  }
  imageOpsTrim(img, offsetLeft, offsetTop);
  return img;
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
    imageOpsImage *img;
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
    snprintf(newItem->shortName, sizeof(newItem->shortName), "%s", dp->d_name);
    snprintf(newItem->filename, sizeof(newItem->filename),
      "%s/%s", dir, dp->d_name);
    img = createSpecificImage(newItem->filename, 0, 0, 0, 0);
    if (!img) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__, "createSpecificImage failed");
      freeFileList(fileList);
      closedir(dirp);
      return 0;
    }
    newItem->width = imageOpsGetWidth(img);
	  newItem->height = imageOpsGetHeight(img);
    if (0 == newItem->width || 0 == newItem->height) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__,
        "0 == newItem->width || 0 == newItem->height");
      imageOpsDestroy(img);
      freeFileList(fileList);
      closedir(dirp);
      return 0;
    }
    imageOpsDestroy(img);
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

typedef struct trimInfo {
  int offsetLeft;
  int offsetTop;
  int originWidth;
  int originHeight;
} trimInfo;

struct squeezer {
  fileItem *fileList;
  int itemCount;
  const char **filenameArray;
  const char **shortNameArray;
  maxRectsSize *inputs;
  maxRectsPosition *results;
  trimInfo *trimInfos;
  float bestOccupancy;
  maxRectsPosition *bestResults;
  imageOpsImage *binImage;
  int binWidth;
  int binHeight;
  int verbose:1;
  int border:1;
  int allowRotations:1;
};

static void initSqueezer(squeezer *ctx) {
  memset(ctx, 0, sizeof(squeezer));
}

static void releaseSqueezer(squeezer *ctx) {
  if (ctx->binImage) {
    imageOpsDestroy(ctx->binImage);
    ctx->binImage = 0;
  }
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
  if (ctx->shortNameArray) {
    free(ctx->shortNameArray);
    ctx->shortNameArray = 0;
  }
  if (ctx->trimInfos) {
    free(ctx->trimInfos);
    ctx->trimInfos = 0;
  }
}

static int outputInfo(squeezer *ctx, const char *outputInfoFilename) {
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
    const char *itemShortName = ctx->shortNameArray[index];
    trimInfo *trim = &ctx->trimInfos[index];
    fprintf(fp,
      "    <sprite name=\"%s\" left=\"%d\" top=\"%d\" rotated=\"%s\" width=\"%d\" height=\"%d\"",
      itemShortName, pos->left, pos->top, pos->rotated ? "true" : "false",
      ipt->width, ipt->height);
    fprintf(fp, " trimOffsetLeft=\"%d\" trimOffsetTop=\"%d\" originWidth=\"%d\" originHeight=\"%d\"",
      trim->offsetLeft, trim->offsetTop, trim->originWidth, trim->originHeight);
    fprintf(fp, "></sprite>\n");
  }
  fprintf(fp, "</texture>\n");
  fclose(fp);
  return 0;
}

squeezer *squeezerCreate(void) {
  squeezer *ctx = (squeezer *)calloc(1, sizeof(squeezer));
  if (!ctx) {
    fprintf(stderr, "%s: calloc failed\n", __FUNCTION__);
    return 0;
  }
  imageOpsInit();
  initSqueezer(ctx);
  return ctx;
}

void squeezerSetBinWidth(squeezer *ctx, int width) {
  ctx->binWidth = width;
}

void squeezerSetBinHeight(squeezer *ctx, int height) {
  ctx->binHeight = height;
}

void squeezerSetAllowRotations(squeezer *ctx, int allowRotations) {
  ctx->allowRotations = allowRotations;
}

void squeezerSetVerbose(squeezer *ctx, int verbose) {
  ctx->verbose = verbose;
}

void squeezerSetHasBorder(squeezer *ctx, int hasBorder) {
  ctx->border = hasBorder;
}

int squeezerDoDir(squeezer *ctx, const char *dir) {
  int index;
  fileItem *loopItem;

  enum maxRectsFreeRectChoiceHeuristic methods[] = {
    rectBestShortSideFit, ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
    rectBestLongSideFit, ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
    rectBestAreaFit, ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
    rectBottomLeftRule, ///< -BL: Does the Tetris placement.
    rectContactPointRule ///< -CP: Choosest the placement where the rectangle touches other rects as much as possible.
  };

  releaseSqueezer(ctx);

  if (ctx->verbose) {
    printf("preparing to squeezer\n");
  }

  if (ctx->verbose) {
    printf("fetching file list from dir(%s)\n", dir);
  }

  ctx->fileList = getFileListInDir(dir, &ctx->itemCount);
  if (!ctx->fileList) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "getFileListInDir failed");
    releaseSqueezer(ctx);
    return -1;
  }

  if (ctx->verbose) {
    printf("alloc memory for squeezer\n");
  }

  ctx->filenameArray = (const char **)calloc(ctx->itemCount,
    sizeof(const char *));
  if (!ctx->filenameArray) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezer(ctx);
    return -1;
  }

  ctx->shortNameArray = (const char **)calloc(ctx->itemCount,
    sizeof(const char *));
  if (!ctx->shortNameArray) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezer(ctx);
    return -1;
  }

  ctx->inputs = (maxRectsSize *)calloc(ctx->itemCount, sizeof(maxRectsSize));
  if (!ctx->inputs) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezer(ctx);
    return -1;
  }
  for (index = 0, loopItem = ctx->fileList; loopItem;
      loopItem = loopItem->next, ++index) {
    maxRectsSize *ipt = &ctx->inputs[index];
    ipt->width = loopItem->width;
    ipt->height = loopItem->height;
    ctx->filenameArray[index] = loopItem->filename;
    ctx->shortNameArray[index] = loopItem->shortName;
  }

  ctx->results = (maxRectsPosition *)calloc(ctx->itemCount,
    sizeof(maxRectsPosition));
  if (!ctx->results) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezer(ctx);
    return -1;
  }

  ctx->bestResults = (maxRectsPosition *)calloc(ctx->itemCount,
    sizeof(maxRectsPosition));
  if (!ctx->results) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezer(ctx);
    return -1;
  }

  ctx->trimInfos = (trimInfo *)calloc(ctx->itemCount,
    sizeof(trimInfo));
  if (!ctx->trimInfos) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    releaseSqueezer(ctx);
    return -1;
  }

  for (index = 0; index < sizeof(methods) / sizeof(methods[0]); ++index) {
    float occupancy = 0;
    enum maxRectsFreeRectChoiceHeuristic method = methods[index];
    if (ctx->verbose) {
      printf("calculating occupancy using method #%d\n", method);
    }
    if (0 != maxRects(ctx->binWidth, ctx->binHeight, ctx->itemCount,
        ctx->inputs, method, ctx->allowRotations, ctx->results, &occupancy)) {
      fprintf(stderr, "%s: maxRects method #%d failed\n", __FUNCTION__,
        method);
      continue;
    }
    if (ctx->verbose) {
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
    releaseSqueezer(ctx);
    return -1;
  }

  if (ctx->verbose) {
    printf("creating bin image\n");
  }

  ctx->binImage = imageOpsCreate(ctx->binWidth, ctx->binHeight);
  for (index = 0; index < ctx->itemCount; ++index) {
    imageOpsImage *itemImage;
    maxRectsSize *ipt = &ctx->inputs[index];
    maxRectsPosition *pos = &ctx->bestResults[index];
    const char *itemFilename = ctx->filenameArray[index];
    int trimOffsetLeft = 0;
    int trimOffsetTop = 0;
    int originWidth = 0;
    int originHeight = 0;
    if (ctx->verbose) {
      printf("openninng image(%s)\n", itemFilename);
    }
    itemImage = createSpecificImage(itemFilename, &trimOffsetLeft,
      &trimOffsetTop, &originWidth, &originHeight);
    assert(ipt->width == imageOpsGetWidth(itemImage));
    assert(ipt->height == imageOpsGetHeight(itemImage));
    ctx->trimInfos[index].offsetLeft = trimOffsetLeft;
    ctx->trimInfos[index].offsetTop = trimOffsetTop;
    ctx->trimInfos[index].originWidth = originWidth;
    ctx->trimInfos[index].originHeight = originHeight;
    if (ctx->border) {
      imageOpsAddBorder(itemImage);
    }
    if (pos->rotated) {
      if (ctx->verbose) {
        printf("rotating image(%s)\n", itemFilename);
      }
      imageOpsRotate(itemImage, 90);
    }
    if (ctx->verbose) {
      printf("coping image(%s) to bin left:%d top:%d width:%d height:%d\n",
        itemFilename, pos->left, pos->top, ipt->width, ipt->height);
    }
    if (0 != imageOpsComposite(ctx->binImage, itemImage, pos->left,
        pos->top)) {
      fprintf(stderr, "%s: imageOpsComposite %s failed\n", __FUNCTION__,
        itemFilename);
      imageOpsDestroy(itemImage);
      releaseSqueezer(ctx);
      return -1;
    }
    imageOpsDestroy(itemImage);
  }

  return 0;
}

void squeezerDestroy(squeezer *ctx) {
  imageOpsUninit();
  releaseSqueezer(ctx);
  free(ctx);
}

int squeezerOutputImage(squeezer *ctx, const char *filename) {
  if (ctx->verbose) {
    printf("outputing bin(%s)\n", filename);
  }
  if (0 != imageOpsSave(ctx->binImage, filename)) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "imageOpsSave failed");
    releaseSqueezer(ctx);
    return -1;
  }
  return 0;
}

int squeezerOutputXml(squeezer *ctx, const char *filename) {
  if (ctx->verbose) {
    printf("outputing xml(%s)\n", filename);
  }
  if (0 != outputInfo(ctx, filename)) {
    fprintf(stderr, "%s: squeezerOutputXml %s failed\n", __FUNCTION__,
      filename);
    releaseSqueezer(ctx);
    return -1;
  }
  return 0;
}

typedef struct customOutput {
  FILE *fp;
  int imageWidth;
  int imageHeight;
  int x;
  int y;
  int trimOffsetLeft;
  int trimOffsetTop;
  int originWidth;
  int originHeight;
  int rotated;
  const char *shortName;
} customOutput;

static void parse(squeezer *ctx, customOutput *output, const char *fmt) {
  const char *p = fmt;
  while (*p) {
    if ('%' == *p) {
      switch (*(p + 1)) {
        case 'W':
          fprintf(output->fp, "%d", ctx->binWidth);
          break;
        case 'H':
          fprintf(output->fp, "%d", ctx->binHeight);
          break;
        case 'n':
          fprintf(output->fp, "%s", output->shortName);
          break;
        case 'w':
          fprintf(output->fp, "%d", output->imageWidth);
          break;
        case 'h':
          fprintf(output->fp, "%d", output->imageHeight);
          break;
        case 'x':
          fprintf(output->fp, "%d", output->x);
          break;
        case 'y':
          fprintf(output->fp, "%d", output->y);
          break;
        case 'l':
          fprintf(output->fp, "%d", output->trimOffsetLeft);
          break;
        case 't':
          fprintf(output->fp, "%d", output->trimOffsetTop);
          break;
        case 'c':
          fprintf(output->fp, "%d", output->originWidth);
          break;
        case 'r':
          fprintf(output->fp, "%d", output->originHeight);
          break;
        case 'f':
          fprintf(output->fp, "%d", output->rotated ? 1 : 0);
          break;
        case '%':
          fprintf(output->fp, "%c", '%');
          break;
        default:
          fprintf(stderr, "warn: unknown format specifier: %c%c\n",
            *p, *(p + 1));
          fprintf(output->fp, "%c", *(p + 1));
      }
      p += 2;
    } else if ('\\' == *p) {
      switch (*(p + 1)) {
        case 'n':
          fprintf(output->fp, "\n");
          break;
        case 'r':
          fprintf(output->fp, "\r");
          break;
        case 't':
          fprintf(output->fp, "\t");
          break;
        case '\\':
          fprintf(output->fp, "%c", '\\');
          break;
        default:
          fprintf(stderr, "warn: unknown format specifier: %c%c\n",
            *p, *(p + 1));
          fprintf(output->fp, "%c", *(p + 1));
      }
      p += 2;
    } else {
      fprintf(output->fp, "%c", *p);
      p += 1;
    }
  }
}

static void releaseCustomOutput(customOutput *output) {
  if (output->fp) {
    fclose(output->fp);
    output->fp = 0;
  }
}

int squeezerOutputCustomFormat(squeezer *ctx, const char *filename,
    const char *header, const char *body, const char *footer,
    const char *split) {
  customOutput output;
  int index;

  if (ctx->verbose) {
    printf("outputing custom format(%s)\n", filename);
  }

  memset(&output, 0, sizeof(output));

  output.fp = fopen(filename, "w");
  if (!output.fp) {
    fprintf(stderr, "%s: fopen %s failed\n", __FUNCTION__,
      filename);
    return -1;
  }

  if (header) {
    parse(ctx, &output, header);
  }

  for (index = 0; index < ctx->itemCount; ++index) {
    maxRectsSize *ipt = &ctx->inputs[index];
    maxRectsPosition *pos = &ctx->bestResults[index];
    const char *itemShortName = ctx->shortNameArray[index];
    trimInfo *trim = &ctx->trimInfos[index];

    if (split) {
      if (0 != index) {
        parse(ctx, &output, split);
      }
    }

    output.shortName = itemShortName;
    output.imageWidth = ipt->width;
    output.imageHeight = ipt->height;
    output.x = pos->left;
    output.y = pos->top;
    output.trimOffsetLeft = trim->offsetLeft;
    output.trimOffsetTop = trim->offsetTop;
    output.originWidth = trim->originWidth;
    output.originHeight = trim->originHeight;
    output.rotated = pos->rotated;
    parse(ctx, &output, body);
  }

  if (footer) {
    parse(ctx, &output, footer);
  }

  releaseCustomOutput(&output);

  return 0;
}
