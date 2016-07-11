#include "squeezer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SQUEEZERW_VER "0.0.1 beta"

static const char *dir = 0;
static int binWidth = 512;
static int binHeight = 512;
static int allowRotations = 1;
static int verbose = 0;
static const char *outputFilename = "squeezer.png";
static int border = 0;

static void usage(void) {
  fprintf(stderr, "squeezerw " SQUEEZERW_VER "\n"
    "usage: squeezerw [options] <sprite image dir>\n"
    "    options:\n"
    "        --width <output width>\n"
    "        --height <output height>\n"
    "        --allowRotations <1/0/true/false/yes/no>\n"
    "        --border <1/0/true/false/yes/no>\n"
    "        --output <output filename>\n"
    "        --verbose\n"
    "        --version\n");
}

static int parseBooleanParam(const char *param) {
  switch (param[0]) {
    case 'F':
    case 'f':
    case 'N':
    case 'n':
    case '0':
      return 0;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  int i;
  if (argc < 2) {
    usage();
    return -1;
  }
  for (i = 1; i < argc; ++i) {
    const char *param = argv[i];
    if ('-' == param[0]) {
      if (0 == strcmp(param, "--version")) {
        printf(SQUEEZERW_VER "\n");
        return 0;
      } else if (0 == strcmp(param, "--width")) {
        binWidth = atoi(argv[++i]);
      } else if (0 == strcmp(param, "--height")) {
        binHeight = atoi(argv[++i]);
      } else if (0 == strcmp(param, "--allowRotations")) {
        allowRotations = parseBooleanParam(argv[++i]);
      } else if (0 == strcmp(param, "--border")) {
        border = parseBooleanParam(argv[++i]);
      } else if (0 == strcmp(param, "--output")) {
        outputFilename = argv[++i];
      } else if (0 == strcmp(param, "--verbose")) {
        verbose = 1;
      } else {
        usage();
        fprintf(stderr, "%s: unknown arg: %s\n", __FUNCTION__, param);
        return -1;
      }
    } else {
      dir = param;
    }
  }
  if (!dir) {
    usage();
    return -1;
  }
  if (verbose) {
    printf("squeezering using the follow options:\n"
      "    --width %d\n"
      "    --height %d\n"
      "    --allowRotations %s\n"
      "    --border %s\n"
      "    --output %s\n"
      "%s",
      binWidth,
      binHeight,
      allowRotations ? "true" : "false",
      border ? "true" : "false",
      outputFilename,
      verbose ? "    --verbose\n" : "");
  }
  return squeezer(dir, binWidth, binHeight, allowRotations, outputFilename,
    border, verbose);
}
