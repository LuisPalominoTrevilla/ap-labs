#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include "logger.h"

#define MAX_PRINT_SIZE 1024
#define BUF_LEN 1024

#define WHITESPACE 64
#define EQUALS 65
#define INVALID 66

static const unsigned char d[] = {
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 64, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 62, 66, 66, 66, 63, 52, 53,
    54, 55, 56, 57, 58, 59, 60, 61, 66, 66, 66, 65, 66, 66, 66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 66, 66, 66, 66, 66, 66, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66};

size_t sourceBytes = 0;
size_t currentByte;
int encodingMode;

// Base64 encoding C code retrieved from https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64#C
int base64encode(const char *data_buf, size_t dataLength, char *result, size_t resultSize)
{
  const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const uint8_t *data = (const uint8_t *)data_buf;
  unsigned long long resultIndex = 0;
  uint32_t n = 0;
  int padCount = dataLength % 3;
  uint8_t n0, n1, n2, n3;

  /* increment over the length of the string, three characters at a time */
  for (currentByte = 0; currentByte < dataLength; currentByte += 3)
  {
    /* these three 8-bit (ASCII) characters become one 24-bit number */
    n = ((uint32_t)data[currentByte]) << 16; //parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0

    if ((currentByte + 1) < dataLength)
      n += ((uint32_t)data[currentByte + 1]) << 8; //parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0

    if ((currentByte + 2) < dataLength)
      n += data[currentByte + 2];

    /* this 24-bit number gets separated into four 6-bit numbers */
    n0 = (uint8_t)(n >> 18) & 63;
    n1 = (uint8_t)(n >> 12) & 63;
    n2 = (uint8_t)(n >> 6) & 63;
    n3 = (uint8_t)n & 63;

    /*
       * if we have one byte available, then its encoding is spread
       * out over two characters
       */
    if (resultIndex >= resultSize)
      return 1; /* indicate failure: buffer too small */
    result[resultIndex++] = base64chars[n0];
    if (resultIndex >= resultSize)
      return 1; /* indicate failure: buffer too small */
    result[resultIndex++] = base64chars[n1];

    /*
       * if we have only two bytes available, then their encoding is
       * spread out over three chars
       */
    if ((currentByte + 1) < dataLength)
    {
      if (resultIndex >= resultSize)
        return 1; /* indicate failure: buffer too small */
      result[resultIndex++] = base64chars[n2];
    }

    /*
       * if we have all three bytes available, then their encoding is spread
       * out over four characters
       */
    if ((currentByte + 2) < dataLength)
    {
      if (resultIndex >= resultSize)
        return 1; /* indicate failure: buffer too small */
      result[resultIndex++] = base64chars[n3];
    }
  }

  /*
    * create and add padding that is required if we did not have a multiple of 3
    * number of characters available
    */
  if (padCount > 0)
  {
    for (; padCount < 3; padCount++)
    {
      if (resultIndex >= resultSize)
        return 1; /* indicate failure: buffer too small */
      result[resultIndex++] = '=';
    }
  }
  if (resultIndex >= resultSize)
    return 1; /* indicate failure: buffer too small */
  result[resultIndex] = 0;
  return 0; /* indicate success */
}

// Base64 decoding C code retrieved from https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64#C
int base64decode(char *in, size_t inLen, unsigned char *out, size_t *outLen)
{
  char *end = in + inLen;
  char iter = 0;
  uint32_t buf = 0;
  size_t len = 0;

  currentByte = 0;
  while (in < end)
  {
    currentByte++;
    unsigned char c = d[*in++];

    switch (c)
    {
    case WHITESPACE:
      continue; /* skip whitespace */
    case INVALID:
      return 1;  /* invalid input, return error */
    case EQUALS: /* pad character, end of data */
      in = end;
      continue;
    default:
      buf = buf << 6 | c;
      iter++; // increment the number of iteration
      /* If the buffer is full, split it into bytes */
      if (iter == 4)
      {
        if ((len += 3) > *outLen)
          return 1; /* buffer overflow */
        *(out++) = (buf >> 16) & 255;
        *(out++) = (buf >> 8) & 255;
        *(out++) = buf & 255;
        buf = 0;
        iter = 0;
      }
    }
  }

  if (iter == 3)
  {
    if ((len += 2) > *outLen)
      return 1; /* buffer overflow */
    *(out++) = (buf >> 10) & 255;
    *(out++) = (buf >> 2) & 255;
  }
  else if (iter == 2)
  {
    if (++len > *outLen)
      return 1; /* buffer overflow */
    *(out++) = (buf >> 4) & 255;
  }

  *outLen = len; /* modify to reflect the actual output size */
  return 0;
}

void displayProgress()
{
  int progress = (float)currentByte / sourceBytes * 100;
  infof("%s is %d%% complete\n", encodingMode ? "Encoding" : "Decoding", progress);
}

void extractFilename(char *dest, const char *filename)
{
  char *ext = strchr(filename, '.');
  strncpy(dest, filename, ext - filename);
  dest[ext - filename] = '\0';
}

int main(int argc, char **argv)
{
  char *mode, *srcPath;
  FILE *srcFile, *destFile;
  char *srcBuf;
  char *destBuf;

  initLogger("stdout");
  if (argc != 3)
  {
    errorf("Usage: ./base64 [--encode|--decode] name_of_file.txt\n");
    return 0;
  }

  mode = argv[1];
  srcPath = argv[2];
  if (strcmp(mode, "--encode") != 0 && strcmp(mode, "--decode") != 0)
  {
    errorf("Unexpected mode %s, please use --encode or --decode\n", mode);
    return 0;
  }
  encodingMode = strcmp(mode, "--decode");

  // Set handlers for signals to display progress
  signal(SIGINT, displayProgress);
  signal(SIGUSR1, displayProgress);

  // Read input file to surce buffer
  if ((srcFile = fopen(srcPath, "r")) == NULL)
  {
    panicf("File %s could not be read\n", srcPath);
    return 0;
  }

  struct stat st;
  stat(srcPath, &st);
  srcBuf = malloc(sizeof(char) * st.st_size);
  if (srcBuf == NULL) {
    panicf("Cannot allocate memory for source buffer\n");
    fclose(srcFile);
    return 0;
  }

  size_t outputLength = 4 * ((st.st_size + 2) / 3.0) + 3;
  destBuf = malloc(sizeof(char) * (outputLength));
  if (destBuf == NULL) {
    panicf("Cannot allocate memory for destination buffer\n");
    fclose(srcFile);
    return 0;
  }

  int read;
  char buf[BUF_LEN];
  while ((read = fread(buf, sizeof(char), BUF_LEN, srcFile)) > 0)
  {
    size_t ncpy = BUF_LEN > st.st_size - sourceBytes ? st.st_size - sourceBytes : BUF_LEN;
    strncpy(srcBuf + sourceBytes, buf, ncpy);
    sourceBytes += read;
  }

  fclose(srcFile);

  // Perform operation (encoding/decoding)
  if (encodingMode && base64encode(srcBuf, sourceBytes, destBuf, outputLength) != 0)
  {
    panicf("Unable to encode file\n");
    free(srcBuf);
    free(destBuf);
    return 0;
  }
  if (!encodingMode && base64decode(srcBuf, sourceBytes, (unsigned char *)destBuf, &outputLength) != 0)
  {
    panicf("Unable to decode file\n");
    free(srcBuf);
    free(destBuf);
    return 0;
  }

  // Write file with output (encoded/decoded) string
  char destPath[50];
  extractFilename(destPath, srcPath);
  sprintf(destPath, "%s-%s.txt", destPath, encodingMode ? "encoded" : "decoded");
  if ((destFile = fopen(destPath, "w")) == NULL)
  {
    panicf("Unable to write to destination file\n");
    free(srcBuf);
    free(destBuf);
    return 0;
  }

  size_t destLen = strlen(destBuf);
  size_t totalPrinted = 0;
  size_t printed;
  size_t maxRead;
  while (totalPrinted < destLen)
  {
    maxRead = MAX_PRINT_SIZE > destLen - totalPrinted ? destLen - totalPrinted : MAX_PRINT_SIZE;
    if ((printed = fwrite(destBuf + totalPrinted, sizeof(char), maxRead, destFile)) != maxRead)
    {
      panicf("Unable to write to destination file\n");
      fclose(destFile);
      free(srcBuf);
      free(destBuf);
      return 0;
    }

    totalPrinted += printed;
  }

  infof("%s completed sucesfully. Output file is %s\n", encodingMode ? "Encoding" : "Decoding", destPath);

  fclose(destFile);
  free(srcBuf);
  free(destBuf);
  return 0;
}
