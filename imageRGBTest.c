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

void test_ImageIsEqual_performance() {
    const uint32 WIDTH = 500;
    const uint32 HEIGHT = 500;

    printf("\n--- Teste de desempenho: ImageIsEqual ---\n");

    // Criar imagens para teste
    printf("6) ImageLoadPBM\n");
    Image img1 = ImageLoadPPM("img/feep.ppm");
    
    

    printf("7) ImageLoadPPM\n");
    Image img2 = ImageLoadPPM("img/feep.ppm");
    


    Image img3 = ImageCreate(WIDTH, HEIGHT);
  
    // Nomear o contador de pixels comparados
    InstrName[0] = "Pixel_Comparisons";

    // --- Teste 1: Imagens idênticas ---
    printf("\nTeste 1 (idênticas)\n");
    InstrReset();
    int equal1 = ImageIsEqual(img1, img2);
    InstrPrint();
    printf("Resultado: %s\n", equal1 ? "Igual" : "Diferente");

    // --- Teste 2: Imagens diferentes ---
    printf("\nTeste 2 (diferentes)\n");
    InstrReset();
    int equal2 = ImageIsEqual(img1, img3);
    InstrPrint();
    printf("Resultado: %s\n", equal2 ? "Igual" : "Diferente");

    // Limpeza de memória
    ImageDestroy(&img1);
    ImageDestroy(&img2);
    ImageDestroy(&img3);
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
  //ImageRAWPrint(copy_image);
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
  Image image_4 = ImageLoadPPM("img/feep.ppm");
  Image image_5 = ImageLoadPBM("img/feep.pbm");
  int result = ImageIsEqual(image_4, image_5);
  printf("Resultado = %d\n", result);

  printf("10) Image90CW\n");
  Image image_6 = ImageLoadPBM("img/feep.pbm");
  Image result90CW = ImageRotate90CW(image_6);
  ImageSavePBM(result90CW, "feep90CW.pbm");

  printf("11) Image180CW\n");
  Image image_7 = ImageLoadPBM("img/feep.pbm");
  Image result180CW = ImageRotate180CW(image_7);
  ImageSavePBM(result180CW, "feep180CW.pbm");

  printf("12) ImageRegionFillingRecursive - Teste básico\n");
  Image image_8 = ImageLoadPBM("img/feep.pbm");
  ImageRAWPrint(image_8);
  // Preencher região WHITE começando em (2, 2) com BLACK
  int pixels = ImageRegionFillingRecursive(image_8, 2, 2, BLACK);  
  printf("Pixels preenchidos: %d\n", pixels);
  ImageRAWPrint(image_8);
  ImageSavePBM(image_8, "feep_recursive.pbm");

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
  ImageDestroy(&image_4);
  ImageDestroy(&image_5);
  ImageDestroy(&image_6);
  ImageDestroy(&image_7);
  ImageDestroy(&image_8);

  test_ImageIsEqual_performance();

  return 0;
}
