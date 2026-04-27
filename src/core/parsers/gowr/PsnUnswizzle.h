#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>

namespace GOW {

inline int Morton(int t, int sx, int sy) {
    int num = 0;
    int num2 = 0;
    int num3 = 1;
    int num4 = 1;
    int num5 = t;
    int num6 = sx;
    int num7 = sy;
    
    while (num6 > 1 || num7 > 1) {
        if (num6 > 1) {
            num += num4 * (num5 & 1);
            num5 >>= 1;
            num4 *= 2;
            num6 >>= 1;
        }
        if (num7 > 1) {
            num2 += num3 * (num5 & 1);
            num5 >>= 1;
            num3 *= 2;
            num7 >>= 1;
        }
    }
    return num2 * sx + num;
}

inline void UnSwizzle(const uint8_t* src, size_t srcLen, uint8_t* dst, uint32_t w, uint32_t h, uint32_t bpp, uint32_t pixbl) {
    size_t minSize = pixbl * pixbl * bpp / 8;
    uint32_t totalBytes = w * h * bpp / 8;
    if (totalBytes <= minSize) {
        if (srcLen >= minSize) std::memcpy(dst, src, minSize);
        return;
    }

    uint32_t num4 = pixbl; 
    uint32_t num5 = bpp * 2;
    if (num4 == 1) {
        num5 = bpp / 8;
    }

    int num6 = h / num4;
    int num7 = w / num4;
    size_t roff = 0;

    for (int i = 0; i < (num6 + 7) / 8; i++) {
        for (int j = 0; j < (num7 + 7) / 8; j++) {
            for (int k = 0; k < 64; k++) {
                int num8 = Morton(k, 8, 8);
                int num9 = num8 / 8;
                int num10 = num8 % 8;
                
                if (j * 8 + num10 < num7 && i * 8 + num9 < num6) {
                    size_t destIdx = num5 * ((i * 8 + num9) * num7 + j * 8 + num10);
                    if (roff + num5 <= srcLen) {
                        std::memcpy(dst + destIdx, src + roff, num5);
                    }
                }
                roff += num5;
            }
        }
    }
}

} // namespace GOW
