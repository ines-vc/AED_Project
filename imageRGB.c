/// imageRGB - A simple image module for handling RGB images,
///            pixel color values are represented using a look-up table (LUT)
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// The AED Team <jmadeira@ua.pt, jmr@ua.pt, ...>
/// 2025

// Student authors (fill in below):
// NMec: 125738
// Name: Inês Cardoso
// NMec: 126541
// Name: Jéssica Zheng
//
// Date: 17/11/2025
//

#include "imageRGB.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PixelCoords.h"
#include "PixelCoordsQueue.h"
#include "PixelCoordsStack.h"
#include "instrumentation.h"

// The data structure
//
// A RGB image is stored in a structure containing 5 fields:
// Two integers store the image width and height.
// The next field is a pointer to an array that stores the pointers
// to the image rows.
//
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// FIXED SIZE of LUT for storing RGB triplets
#define FIXED_LUT_SIZE 1000

// Internal structure for storing RGB images
struct image {
  uint32 width;
  uint32 height;
  uint16** image;     // pointer to an array of pointers referencing the image rows
  uint16 num_colors;  // the number of colors (i.e., pixel labels) used
  rgb_t* LUT;         // table storing (R,G,B) triplets
};

// Design by Contract

// This module follows "design-by-contract" principles.
// `assert` is used to check function preconditions, postconditions
// and type invariants.
// This helps to find programmer errors.

/// Defensive Error Handling

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
//
// When one of these functions detects a memory or I/O error,
// it immediately prints an error message and aborts the program.
// This is a Fail-Fast strategy.
//
// You may use the `check` function to check a condition
// and exit the program with an error message if it is false.
// Note that it works similarly to `assert`, but cannot be disabled.
// It should be used to detect "external" uncontrolable errors,
// and not for "internal" programmer errors.
//
// See how it's used in ImageLoadPBM, for example.

// Check a condition and if false, print failmsg and exit.
static void check(int condition, const char* failmsg) {
  if (!condition) {
    perror(failmsg);
    exit(errno || 255);
  }
}

/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) {  ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Auxiliary (static) functions

static Image AllocateImageHeader(uint32 width, uint32 height) {
  // Create the header of an image data structure
  // Allocate the array of pointers to rows
  // And the look-up table

  Image newHeader = malloc(sizeof(struct image));
  // Error handling
  check(newHeader != NULL, "malloc");

  newHeader->width = width;
  newHeader->height = height;

  // Allocating the array of pointers to image rows
  newHeader->image = malloc(height * sizeof(uint16*));
  // Error handling
  check(newHeader->image != NULL, "Alloc failed ->image array");

  // Allocating the LUT
  newHeader->LUT = malloc(FIXED_LUT_SIZE * sizeof(rgb_t));
  // Error handling
  check(newHeader->LUT != NULL, "Alloc failed ->LUT array");

  // Initialize LUT with 2 fixed colors
  newHeader->num_colors = 2;
  newHeader->LUT[0] = 0xffffff;  // RGB WHITE
  newHeader->LUT[1] = 0x000000;  // RGB BLACK

  return newHeader;
}

// Allocate row of background (label=0) pixels
static uint16* AllocateRowArray(uint32 size) {
  uint16* newArray = calloc((size_t)size, sizeof(uint16));
  // Error handling
  check(newArray != NULL, "AllocateRowArray");

  return newArray;
}

/// Find color label for given RGB color in img LUT.
/// Return the label or -1 if not found.
static int LUTFindColor(Image img, rgb_t color) {
  for (uint16 index = 0; index < img->num_colors; index++) {
    if (img->LUT[index] == color) return index;
  }
  return -1;
}

/// Return color label for RGB color in img LUT.
/// Finds existing color or allocs new one!
static int LUTAllocColor(Image img, rgb_t color) {
  int index = LUTFindColor(img, color);
  if (index < 0) {
    check(img->num_colors < FIXED_LUT_SIZE, "LUT Overflow");
    index = img->num_colors++;
    img->LUT[index] = color;
  }
  return index;
}

/// Return a pseudo-random successor of the given color.
static rgb_t GenerateNextColor(rgb_t color) {
  return (color + 7639) & 0xffffff;
}

/// Image management functions

/// Create a new RGB image. All pixels with the background WHITE color.
///   width, height: the dimensions of the new image.
/// Requires: width and height must be non-negative.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageCreate(uint32 width, uint32 height) {
  assert(width > 0);
  assert(height > 0);

  // Just two possible pixel colors
  Image img = AllocateImageHeader(width, height);

  // Creating the image rows
  for (uint32 i = 0; i < height; i++) {
    img->image[i] = AllocateRowArray(width);  // Alloc all WHITE row
  }

  return img;
}

/// Create a new RGB image, with a color chess pattern.
/// The background is WHITE.
///   width, height: the dimensions of the new image.
///   edge: the width and height of a chess square.
///   color: the foreground color.
/// Requires: width, height and edge must be non-negative.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageCreateChess(uint32 width, uint32 height, uint32 edge, rgb_t color) {
  assert(width > 0);
  assert(height > 0);
  assert(edge > 0);

  Image img = ImageCreate(width, height);

  // Alloc color in LUT.
  uint8 label = LUTAllocColor(img, color);

  // Assigning the color to each image pixel

  // Pixel (0, 0) gets the chosen color label
  for (uint32 i = 0; i < height; i++) {
    uint32 I = i / edge;
    for (uint32 j = 0; j < width; j++) {
      uint32 J = j / edge;
      img->image[i][j] = (I + J) % 2 ? 0 : label;
    }
  }

  // Return the created chess image
  return img;
}

/// Create an image with a palete of generated colors.
Image ImageCreatePalete(uint32 width, uint32 height, uint32 edge) {
  assert(width > 0);
  assert(height > 0);
  assert(edge > 0);

  Image img = ImageCreate(width, height);

  // Fill LUT with generated colors
  rgb_t color = 0x000000;
  while (img->num_colors < FIXED_LUT_SIZE) {
    color = GenerateNextColor(color);
    img->LUT[img->num_colors++] = color;
  }

  // number of tiles
  uint32 wtiles = width / edge;

  // Pixel (0, 0) gets the chosen color label
  for (uint32 i = 0; i < height; i++) {
    uint32 I = i / edge;
    for (uint32 j = 0; j < width; j++) {
      uint32 J = j / edge;
      img->image[i][j] = (I * wtiles + J) % FIXED_LUT_SIZE;
    }
  }

  return img;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
///
/// Ensures: (*imgp)==NULL.
void ImageDestroy(Image* imgp) {
  assert(imgp != NULL);

  Image img = *imgp;

  for (uint32 i = 0; i < img->height; i++) {
    free(img->image[i]);
  }
  free(img->image);
  free(img->LUT);
  free(img);

  *imgp = NULL;
}

/// Create a deep copy of the image pointed to by img.
///   img : address of an Image variable.
///
/// On success, a new copied image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageCopy(const Image img) {
  assert(img != NULL);

  // Criar uma nova imagem com as mesmas dimensões da imagem original.
  Image copyImg = AllocateImageHeader(img->width, img->height);

  // Copia o número de cores utilizadas na LUT (Look-Up Table) para a imagem copiada.
  copyImg->num_colors = img->num_colors;

  // Copiar toda a LUT da imagem original para a imagem copiada.
  // Como as cores estão no formato LUT (num_colors) cada uma com um valor RGB (rgb_t),
  // temos de multiplicar pelo tamanho de LUT (num_colors).
  memcpy(copyImg->LUT, img->LUT, img->num_colors * sizeof(rgb_t));

  // Localizar e copiar linha por linha da imagem.
  for (uint32 i = 0; i < img->height; i++) {
    copyImg->image[i] = AllocateRowArray(img->width);
    memcpy(copyImg->image[i], img->image[i], img->width * sizeof(uint16));
  }

  return copyImg;                                         // Retornar a imagem copiada.
}

/// Printing on the console

/// These functions do not modify the image and never fail.

/// Output the raw RGB image (i.e., print the integer value of pixel).
void ImageRAWPrint(const Image img) {
  printf("width = %d height = %d\n", (int)img->width, (int)img->height);
  printf("num_colors = %d\n", (int)img->num_colors);
  printf("RAW image\n");

  // Print the pixel labels of each image row
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      printf("%2d", img->image[i][j]);
    }
    // At current row end
    printf("\n");
  }

  printf("LUT:\n");
  // Print the LUT (R,G,B) values
  for (int i = 0; i < (int)img->num_colors; i++) {
    rgb_t color = img->LUT[i];
    int r = color >> 16 & 0xff;
    int g = color >> 8 & 0xff;
    int b = color & 0xff;
    printf("%3d -> (%3d,%3d,%3d)\n", i, r, g, b);
  }

  printf("\n");
}

/// PBM file operations --- For BW images

// See PBM format specification: http://netpbm.sourceforge.net/doc/pbm.html

//
static void unpackBits(int nbytes, const uint8 bytes[], uint8 raw_row[]) {
  // bitmask starts at top bit
  int offset = 0;
  uint8 mask = 1 << (7 - offset);
  while (offset < 8) {  // or (mask > 0)
    for (int b = 0; b < nbytes; b++) {
      raw_row[8 * b + offset] = (bytes[b] & mask) != 0;
    }
    mask >>= 1;
    offset++;
  }
}

static void packBits(int nbytes, uint8 bytes[], const uint8 raw_row[]) {
  // bitmask starts at top bit
  int offset = 0;
  uint8 mask = 1 << (7 - offset);
  while (offset < 8) {  // or (mask > 0)
    for (int b = 0; b < nbytes; b++) {
      if (offset == 0) bytes[b] = 0;
      bytes[b] |= raw_row[8 * b + offset] ? mask : 0;
    }
    mask >>= 1;
    offset++;
  }
}

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PBM file.
/// Only binary PBM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageLoadPBM(const char* filename) {  ///
  int w, h;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  check((f = fopen(filename, "rb")) != NULL, "Open failed");
  // Parse PBM header
  check(fscanf(f, "P%c ", &c) == 1 && c == '4', "Invalid file format");
  skipComments(f);
  check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width");
  skipComments(f);
  check(fscanf(f, "%d", &h) == 1 && h >= 0, "Invalid height");
  check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected");

  // Allocate image
  img = AllocateImageHeader((uint32)w, (uint32)h);

  // Read pixels
  int nbytes = (w + 8 - 1) / 8;  // number of bytes for each row
  // using VLAs...
  uint8 bytes[nbytes];
  uint8 raw_row[nbytes * 8];
  for (uint32 i = 0; i < img->height; i++) {
    check(fread(bytes, sizeof(uint8), nbytes, f) == (size_t)nbytes,
          "Reading pixels");
    unpackBits(nbytes, bytes, raw_row);
    img->image[i] = AllocateRowArray((uint32)w);
    for (uint32 j = 0; j < (uint32)w; j++) {
      img->image[i][j] = (uint16)raw_row[j];
    }
  }

  fclose(f);
  return img;
}

/// Save image to PBM file.
/// On success, returns nonzero.
/// On failure, a partial and invalid file may be left in the system.
int ImageSavePBM(const Image img, const char* filename) {  ///
  assert(img != NULL);
  assert(img->num_colors == 2);

  int w = (int)img->width;
  int h = (int)img->height;
  FILE* f = NULL;

  check((f = fopen(filename, "wb")) != NULL, "Open failed");
  check(fprintf(f, "P4\n%d %d\n", w, h) > 0, "Writing header failed");

  // Write pixels
  int nbytes = (w + 8 - 1) / 8;  // number of bytes for each row
  // using VLAs...
  uint8 bytes[nbytes];
  uint8 raw_row[nbytes * 8];
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      raw_row[j] = (uint8)img->image[i][j];
    }
    // Fill padding pixels with WHITE
    memset(raw_row + w, WHITE, nbytes * 8 - w);
    packBits(nbytes, bytes, raw_row);
    check(fwrite(bytes, sizeof(uint8), nbytes, f) == (size_t)nbytes,
          "Writing pixels failed");
  }

  // Cleanup
  fclose(f);

  return 0;
}

/// PPM file operations --- For RGB images

/// Load a raw PPM file.
/// Only ASCII PPM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageLoadPPM(const char* filename) {
  assert(filename != NULL);
  int w, h;
  int levels;
  char c;
  FILE* f = NULL;

  check((f = fopen(filename, "rb")) != NULL, "Open failed");
  // Parse PPM header
  check(fscanf(f, "P%c ", &c) == 1 && c == '3', "Invalid file format");
  skipComments(f);
  check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width");
  skipComments(f);
  check(fscanf(f, "%d", &h) == 1 && h >= 0, "Invalid height");
  skipComments(f);
  check(fscanf(f, "%d", &levels) == 1 && 0 <= levels && levels <= 255,
        "Invalid depth");
  check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected");

  // Allocate image
  Image img = ImageCreate((uint32)w, (uint32)h);

  // Read pixels
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      int r, g, b;
      check(fscanf(f, "%d %d %d", &r, &g, &b) == 3 && 0 <= r && r <= levels &&
                0 <= g && g <= levels && 0 <= b && b <= levels,
            "Invalid pixel color");
      rgb_t color = r << 16 | g << 8 | b;
      uint16 index = LUTAllocColor(img, color);
      img->image[i][j] = index;
      // printf("[%u][%u]: (%d,%d,%d) -> %u (%6x)\n", i, j, r,g,b, index,
      // color);
    }
    fprintf(f, "\n");
  }

  fclose(f);
  return img;
}

/// Save image to PPM file.
/// On success, returns nonzero.
/// On failure, a partial and invalid file may be left in the system.
int ImageSavePPM(const Image img, const char* filename) {
  assert(img != NULL);

  int w = (int)img->width;
  int h = (int)img->height;
  FILE* f = NULL;

  check((f = fopen(filename, "wb")) != NULL, "Open failed");
  check(fprintf(f, "P3\n%d %d\n255\n", w, h) > 0, "Writing header failed");

  // The pixel RGB values
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      uint16 index = img->image[i][j];
      rgb_t color = img->LUT[index];
      int r = color >> 16 & 0xff;
      int g = color >> 8 & 0xff;
      int b = color & 0xff;
      fprintf(f, "  %3d %3d %3d", r, g, b);
    }
    fprintf(f, "\n");
  }

  // Cleanup
  fclose(f);

  return 0;
}

/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
uint32 ImageWidth(const Image img) {
  assert(img != NULL);
  return img->width;
}

/// Get image height
uint32 ImageHeight(const Image img) {
  assert(img != NULL);
  return img->height;
}

/// Get number of image colors
uint16 ImageColors(const Image img) {
  assert(img != NULL);
  return img->num_colors;
}

/// Image comparison

/// These functions do not modify the images and never fail.

/// Check if img1 and img2 represent equal images.
/// NOTE: The same rgb color may correspond to different LUT labels in
/// different images!
int ImageIsEqual(const Image img1, const Image img2) {              //completar
  assert(img1 != NULL);
  assert(img2 != NULL);

  // Número de comparações realizadas pixel a pixel.
  int comp = 0;

  // Se a altura ou a largura de uma imagem for diferente à outra,
  // então as imagens não são iguais (return 0).
  if (img1->height != img2->height || img1->width != img2->width) {
    printf("Número de comparações: %d\n", comp);      //apagar
    return 0;
  }

  // Se o número de cores de duas imagens forem diferentes,
  // então as imagens não são iguais (return 0).
  if (img1->num_colors != img2->num_colors) {
    printf("Número de comparações: %d\n", comp);      //apagar
    return 0;
  }

  // Percorrer sobre todas as linhas e colunas da imagem1.
  // Se a cor do pixel da imagem1 for diferente ao da imagem2 (nas mesmas posições),
  // então não são imagens iguais (return 0).
  for (uint32 i = 0; i < img1->height; i++) {
    for (uint32 j = 0; j < img1->width; j++) {
      comp ++;                                                                       // Incrementa 1 a cada comparação.
      if (img1->LUT[img1->image[i][j]] != img2->LUT[img2->image[i][j]]){                  //  Compara as cores pixel a pixel.
        return 0;             
      }
    }
  }
  printf("Número de comparações: %d\n", comp);      //apagar
  return 1;                                                                              // Retorna 1 se as imagens forem iguais.
}

int ImageIsDifferent(const Image img1, const Image img2) {
  assert(img1 != NULL);
  assert(img2 != NULL);

  return !ImageIsEqual(img1, img2);
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)

/// Rotate 90 degrees clockwise (CW).
/// Returns a rotated version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageRotate90CW(const Image img) {                            //completar
  assert(img != NULL);

  // Criar uma nova imagem com as dimensões invertidas(linha passa a coluna e coluna passa a linha).
  Image img90CW = AllocateImageHeader(img->height, img->width);

  // Copiar o número de cores utilizadas na LUT (Look-Up Table) para a imagem rodada.
  img90CW->num_colors = img->num_colors;

  // Copiar a LUT da imagem original para a imagem rodada 90CW.
  // Como as cores estão no formato LUT, temos de multiplicar pelo tamanho de LUT.
  memcpy(img90CW->LUT, img->LUT, img->num_colors * sizeof(rgb_t));

  // Alocar a linha que vai usar.
  for (uint32 i = 0; i < img->width; i++)
        img90CW->image[i] = AllocateRowArray(img90CW->width);

  // O pixel da img(i, j) passa a ser img90CW(j, imgHeight - 1 - i).
  // A primeira linha passa a ser a última coluna.
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      img90CW->image[j][(img->height) - 1 - i] = img->image[i][j];
    }
  }

  return img90CW;                   // Retorna a imagem rodada 90 graus.
}

/// Rotate 180 degrees clockwise (CW).
/// Returns a rotated version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageRotate180CW(const Image img) {                           //completar
  assert(img != NULL);

  // Criar uma nova imagem com as mesmas dimensões da imagem original.
  Image img180CW = AllocateImageHeader(img->width, img->height);

  // Igualar o número de cores.
  img180CW->num_colors = img->num_colors;

  // Copiar a LUT da imagem original para a imagem rodada 180CW.
  // Como as cores estão no formato LUT, temos de multiplicar pelo tamanho de LUT.
  memcpy(img180CW->LUT, img->LUT, img->num_colors * sizeof(rgb_t));

  // Alocar a linha que vai usar.
  for (uint32 i = 0; i < img180CW->height; i++)
        img180CW->image[i] = AllocateRowArray(img180CW->width);

  // O pixel da img(i, j) passa a ser img180CW(imgHeight - 1 - i, imgWidth - 1 - j)
  for (uint32 i = 0; i < img->height; i++) {
    for (uint32 j = 0; j < img->width; j++) {
      img180CW->image[(img->height) - 1 - i][(img->width) - 1 - j] = img->image[i][j];
    }
  }
  return img180CW;                  // Retorna a imagem rodada 180 graus.                                                     
}

/// Check whether pixel coords (u, v) are inside img.
/// ATTENTION
///   u : column index
///   v : row index
int ImageIsValidPixel(const Image img, int u, int v) {
  return 0 <= u && u < (int)img->width && 0 <= v && v < (int)img->height;
}

/// Region Growing

/// The following three *RegionFilling* functions perform region growing
/// using some variation of the 4-neighbors flood-filling algorithm:
///   Given the coordinates (u, v) of a seed pixel,
///   fill all similarly-colored adjacent pixels with a new color label.
///
/// All of these functions receive the same arguments:
///   img: The image to operate on (and modify).
///   u, v: the coordinates of the seed pixel.
///   label: the new color label (LUT index) to fill the region with.
///
/// And return: the number of labeled pixels.

/// Each function carries out a different version of the algorithm.

/// Region growing using the recursive flood-filling algorithm.

InstrReset();  // zera todos os contadores, incluindo PIXMEM

int ImageRegionFillingRecursive(Image img, int u, int v, uint16 label) {
  assert(img != NULL);
  assert(ImageIsValidPixel(img, u, v));
  assert(label < FIXED_LUT_SIZE);

  // Guardar a cor do pixel atual da imagem em original_color.
  uint16 original_color = img->image[v][u];
  
  // Se a cor do pixel atual (original_color) for igual à que pretendemos mudar (label),
  // não altera a cor (return 0).
  if (original_color == label) {
    return 0;
  }
  
  // Mudar a cor do pixel atual para a cor pretendida (label).
  img->image[v][u] = label;
  PIXMEM++;                        // Incrementar o contador de acessos à memória de pixels.
  int count = 1;                    // Incrementa 1 ao número de pixels alterados (labeled pixels).
  
  // Percurrer os 4 pixels vizinhos (direita, baixo, cima, esquerda).

  // Se o pixel for valido, ou seja, estiver dentro do limite da imagem
  // e se a cor do pixel for igual à cor que queremos alterar (original_color),
  // então muda a cor para a cor pretendida (label) e incrementa 1 ao número de pixels alterados.

  // Deslocar para a direita (u+1, v).
  if (ImageIsValidPixel(img, u + 1, v) && img->image[v][u + 1] == original_color) {
    PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.  
    count += ImageRegionFillingRecursive(img, u + 1, v, label);
  }
  
  // Deslocar para baixo (u, v+1).
  if (ImageIsValidPixel(img, u, v + 1) && img->image[v + 1][u] == original_color) {
    PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.  
    count += ImageRegionFillingRecursive(img, u, v + 1, label);
  }
  
  // Deslocar para cima (u, v-1).
  if (ImageIsValidPixel(img, u, v - 1) && img->image[v - 1][u] == original_color) {
    PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.
    count += ImageRegionFillingRecursive(img, u, v - 1, label);
  }

  // Deslocar para a esquerda (u-1, v).
  if (ImageIsValidPixel(img, u - 1, v) && img->image[v][u - 1] == original_color) {
    PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.
    count += ImageRegionFillingRecursive(img, u - 1, v, label);
  }
  
  return count;                     // Returnar o número de pixels alterados.
}

/// Region growing using a STACK of pixel coordinates to
/// implement the flood-filling algorithm.

InstrReset();  // zera todos os contadores, incluindo PIXMEM

int ImageRegionFillingWithSTACK(Image img, int u, int v, uint16 label) {
  assert(img != NULL);
  assert(ImageIsValidPixel(img, u, v));
  assert(label < FIXED_LUT_SIZE);

  // Guardar a cor do pixel atual da imagem em original_color.
  uint16 original_color = img->image[v][u];
  
  // Se a cor do pixel atual (original_color) for igual à que pretendemos mudar(label),
  // não altera a cor (return 0).
  if (original_color == label) {
    return 0;
  }
  
  // Criar um stack vazio para guardar as coordenadas dos pixels com um tamanho inicial de 100 pixels.
  Stack* stack = StackCreate(100);
  
  // Contar os pixels alterados.
  int count = 0;
  
  // Adicionar o pixel inicial ao stack (stack push).
  StackPush(stack, PixelCoordsCreate(u, v));
  
  // Remover o pixel do topo do stack enquanto não estiver vazio.
  while (!StackIsEmpty(stack)) {
    PixelCoords current = StackPop(stack);            // Remove o pixel.
    int cu = PixelCoordsGetU(current);                // Obtem a coordenada u do pixel a remover (cu = current u (coluna)).
    int cv = PixelCoordsGetV(current);                // Obtem a coordenada v do pixel a remover (cv = current v (linha)).
    
    // Se o pixel não for valido, ou seja, se não estiver dentro do limite da imagem 
    // ou se o pixel atual não tem a cor do pixel original (original_color),
    // então o pixel é ignorado (continue), ou seja, não é alterado e passa para o próximo pixel do stack.
    if (!ImageIsValidPixel(img, cu, cv) || img->image[cv][cu] != original_color) {
      PIXMEM++;                        // Incrementar o contador de acessos à memória de pixels.
      continue;
    }
    
    // Mudar a cor do pixel atual para a cor pretendida (label).
    img->image[cv][cu] = label;
    PIXMEM++;                        // Incrementar o contador de acessos à memória de pixels.
    count++;                                          // Incrementar 1 ao número de pixels alterados (labeld pixels).
    
    // Percurrer os 4 pixels vizinhos (direita, baixo, cima, esquerda).
    // Se o pixel for valido, adiciona o pixel atual ao topo do stack.
    
    // Deslocar para a direita (u+1, v).
    if (ImageIsValidPixel(img, cu + 1, cv)) {
      StackPush(stack, PixelCoordsCreate(cu + 1, cv));
    }
    
    // Deslocar para baixo (u, v+1).
    if (ImageIsValidPixel(img, cu, cv + 1)) {
      StackPush(stack, PixelCoordsCreate(cu, cv + 1));
    }
    
    // Deslocar para cima (u, v-1).
    if (ImageIsValidPixel(img, cu, cv - 1)) {
      StackPush(stack, PixelCoordsCreate(cu, cv - 1));
    }

    // Deslocar para a esquerda (u-1, v).
    if (ImageIsValidPixel(img, cu - 1, cv)) {
      StackPush(stack, PixelCoordsCreate(cu - 1, cv));
    }
  }
  
  // Destruir o stack.
  StackDestroy(&stack);
  
  return count;                // Retornar o número de pixels alterados.
}

/// Region growing using a QUEUE of pixel coordinates to
/// implement the flood-filling algorithm.

InstrReset();  // zera todos os contadores, incluindo PIXMEM

int ImageRegionFillingWithQUEUE(Image img, int u, int v, uint16 label) {
  assert(img != NULL);
  assert(ImageIsValidPixel(img, u, v));
  assert(label < FIXED_LUT_SIZE);

  // Guardar a cor do pixel atual da imagem em original_color.
  uint16 original_color = img->image[v][u];
  
  // Se a cor do pixel atual (original_color) for igual à que pretendemos mudar (label),
  // não altera a cor (return 0).
  if (original_color == label) {
    return 0;
  }
  
  // Criar uma queue vazia para guardar as coordenadas dos pixels com um tamanho inicial igual ao tamanho da imagem.
  Queue* queue = QueueCreate(img->width * img->height);
  if (queue == NULL) {
    return 0;
  }

  // Adicionar o pixel inicial à fila.
  PixelCoords start = {u, v};
  QueueEnqueue(queue, start);

  // Mudar a cor do pixel atual para a cor pretendida (label).
  img->image[v][u] = label;
  int count = 1;                                    // Incrementar 1 ao número de pixels alterados (labeld pixels).

  // Remover o pixel do início da queue enquanto não estiver vazia.
  while (!QueueIsEmpty(queue)) {
    PixelCoords curr = QueueDequeue(queue);         // Remove o pixel.
    int curr_u = curr.u;                            // Obtem a coordenada u do pixel a remover (curr_u = current u (coluna)).
    int curr_v = curr.v;                            // Obtem a coordenada v do pixel a remover (curr_v = current v (linha)).


    // Percurrer os 4 pixels vizinhos (direita, baixo, cima, esquerda).

    // Se o pixel for valido, ou seja, estiver dentro do limite da imagem
    // e se a cor do pixel for igual à cor que queremos alterar (original_color),
    // então muda a cor para a cor pretendida (label) e adiciona o pixel no fim da fila
    // e incrementa 1 ao número de pixels alterados.

    // Verificar e adicionar o vizinho da direita (u+1, v).
    if (ImageIsValidPixel(img, curr_u + 1, curr_v) && img->image[curr_v][curr_u + 1] == original_color) {
      PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.
      img->image[curr_v][curr_u + 1] = label;
      PixelCoords next = {curr_u + 1, curr_v};
      QueueEnqueue(queue, next);
      count++;
    }
      
    // Verificar e adicionar o vizinho de baixo (u, v+1).
    if (ImageIsValidPixel(img, curr_u, curr_v + 1) && img->image[curr_v + 1][curr_u] == original_color) {
      PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.
      img->image[curr_v + 1][curr_u] = label;
      PixelCoords next = {curr_u, curr_v + 1};
      QueueEnqueue(queue, next);
      count++;
    }
    
    // Verificar e adicionar o vizinho de cima (u, v-1).
    if (ImageIsValidPixel(img, curr_u, curr_v - 1) && img->image[curr_v - 1][curr_u] == original_color) {
      PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.
      img->image[curr_v - 1][curr_u] = label;
      PixelCoords next = {curr_u, curr_v - 1};
      QueueEnqueue(queue, next);
      count++;
    }

    // Verificar e adicionar o vizinho da esquerda (u-1, v).
    if (ImageIsValidPixel(img, curr_u - 1, curr_v) && img->image[curr_v][curr_u - 1] == original_color) {
      PIXMEM++;                    // Incrementar o contador de acessos à memória de pixels.
      img->image[curr_v][curr_u - 1] = label;
      PixelCoords next = {curr_u - 1, curr_v};
      QueueEnqueue(queue, next);
      count++;
    }
  }
  
  QueueDestroy(&queue);                       // Destruir a queue.

  return count;                             // Retorna o número de pixels alterados.
}

/// Image Segmentation

/// Label each WHITE region with a different color.
/// - WHITE (the background color) has label (LUT index) 0.
/// - Use GenerateNextColor to create the RGB color for each new region.
///
/// One of the region filling functions above is passed as the
/// last argument, using a function pointer.
///
/// Returns the number of image regions found.
int ImageSegmentation(Image img, FillingFunction fillFunct) {
  assert(img != NULL);
  assert(fillFunct != NULL);

  int region_count = 0;            // Contador para as regiões encontradas.
  rgb_t current_color = 0x000000;  // Começar com uma cor base (preto).

  // Percorrer todos os pixels da imagem.
  for (uint32 v = 0; v < img->height; v++) {
    for (uint32 u = 0; u < img->width; u++) {
      
      // Se encontrar um pixel do background (WHITE).
      if (img->image[v][u] == WHITE) {
        
        // Gerar uma cor nova para a região.
        current_color = GenerateNextColor(current_color);
        
        // Usar LUTAllocColor para obter um label para esta cor.
        // Como é função é static verifica se a cor já existe na LUT.
        int label = -1;
        for (uint16 i = 0; i < img->num_colors; i++) {
          if (img->LUT[i] == current_color) {
            label = i;
            break;
          }
        }
        
        // Se a cor não existir na LUT.
        if (label == -1) {    
          // Se a LUT estiver cheia, reutiliza cores já existentes.  
          if (img->num_colors >= FIXED_LUT_SIZE) {
            label = (region_count % (img->num_colors - 2)) + 2;
          // Senão, adicionar a nova cor à LUT.
          } else {
            label = img->num_colors;
            img->LUT[label] = current_color;
            img->num_colors++;
          }
        }

        // Preencher a região usando uma função anterior.
        int pixels_filled = fillFunct(img, u, v, label);
        
        if (pixels_filled > 0) {
          region_count++;                 // Incrementa o número de regiões encontradas.
          printf("Região %d: %d pixels preenchidos com cor 0x%06x (label %d)\n", region_count, pixels_filled, current_color, label);
        }
      }
    }
  }
  
  return region_count;                    // Retorna o número de regiões encontradas.
}
