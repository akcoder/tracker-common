#pragma once
// Baseline JPEG from memory into RGB565, with the ROM's TJpgDec, shared by the trackers
// (merged from sky-tracker's sky_jpg.h, plane-tracker's plane_jpg.h and f1-tracker's f1_jpg.h).
//
//   decode_half  half size, every 2x2 averaged (a portrait, the Moon): centred and cropped
//   decode_full  1:1, centred and cropped (a card's portrait)
//   decode_fit   any size down to tw x th by a box filter (a satellite picture), with a
//                window (`keep`, `centre`) and a destination stride
//   decode_copy  1:1 into part of a larger picture (pictures stored at their display size)
//
// TJpgDec hands over MCU blocks (8 or 16 px, even-sized for even image sizes), so every 2x2
// square lies inside one block and is averaged there: no work buffer. The device uses the
// ESP32-S3 ROM's TJpgDec (R0.01, RGB888 out); the host tests build ChaN's later release
// (JD_FORMAT 0). Its own descaling is used only by decode_fit.
//
// Progress: `rows` is told how many output rows are finished, top down, at the end of each row
// of blocks; `cancel`, once true, ends the decode at the next read; `have`/`eof` decode while
// the file is still arriving (the reader waits for the bytes it needs; `jpg` then points at a
// buffer that is filling). By default every decoder works on its own 4 KB of stack, so two may
// overlap; a trailing `pool` argument (4096 bytes, 4-byte aligned) supplies the work area instead
// for a task short of stack, but then it must not be shared by two decodes running at once.
// decode_fit's lookup tables are static: one at a time.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef TRACKER_HOST_TEST
extern "C" {
#include "tjpgd.h"
}
namespace tracker_jpg_rom {
using ::JDEC;
using ::JRECT;
using ::JRESULT;
using ::jd_decomp;
using ::jd_prepare;
typedef size_t in_len_t;
typedef int out_ret_t;
}  // namespace tracker_jpg_rom
#else
namespace tracker_jpg_rom {   // the ROM header's short type names (BYTE, WORD, ...) stay in here
#include "rom/tjpgd.h"
typedef UINT in_len_t;
typedef UINT out_ret_t;
}  // namespace tracker_jpg_rom
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

namespace tracker {
namespace jpg {

namespace rom = tracker_jpg_rom;

struct Progress {
  void (*rows)(int done, void *ctx) = nullptr;
  void *ctx = nullptr;
  const volatile bool *cancel = nullptr;
  const volatile size_t *have = nullptr;   // streaming: bytes in so far ...
  const volatile bool *eof = nullptr;      // ... and whether more will come
};

struct Src {
  const uint8_t *p;
  size_t len, pos;
  void *dst;                  // the decoder's own context (a pixel buffer, Fit or Copy)
  int tw, th, offx, offy;
  const Progress *pr;
};

inline bool cancelled(const Progress *pr) { return pr && pr->cancel && *pr->cancel; }

inline rom::in_len_t in(rom::JDEC *jd, uint8_t *buf, rom::in_len_t n) {
  Src *s = (Src *) jd->device;
  if (s->pr && s->pr->have) {   // wait for the download to bring these bytes (or end)
    while (s->pos + n > *s->pr->have && !(s->pr->eof && *s->pr->eof)) {
      if (cancelled(s->pr)) return 0;
#ifndef TRACKER_HOST_TEST
      vTaskDelay(pdMS_TO_TICKS(5));
#endif
    }
    s->len = *s->pr->have;
  }
  if (cancelled(s->pr)) return 0;
  const size_t k = n < s->len - s->pos ? (size_t) n : s->len - s->pos;
  if (buf) std::memcpy(buf, s->p + s->pos, k);
  s->pos += k;
  return (rom::in_len_t) k;
}

inline uint16_t rgb565(unsigned r, unsigned g, unsigned b) {
  return (uint16_t) ((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

// TJpgDec's work area (~3.1 KB; 4096 is fixed). Each decoder takes an optional trailing `pool`:
// nullptr = a 4 KB aligned array on the caller's stack (re-entrant, nothing held when idle); a
// project short of stack can pass its own 4-byte-aligned 4096-byte buffer, which must not be shared
// by two decodes that run at the same time.
constexpr size_t POOL_SIZE = 4096;
template <class F>
__attribute__((noinline)) const char *with_stack_pool(F &&f) {
  uint8_t pool[POOL_SIZE] __attribute__((aligned(4)));
  return f(pool);
}

// ---- half size ------------------------------------------------------------------------
inline rom::out_ret_t out_half(rom::JDEC *jd, void *bitmap, rom::JRECT *r) {
  Src *s = (Src *) jd->device;
  uint16_t *dst = (uint16_t *) s->dst;
  const uint8_t *px = (const uint8_t *) bitmap;      // RGB888, the rectangle row by row
  const int w = r->right - r->left + 1, stride = 3 * w;
  for (int y = r->top; y + 1 <= r->bottom; y += 2) {
    const int ty = y / 2 - s->offy;
    if (ty < 0 || ty >= s->th) continue;
    uint16_t *o = dst + (size_t) ty * s->tw;
    const uint8_t *a = px + (size_t) (y - r->top) * stride;
    for (int x = r->left; x + 1 <= r->right; x += 2, a += 6) {
      const int tx = x / 2 - s->offx;
      if (tx < 0 || tx >= s->tw) continue;
      const uint8_t *b = a + stride;
      o[tx] = rgb565((a[0] + a[3] + b[0] + b[3] + 2) >> 2, (a[1] + a[4] + b[1] + b[4] + 2) >> 2,
                     (a[2] + a[5] + b[2] + b[5] + 2) >> 2);
    }
  }
  if (s->pr && s->pr->rows && r->right + 1 >= (int) jd->width)    // the end of a row of blocks
    s->pr->rows(std::max(0, std::min(s->th, (r->bottom + 1) / 2 - s->offy)), s->pr->ctx);
  return cancelled(s->pr) ? 0 : 1;
}

inline const char *open(rom::JDEC *jd, uint8_t *pool, size_t pool_n, Src *s) {
  std::memset(jd, 0, sizeof(*jd));
  const rom::JRESULT r = rom::jd_prepare(jd, in, pool, pool_n, s);
  if (r == 0) return nullptr;
  if (cancelled(s->pr)) return "cancelled";
  return r == 8 || r == 7 ? "unsupported JPEG (progressive?)" : "not a readable JPEG";
}

inline const char *finish(int r, const Progress *pr, int th) {
  if (cancelled(pr)) return "cancelled";
  if (r == 0 && pr && pr->rows) pr->rows(th, pr->ctx);
  return r == 0 ? nullptr : "corrupt JPEG data";
}

// nullptr on success, else a short reason. The picture must be at least 2*tw x 2*th; a
// larger one is centred and cropped. The caller owns `dst` (tw*th pixels). `sw`/`sh`: the
// source size.
inline const char *decode_half(const uint8_t *jpg, size_t len, uint16_t *dst, int tw, int th,
                               const Progress *pr = nullptr, int *sw = nullptr, int *sh = nullptr,
                               uint8_t *pool = nullptr) {
  auto run = [&](uint8_t *wk) -> const char * {
    Src s{jpg, len, 0, dst, tw, th, 0, 0, pr};
    rom::JDEC jd;
    if (const char *e = open(&jd, wk, POOL_SIZE, &s)) return e;
    if (sw) *sw = (int) jd.width;
    if (sh) *sh = (int) jd.height;
    const int hw = (int) (jd.width + 1) / 2, hh = (int) (jd.height + 1) / 2;
    if (hw < tw || hh < th) return "picture smaller than expected";
    s.offx = (hw - tw) / 2;
    s.offy = (hh - th) / 2;    // (every target pixel is written: the picture is at least the target)
    return finish(rom::jd_decomp(&jd, out_half, 0), pr, th);
  };
  return pool ? run(pool) : with_stack_pool(run);
}

// ---- 1:1 --------------------------------------------------------------------------------
inline rom::out_ret_t out_full(rom::JDEC *jd, void *bitmap, rom::JRECT *r) {
  Src *s = (Src *) jd->device;
  uint16_t *dst = (uint16_t *) s->dst;
  const uint8_t *px = (const uint8_t *) bitmap;
  for (int y = r->top; y <= r->bottom; y++) {
    const int ty = y - s->offy;
    for (int x = r->left; x <= r->right; x++, px += 3) {
      const int tx = x - s->offx;
      if (ty < 0 || ty >= s->th || tx < 0 || tx >= s->tw) continue;
      dst[(size_t) ty * s->tw + tx] = rgb565(px[0], px[1], px[2]);
    }
  }
  if (s->pr && s->pr->rows && r->right + 1 >= (int) jd->width)
    s->pr->rows(std::max(0, std::min(s->th, r->bottom + 1 - s->offy)), s->pr->ctx);
  return cancelled(s->pr) ? 0 : 1;
}

// The picture 1:1 (a 240x320 portrait). Same contract as decode_half.
inline const char *decode_full(const uint8_t *jpg, size_t len, uint16_t *dst, int tw, int th,
                               const Progress *pr = nullptr, int *sw = nullptr, int *sh = nullptr,
                               uint8_t *pool = nullptr) {
  auto run = [&](uint8_t *wk) -> const char * {
    Src s{jpg, len, 0, dst, tw, th, 0, 0, pr};
    rom::JDEC jd;
    if (const char *e = open(&jd, wk, POOL_SIZE, &s)) return e;
    if (sw) *sw = (int) jd.width;
    if (sh) *sh = (int) jd.height;
    if ((int) jd.width < tw || (int) jd.height < th) return "picture smaller than expected";
    s.offx = ((int) jd.width - tw) / 2;
    s.offy = ((int) jd.height - th) / 2;
    return finish(rom::jd_decomp(&jd, out_full, 0), pr, th);
  };
  return pool ? run(pool) : with_stack_pool(run);
}

// ---- any size down to tw x th ------------------------------------------------------------
// A box filter. The sums live in `acc` (3 x uint16 per target pixel: at most 4 source pixels
// of 255 per cell below 2:1).
struct Fit {
  uint16_t *dst, *acc;
  int tw, th, W, H;
  uint16_t *xm, *ym;         // source column / row -> target
  int stride = 0, next = 0;  // rows below `next` are finished (written to dst)
};
inline uint8_t fit_cx[1024], fit_cy[1024];  // source pixels per target column / row
inline uint16_t fit_last[1024];             // the last source row of each target row
inline void fit_row(Fit *f, int y) {        // one target row: the sums averaged into dst
  for (int x = 0; x < f->tw; x++) {
    const uint16_t *a = f->acc + ((size_t) y * f->tw + x) * 3;
    const unsigned n = fit_cx[x] * fit_cy[y], h2 = n / 2;
    f->dst[(size_t) y * (f->stride > 0 ? f->stride : f->tw) + x] =
        rgb565((a[0] + h2) / n, (a[1] + h2) / n, (a[2] + h2) / n);
  }
}
inline rom::out_ret_t out_fit(rom::JDEC *jd, void *bitmap, rom::JRECT *r) {
  Src *s = (Src *) jd->device;
  Fit *f = (Fit *) s->dst;
  const uint8_t *px = (const uint8_t *) bitmap;
  for (int y = r->top; y <= r->bottom; y++) {
    if (f->ym[y] == 0xFFFF) {  // outside the source window
      px += 3 * (r->right - r->left + 1);
      continue;
    }
    uint16_t *row = f->acc + (size_t) f->ym[y] * f->tw * 3;
    for (int x = r->left; x <= r->right; x++, px += 3) {
      if (f->xm[x] == 0xFFFF) continue;
      uint16_t *a = row + f->xm[x] * 3;
      a[0] += px[0], a[1] += px[1], a[2] += px[2];
    }
  }
  if (r->right + 1 >= f->W) {  // the end of a row of blocks: the target rows now complete
    const int before = f->next;
    while (f->next < f->th && fit_last[f->next] <= r->bottom) fit_row(f, f->next++);
    if (s->pr && s->pr->rows && f->next != before) s->pr->rows(f->next, s->pr->ctx);
  }
  return cancelled(s->pr) ? 0 : 1;
}
// `keep`: the fraction of the height kept from the top, centred square window (NOAA's regional
// frames carry a label strip along the bottom). 1 = the whole picture; < 0: the top square.
// `stride`: pixels per destination row (0 = tw), to decode into part of a larger picture.
// `centre`: the kept square in the middle (SOHO's Sun).
inline const char *decode_fit(const uint8_t *jpg, size_t len, uint16_t *dst, int tw, int th, uint16_t *acc,
                              const Progress *pr = nullptr, int *sw = nullptr, int *sh = nullptr,
                              float keep = 1.0f, int stride = 0, bool centre = false,
                              uint8_t *pool = nullptr) {
  auto run = [&](uint8_t *wk) -> const char * {
    static uint16_t xm[2048], ym[2048];
    Fit f;
    Src s{jpg, len, 0, &f, tw, th, 0, 0, pr};
    rom::JDEC jd;
    if (const char *e = open(&jd, wk, POOL_SIZE, &s)) return e;
    const int W = (int) jd.width, H = (int) jd.height;
    if (sw) *sw = W;
    if (sh) *sh = H;
    // keep < 0: the top square (the all-sky camera's fisheye above its caption strip)
    const int wh = keep < 0 ? std::min(W, H) : keep >= 1.0f ? H : (int) (H * keep),
              ww = keep >= 1.0f ? W : std::min(W, wh);
    if (ww < tw || wh < th || W > 2048 || H > 2048) return "unexpected picture size";
    // more than 2:1 (the sums would overflow): TJpgDec's own halving first, 1/2..1/8
    int sc = 0;
    while (sc < 3 && (((ww >> sc) >= 2 * tw) || ((wh >> sc) >= 2 * th))) sc++;
    const int Ws = W >> sc, Hs = H >> sc, wws = ww >> sc, whs = wh >> sc;
    if (wws < tw || whs < th || wws >= 2 * tw || whs >= 2 * th) return "unexpected picture size";
    const int x0 = (Ws - wws) / 2, y0 = centre ? (Hs - whs) / 2 : 0;
    std::memset(fit_cx, 0, sizeof(fit_cx));
    std::memset(fit_cy, 0, sizeof(fit_cy));
    std::memset(fit_last, 0, sizeof(fit_last));
    for (int x = 0; x < Ws; x++)
      xm[x] = x < x0 || x >= x0 + wws ? 0xFFFF : (uint16_t) ((int64_t) (x - x0) * tw / wws);
    for (int y = 0; y < Hs; y++)
      ym[y] = y < y0 || y >= y0 + whs ? 0xFFFF : (uint16_t) ((int64_t) (y - y0) * th / whs);
    for (int x = 0; x < Ws; x++)
      if (xm[x] != 0xFFFF) fit_cx[xm[x]]++;
    for (int y = 0; y < Hs; y++)
      if (ym[y] != 0xFFFF) {
        fit_cy[ym[y]]++;
        fit_last[ym[y]] = (uint16_t) y;
      }
    f = Fit{dst, acc, tw, th, Ws, Hs, xm, ym, stride, 0};
    std::memset(acc, 0, sizeof(uint16_t) * 3 * tw * th);
    const int r = rom::jd_decomp(&jd, out_fit, (uint8_t) sc);
    if (cancelled(pr)) return "cancelled";
    if (r != 0) return "corrupt JPEG data";
    while (f.next < th) fit_row(&f, f.next++);   // (normally all done by the last row of blocks)
    if (pr && pr->rows) pr->rows(th, pr->ctx);
    return nullptr;
  };
  return pool ? run(pool) : with_stack_pool(run);
}

// ---- 1:1 into part of a larger picture ----------------------------------------------------
// (planet photos stored at their display size): no scaling, no sums.
struct Copy {
  uint16_t *dst;
  int w, h, stride;
};
inline rom::out_ret_t out_copy(rom::JDEC *jd, void *bitmap, rom::JRECT *r) {
  Src *s = (Src *) jd->device;
  const Copy *c = (const Copy *) s->dst;
  const uint8_t *px = (const uint8_t *) bitmap;
  for (int y = r->top; y <= r->bottom; y++)
    for (int x = r->left; x <= r->right; x++, px += 3)
      if (x < c->w && y < c->h) c->dst[(size_t) y * c->stride + x] = rgb565(px[0], px[1], px[2]);
  return cancelled(s->pr) ? 0 : 1;
}
inline const char *decode_copy(const uint8_t *jpg, size_t len, uint16_t *dst, int stride, int maxw, int maxh,
                               const Progress *pr = nullptr, uint8_t *pool = nullptr) {
  auto run = [&](uint8_t *wk) -> const char * {
    Copy c{dst, 0, 0, stride};
    Src s{jpg, len, 0, &c, 0, 0, 0, 0, pr};
    rom::JDEC jd;
    if (const char *e = open(&jd, wk, POOL_SIZE, &s)) return e;
    c.w = std::min((int) jd.width, maxw);
    c.h = std::min((int) jd.height, maxh);
    const int r = rom::jd_decomp(&jd, out_copy, 0);
    if (cancelled(pr)) return "cancelled";
    return r == 0 ? nullptr : "corrupt JPEG data";
  };
  return pool ? run(pool) : with_stack_pool(run);
}

}  // namespace jpg
}  // namespace tracker
