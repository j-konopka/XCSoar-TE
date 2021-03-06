/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/Custom/TopCanvas.hpp"
#include "Screen/Canvas.hpp"

#ifdef DITHER
#include "../Memory/Dither.hpp"
#endif

#if defined(KOBO) && defined(USE_FB)
#include "mxcfb.h"
#endif

#include <algorithm>

#ifdef USE_FB
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

void
TopCanvas::Destroy()
{
  buffer.Free();

#ifdef USE_FB
#ifdef USE_TTY
  DeinitialiseTTY();
#endif

  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
#endif
}

PixelRect
TopCanvas::GetRect() const
{
  assert(IsDefined());

  return { 0, 0, int(buffer.width), int(buffer.height) };
}

void
TopCanvas::Create(PixelSize new_size,
                  bool full_screen, bool resizable)
{
#ifdef USE_FB
  assert(fd < 0);

#ifdef USE_TTY
  InitialiseTTY();
#endif

  const char *path = "/dev/fb0";
  fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
    return;
  }

  struct fb_fix_screeninfo finfo;
  if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
    fprintf(stderr, "FBIOGET_FSCREENINFO failed: %s\n", strerror(errno));
    Destroy();
    return;
  }

  if (finfo.type != FB_TYPE_PACKED_PIXELS) {
    fprintf(stderr, "Unsupported console hardware\n");
    Destroy();
    return;
  }

  switch (finfo.visual) {
  case FB_VISUAL_TRUECOLOR:
  case FB_VISUAL_PSEUDOCOLOR:
  case FB_VISUAL_STATIC_PSEUDOCOLOR:
  case FB_VISUAL_DIRECTCOLOR:
    break;

  default:
    fprintf(stderr, "Unsupported console hardware\n");
    Destroy();
    return;
  }

  /* Memory map the device, compensating for buggy PPC mmap() */
  const off_t page_size = getpagesize();
  off_t offset = off_t(finfo.smem_start)
    - (off_t(finfo.smem_start) &~ (page_size - 1));
  off_t map_size = finfo.smem_len + offset;

  map = mmap(nullptr, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == (void *)-1) {
    fprintf(stderr, "Unable to memory map the video hardware: %s\n",
            strerror(errno));
    map = nullptr;
    Destroy();
    return;
  }

  /* Determine the current screen depth */
  struct fb_var_screeninfo vinfo;
  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ) {
    fprintf(stderr, "Couldn't get console pixel format: %s\n",
            strerror(errno));
    Destroy();
    return;
  }

#ifdef GREYSCALE
  /* switch the frame buffer to 8 bits per pixel greyscale */

  vinfo.bits_per_pixel = 8;
  vinfo.grayscale = true;

  if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) < 0) {
    fprintf(stderr, "Couldn't set greyscale pixel format: %s\n",
            strerror(errno));
    Destroy();
    return;
  }

  /* read new finfo */
  ioctl(fd, FBIOGET_FSCREENINFO, &finfo);

  map_bpp = 1;
#else
  map_bpp = vinfo.bits_per_pixel / 8;
  if (map_bpp != 2 && map_bpp != 4) {
    fprintf(stderr, "Unsupported console hardware\n");
    Destroy();
    return;
  }
#endif

  map_pitch = finfo.line_length;
  epd_update_marker = 0;

#ifdef KOBO
  ioctl(fd, MXCFB_SET_UPDATE_SCHEME, UPDATE_SCHEME_QUEUE_AND_MERGE);
#endif

  const unsigned width = vinfo.xres, height = vinfo.yres;
#elif defined(USE_VFB)
  const unsigned width = new_size.cx, height = new_size.cy;
#else
#error No implementation
#endif

  buffer.Allocate(width, height);
}

#ifdef USE_FB

bool
TopCanvas::CheckResize()
{
  /* get new frame buffer dimensions and check if they have changed */
  struct fb_var_screeninfo vinfo;
  ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);

  if (vinfo.xres == buffer.width && vinfo.yres == buffer.height)
    return false;

  /* yes, they did change: update the size and allocate a new buffer */

  struct fb_fix_screeninfo finfo;
  ioctl(fd, FBIOGET_FSCREENINFO, &finfo);

  map_pitch = finfo.line_length;

  buffer.Free();
  buffer.Allocate(vinfo.xres, vinfo.yres);
  return true;
}

#endif

void
TopCanvas::OnResize(PixelSize new_size)
{
  // TODO: is this even possible?
}

void
TopCanvas::Fullscreen()
{
}

Canvas
TopCanvas::Lock()
{
  return Canvas(buffer);
}

void
TopCanvas::Unlock()
{
}

#ifdef USE_FB

#ifdef GREYSCALE

#ifdef DITHER

#else

static uint32_t
GreyscaleToRGB8(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return value | (value << 8) | (value << 16) | (value << 24);
}

static void
CopyGreyscaleToRGB8(uint32_t *gcc_restrict dest,
                     const Luminosity8 *gcc_restrict src,
                     unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB8(*src++);
}

static RGB565Color
GreyscaleToRGB565(Luminosity8 luminosity)
{
  const unsigned value = luminosity.GetLuminosity();

  return RGB565Color(value, value, value);
}

static void
CopyGreyscaleToRGB565(RGB565Color *gcc_restrict dest,
                      const Luminosity8 *gcc_restrict src,
                      unsigned width)
{
  for (unsigned i = 0; i < width; ++i)
    *dest++ = GreyscaleToRGB565(*src++);
}

#endif

#ifdef KOBO

static void
CopyGreyscale(uint8_t *dest_pixels, unsigned dest_pitch,
              const uint8_t *src_pixels, unsigned src_pitch,
              unsigned width, unsigned height)
{
  for (unsigned y = 0; y < height;
       ++y, dest_pixels += dest_pitch, src_pixels += src_pitch)
    std::copy_n(src_pixels, width, dest_pixels);
}

#endif

static void
CopyFromGreyscale(
#ifdef DITHER
                  Dither &dither,
#endif
#ifdef KOBO
                  bool enable_dither,
#endif
                  void *dest_pixels, unsigned dest_pitch, unsigned dest_bpp,
                  ConstImageBuffer<GreyscalePixelTraits> src)
{
  const uint8_t *src_pixels = reinterpret_cast<const uint8_t *>(src.data);

  const unsigned width = src.width, height = src.height;

#ifdef KOBO
  if (!enable_dither) {
    CopyGreyscale((uint8_t *)dest_pixels, dest_pitch,
                  src_pixels, src.pitch,
                  width, height);
    return;
  }
#endif

#ifdef DITHER

  dither.DitherGreyscale(src_pixels, src.pitch,
                         (uint8_t *)dest_pixels,
                         dest_pitch,
                         width, height);

#ifndef KOBO
  if (dest_bpp == 4) {
    const unsigned n_pixels = (dest_pitch / dest_bpp)
      * height;
    int32_t *d = (int32_t *)dest_pixels + n_pixels;
    const int8_t *end = (int8_t *)dest_pixels;
    const int8_t *s = end + n_pixels;

    while (s != end)
      *--d = *--s;
  }
#endif

#else

  const unsigned src_pitch = src.pitch;

  if (dest_bpp == 2) {
    for (unsigned row = height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB565((RGB565Color *)dest_pixels,
                            (const Luminosity8 *)src_pixels, width);
  } else {
    for (unsigned row = height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      CopyGreyscaleToRGB8((uint32_t *)dest_pixels,
                           (const Luminosity8 *)src_pixels, width);
  }

#endif
}

#else

static RGB565Color
ToRGB565(BGRA8Color c)
{
  return RGB565Color(c.Red(), c.Green(), c.Blue());
}

static void
BGRAToRGB565(RGB565Color *dest, const BGRA8Color *src, unsigned n)
{
  for (unsigned i = 0; i < n; ++i)
    dest[i] = ToRGB565(src[i]);
}

static void
CopyFromBGRA(void *_dest_pixels, unsigned _dest_pitch, unsigned dest_bpp,
             ConstImageBuffer<BGRAPixelTraits> src)
{
  assert(dest_bpp == 4 || dest_bpp == 2);

  const uint32_t dest_pitch = _dest_pitch / dest_bpp;
  const uint32_t src_pitch = src.pitch / sizeof(*src.data);

  if (dest_bpp == 2) {
    /* convert to RGB565 */

    RGB565Color *dest_pixels = reinterpret_cast<RGB565Color *>(_dest_pixels);
    const BGRA8Color *src_pixels = src.data;

    for (unsigned row = src.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      BGRAToRGB565((RGB565Color *)dest_pixels,
                   (const BGRA8Color *)src_pixels,
                   src.width);
  } else {
    uint32_t *dest_pixels = reinterpret_cast<uint32_t *>(_dest_pixels);
    const uint32_t *src_pixels = reinterpret_cast<const uint32_t *>(src.data);

    for (unsigned row = src.height; row > 0;
         --row, src_pixels += src_pitch, dest_pixels += dest_pitch)
      std::copy_n(src_pixels, src.width, dest_pixels);
  }
}

#endif

#endif /* USE_FB */

void
TopCanvas::Flip()
{
#ifdef USE_FB

#ifdef GREYSCALE
  CopyFromGreyscale(
#ifdef DITHER
                    dither,
#endif
#ifdef KOBO
                    enable_dither,
#endif
                    map, map_pitch, map_bpp,
                    buffer);
#else
  CopyFromBGRA(map, map_pitch, map_bpp, buffer);
#endif


#ifdef KOBO
  epd_update_marker++;

  struct mxcfb_update_data epd_update_data = {
    {
      0, 0, buffer.width, buffer.height
    },

    WAVEFORM_MODE_AUTO,
    UPDATE_MODE_FULL, // PARTIAL
    epd_update_marker,
    TEMP_USE_AMBIENT,
    enable_dither ? EPDC_FLAG_FORCE_MONOCHROME : 0,
  };

  ioctl(fd, MXCFB_SEND_UPDATE, &epd_update_data);
#endif

#endif /* USE_FB */
}

#ifdef KOBO

void
TopCanvas::Wait()
{
  ioctl(fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &epd_update_marker);
}

#endif
