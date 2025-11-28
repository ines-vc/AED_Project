// imageRGBTest - A program that performs some operations on RGB images.
//
// This program is an example use of the imageRGB module,
// a programming project for the course AED, DETI / UA.PT
//
// You may freely use and modify this code, NO WARRANTY, blah blah,
// as long as you give proper credit to the original and subsequent authors.
//
// The AED Team <jmadeira@ua.pt, jmr@ua.pt, ...>
// 2025

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "imageRGB.h"
#include "instrumentation.h"

#define PIXMEM InstrCount[0]

void test_RegionFilling_performance() {
  printf("\n=== TESTE DE DESEMPENHO: Region Filling Functions ===\n");

  // Criar diferentes tamanhos de imagens para teste
  const uint32 sizes[] = {50, 80, 100};
  const int num_sizes = 3;

  for (int s = 0; s < num_sizes; s++) {
    uint32 size = sizes[s];
    printf("\n--- %dx%d ---\n", size, size);

    //Teste 1: ImageRegionFillingRecursive
    printf("\n1) ImageRegionFillingRecursive\n");
    PIXMEM = 0;  // Zera o contador de acessos à memória de pixels.
    Image img1 = ImageCreate(size, size);
    int pixels1 = ImageRegionFillingRecursive(img1, 0, 0, BLACK);
    printf("Pixels preenchidos: %d\n", pixels1);
    printf("PIXMEM (acessos a pixels): %llu\n", PIXMEM);
    ImageDestroy(&img1);

    // Teste 2: ImageRegionFillingWithSTACK
    printf("\n2) ImageRegionFillingWithSTACK\n");
    PIXMEM = 0;  // Zera o contador de acessos à memória de pixels.
    Image img2 = ImageCreate(size, size);
    int pixels2 = ImageRegionFillingWithSTACK(img2, 0, 0, BLACK);
    printf("Pixels preenchidos: %d\n", pixels2);
    printf("PIXMEM (acessos a pixels): %llu\n", PIXMEM);
    ImageDestroy(&img2);

    // Teste 3: ImageRegionFillingWithQUEUE
    printf("\n3) ImageRegionFillingWithQUEUE\n");
    PIXMEM = 0;  // Zera o contador de acessos à memória de pixels.
    Image img3 = ImageCreate(size, size);
    int pixels3 = ImageRegionFillingWithQUEUE(img3, 0, 0, BLACK);
    printf("Pixels preenchidos: %d\n", pixels3);
    printf("PIXMEM (acessos a pixels): %llu\n", PIXMEM);
    ImageDestroy(&img3);

    printf("\n" "========================================\n");
  }

  // Teste de Segmentação
  printf("\n--- Teste de ImageSegmentation ---\n");
    
  Image seg_img = ImageCreateChess(80, 80, 20, 0x000000);
    
  printf("\n1) Segmentacao com Recursive\n");
  Image seg1 = ImageCopy(seg_img);
  int regions1 = ImageSegmentation(seg1, ImageRegionFillingRecursive);
  printf("Regioes encontradas: %d\n", regions1);
  ImageSavePPM(seg1, "segment_recursive_test.ppm");
  ImageDestroy(&seg1);

  printf("\n2) Segmentacao com Stack\n");
  Image seg2 = ImageCopy(seg_img);
  int regions2 = ImageSegmentation(seg2, ImageRegionFillingWithSTACK);
  printf("Regioes encontradas: %d\n", regions2);
  ImageSavePPM(seg2, "segment_stack_test.ppm");
  ImageDestroy(&seg2);

  printf("\n3) Segmentacao com Queue\n");
  Image seg3 = ImageCopy(seg_img);  InstrReset();
  int regions3 = ImageSegmentation(seg3, ImageRegionFillingWithQUEUE);
  printf("Regioes encontradas: %d\n", regions3);
  ImageSavePPM(seg3, "segment_queue_test.ppm");
  ImageDestroy(&seg3);

  ImageDestroy(&seg_img);

  printf("\n=== FIM DOS TESTES DE DESEMPENHO ===\n");
}

int main(int argc, char* argv[]) {
  program_name = argv[0];
  if (argc != 1) {
    error(1, 0, "Usage: imageRGBTest");
  }

  ImageInit();

  // Creating and displaying some images

  printf("1) ImageCreate\n");
  Image white_image = ImageCreate(100, 100);
  // ImageRAWPrint(white_image);

  printf("2) ImageCreateChess(black)+ ImageSavePBM\n");
  Image image_chess_1 = ImageCreateChess(150, 120, 30, 0x000000);  // black
  // ImageRAWPrint(image_chess_1);
  ImageSavePBM(image_chess_1, "chess_image_1.pbm");

  printf("3) ImageCreateChess(red) + ImageSavePPM\n");
  Image image_chess_2 = ImageCreateChess(20, 20, 8, 0xff0000);  // red
  ImageRAWPrint(image_chess_2);
  ImageSavePPM(image_chess_2, "chess_image_2.ppm");

  printf("4) ImageCreateChess(all black)\n");
  Image black_image = ImageCreateChess(100, 100, 100, 0x000000);  // all black
  // ImageRAWPrint(black_image);
  ImageSavePBM(black_image, "black_image.pbm");

  printf("5) ImageCopy\n");
  Image copy_image = ImageCopy(image_chess_1);
  // ImageRAWPrint(copy_image);
  if (copy_image != NULL) {
    ImageSavePBM(copy_image, "copy_image.pbm");
  }

  printf("6) ImageLoadPBM\n");
  Image image_1 = ImageLoadPBM("img/feep.pbm");
  ImageRAWPrint(image_1);

  printf("7) ImageLoadPPM\n");
  Image image_2 = ImageLoadPPM("img/feep.ppm");
  ImageRAWPrint(image_2);

  printf("8) ImageCreatePalete\n");
  Image image_3 = ImageCreatePalete(4 * 32, 4 * 32, 4);
  ImageSavePPM(image_3, "palete.ppm");

  printf("9) ImageIsEqual\n");
  // Criar diferentes tamanhos de imagens para teste
  const uint32 sizes[] = {100, 1000, 2000};
  const int num_sizes = 3;
  for (int s = 0; s < num_sizes; s++) {
    uint32 size = sizes[s];
    printf("\n--- Testando com imagem %dx%d ---\n", size, size);
    Image img1 = ImageCreate(size, size);
    Image img2 = ImageCreate(size, size);
    int result = ImageIsEqual(img1, img2);
    printf("Resultado = %d\n", result);
  }
  //Image image_4 = ImageLoadPBM("img/feep.pbm");
  //Image image_5 = ImageLoadPPM("chess_image_2.ppm");
  //int result = ImageIsEqual(image_4, image_5);
  //printf("Resultado = %d\n", result);

  printf("10) Image90CW\n");
  Image image_6 = ImageLoadPBM("img/feep.pbm");
  Image result90CW = ImageRotate90CW(image_6);
  ImageSavePBM(result90CW, "feep90CW.pbm");

  printf("11) Image180CW\n");
  Image image_7 = ImageLoadPBM("img/feep.pbm");
  Image result180CW = ImageRotate180CW(image_7);
  ImageSavePBM(result180CW, "feep180CW.pbm");

  printf("12) ImageRegionFillingRecursive\n");
  Image image_8 = ImageLoadPBM("img/feep.pbm");
  ImageRAWPrint(image_8);
  printf("ANTES:\n");
  ImageRAWPrint(image_8);
  // Preencher região WHITE começando em (2, 2) com BLACK
  int pixels = ImageRegionFillingRecursive(image_8, 0, 0, BLACK);
  printf("Pixels preenchidos (Recursive): %d\n", pixels);
  printf("DEPOIS:\n");
  ImageRAWPrint(image_8);
  ImageSavePBM(image_8, "feep_recursive.pbm");

  printf("\n13) ImageRegionFillingWithSTACK\n");
  Image image_9 = ImageLoadPBM("img/feep.pbm");
  printf("ANTES:\n");
  ImageRAWPrint(image_9);
  // Preencher região WHITE começando em (0, 0) com BLACK
  int pixels_stack = ImageRegionFillingWithSTACK(image_9, 0, 0, BLACK);
  printf("Pixels preenchidos (STACK): %d\n", pixels_stack); 
  printf("DEPOIS:\n");
  ImageRAWPrint(image_9);
  ImageSavePBM(image_9, "feep_stack.pbm");

  printf("\n14) ImageRegionFillingWithQUEUE\n");
  Image image_10 = ImageLoadPBM("img/feep.pbm");
  printf("ANTES:\n");
  ImageRAWPrint(image_10);
  // Preencher região WHITE começando em (0, 0) com BLACK
  int pixels_queue = ImageRegionFillingWithQUEUE(image_10, 0, 0, BLACK);  
  printf("Pixels preenchidos (QUEUE): %d\n", pixels_queue);
  printf("DEPOIS:\n");
  ImageRAWPrint(image_10);
  ImageSavePBM(image_10, "feep_queue.pbm");

  printf("\n15) ImageSegmentation\n");
  Image image_11 = ImageLoadPPM("img/feep.ppm");
  printf("ANTES:\n");
  ImageRAWPrint(image_11);
  // Preencher região WHITE começando em (0, 0) com BLACK
  int pixels_segment = ImageSegmentation(image_11, ImageRegionFillingWithQUEUE);  
  printf("Pixels preenchidos: %d\n", pixels_segment);
  printf("DEPOIS:\n");
  ImageRAWPrint(image_11);
  ImageSavePPM(image_11, "feep_segment.ppm");

  // Teste de desempenho das funções de preenchimento de região
  test_RegionFilling_performance();

  ImageDestroy(&white_image);
  ImageDestroy(&black_image);
  if (copy_image != NULL) {
    ImageDestroy(&copy_image);
  }
  ImageDestroy(&image_chess_1);
  ImageDestroy(&image_chess_2);
  ImageDestroy(&image_1);
  ImageDestroy(&image_2);
  ImageDestroy(&image_3);
  ImageDestroy(&image_6);
  ImageDestroy(&image_7);
  ImageDestroy(&image_8);
  ImageDestroy(&image_9);
  ImageDestroy(&image_10);
  ImageDestroy(&image_11);

  return 0;
}
