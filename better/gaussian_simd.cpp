#include <cstdint>
#include <stdio.h>
#include <string>
#include <cmath>
#include <immintrin.h>
#include "BmpReader.h"

int main(int argc, char const *argv[]) {
  BmpReader bmpReader;
  int width, height, color;
  uint32_t *kern;
  FILE *fgauss = fopen("mask_Gaussian.txt", "r");
  int kwidth, kheight;
  uint32_t scale = 0;
  if (!fgauss) {
    printf("Fail to read Gaussian\n");
    return 1;
  }
  char line[100] = "";
  fgets(line, 100, fgauss);
  if (sscanf(line, "%d %d", &kwidth, &kheight) == 1) {
    kheight = sqrt(kwidth);
    kwidth = kheight;
  }
  kern = new uint32_t[kwidth * kheight];
  for (int i = 0; i < kwidth * kheight; i++) {
    fscanf(fgauss, "%d", &kern[i]);
    scale += kern[i];
  }
  fclose(fgauss);
  
  if (argc < 2) {
    printf("usage: ./gauss <bmp file>\n");
    return 1;
  }
  
  for (int i = 1; i < argc; i++) {
    uint8_t *bmp = bmpReader.ReadBMP(argv[i], &width, &height);
    if (!bmp) {
      printf("error reading %s!\n", argv[i]);
      continue;
    }
    int color_stride = (height + kheight) * (width + kwidth);
    uint8_t *padded = new uint8_t[color_stride];
    uint32_t *out = new uint32_t[width * height];
    __m256i *tmp = (__m256i*)_mm_malloc(sizeof(__m256i) * kwidth, sizeof(__m256i));
    for (int ch = 0; ch < 3; ch++) {
      printf("ch = %d\n", ch);
      for (int i = 0; i < height+kheight; i++) {
        for (int j = 0; j < width+kwidth; j++) {
          int myidx = i * (width+kwidth) + j;
          if (i < kheight/2 || i >= height+kheight/2 || j < kwidth/2 || j >= width+kwidth/2) {
            padded[myidx] = 0;
          }
          else {
            int idx = (i-kheight/2) * width + (j-kwidth/2);
            padded[myidx] = bmp[idx*3 + ch];
          }
        }
      }
      for (int i = 0; i < width * height; i++) {
        out[i] = 0;
      }
      for (int ky = 0; ky < kheight; ky++) {
        for (int kx = 0; kx < kwidth; kx++) {
          tmp[kx] = _mm256_set1_epi32(kern[ky * kwidth + kx]);
        }
        for (int y = 0; y < height; y++) {
          int x = 0;
          for (x = 0; x <= width-16; x+=8) {
            __m256i accum = _mm256_loadu_si256((__m256i*) &out[y * width + x]);
            __m256i in, in2;
            __m256i w, w2;
            // loop unroll by 2
            for (int kx = 0; kx <= kwidth-2; kx+=2) {
              in = _mm256_cvtepu8_epi32(_mm_loadu_si128((__m128i*) &padded[(y+ky)*(width+kwidth) + x+kx]));
              w = _mm256_load_si256(&tmp[kx]);
              accum = _mm256_add_epi32(accum, _mm256_mullo_epi32(w, in));
              in2 = _mm256_cvtepu8_epi32(_mm_loadu_si128((__m128i*) &padded[(y+ky)*(width+kwidth) + x+kx+1]));
              w2 = _mm256_load_si256(&tmp[kx+1]);
              accum = _mm256_add_epi32(accum, _mm256_mullo_epi32(w2, in2));
            }
            // remaining kx
            if (kwidth&1) {
              int kx = kwidth-1;
              in = _mm256_cvtepu8_epi32(_mm_loadu_si128((__m128i*) &padded[(y+ky)*(width+kwidth) + x+kx]));
              w = _mm256_load_si256(&tmp[kx]);
              accum = _mm256_add_epi32(accum, _mm256_mullo_epi32(w, in));
            }
            _mm256_storeu_si256((__m256i*) &out[y * width + x], accum);
          }
          // last part
          for (x = x; x < width; x++) {
            uint32_t accum = out[y * width + x];
            for (int kx = 0; kx < kwidth; kx++) {
              uint32_t in = padded[(y+ky)*(width+kwidth) + x+kx];
              uint32_t w = kern[ky * kwidth + kx];
              accum = accum + w * in;
            }
            out[y * width + x] = accum;
          }
        }
      }
      for (int i = 0; i < width * height; i++) {
        bmp[i*3 + ch] = out[i] / scale;
      }
    }
    _mm_free(tmp);
    delete[] out;
    delete[] padded;
    
    std::string out_name = argv[i];
    out_name = out_name.substr(0, out_name.size() - 4) + "_blur_avx2.bmp";
    if (bmpReader.WriteBMP(out_name.c_str(), width, height, bmp)) {
      printf("error writing %s!\n", out_name.c_str());
    }
    delete[] bmp;
  }
  return 0;
}
