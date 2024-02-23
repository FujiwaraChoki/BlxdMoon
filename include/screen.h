#include <stdio.h>
#include <Windows.h>

BOOL SaveToFile(HBITMAP hBitmap3, LPCTSTR lpszFileName)
{
  HDC hDC;
  int iBits;
  WORD wBitCount;
  DWORD dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0, dwWritten = 0;
  BITMAP Bitmap0;
  BITMAPFILEHEADER bmfHdr;
  BITMAPINFOHEADER bi;
  LPBITMAPINFOHEADER lpbi;
  HANDLE fh, hDib, hPal, hOldPal2 = NULL;
  hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
  iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
  DeleteDC(hDC);
  if (iBits <= 1)
    wBitCount = 1;
  else if (iBits <= 4)
    wBitCount = 4;
  else if (iBits <= 8)
    wBitCount = 8;
  else
    wBitCount = 24;
  GetObject(hBitmap3, sizeof(Bitmap0), (LPSTR)&Bitmap0);
  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = Bitmap0.bmWidth;
  bi.biHeight = -Bitmap0.bmHeight;
  bi.biPlanes = 1;
  bi.biBitCount = wBitCount;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrImportant = 0;
  bi.biClrUsed = 256;
  dwBmBitsSize = ((Bitmap0.bmWidth * wBitCount + 31) & ~31) / 8 * Bitmap0.bmHeight;
  hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
  lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
  *lpbi = bi;

  hPal = GetStockObject(DEFAULT_PALETTE);
  if (hPal)
  {
    hDC = GetDC(NULL);
    hOldPal2 = SelectPalette(hDC, (HPALETTE)hPal, FALSE);
    RealizePalette(hDC);
  }

  GetDIBits(hDC, hBitmap3, 0, (UINT)Bitmap0.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

  if (hOldPal2)
  {
    SelectPalette(hDC, (HPALETTE)hOldPal2, TRUE);
    RealizePalette(hDC);
    ReleaseDC(NULL, hDC);
  }

  fh = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if (fh == INVALID_HANDLE_VALUE)
    return FALSE;

  bmfHdr.bfType = 0x4D42; // "BM"
  dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
  bmfHdr.bfSize = dwDIBSize;
  bmfHdr.bfReserved1 = 0;
  bmfHdr.bfReserved2 = 0;
  bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

  WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);

  WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
  GlobalUnlock(hDib);
  GlobalFree(hDib);
  CloseHandle(fh);

  return TRUE;
}

int screenCapture(int x, int y, int w, int h, LPCTSTR fname)
{
  HDC hdcSource = GetDC(NULL);
  HDC hdcMemory = CreateCompatibleDC(hdcSource);

  int capX = GetDeviceCaps(hdcSource, HORZRES);
  int capY = GetDeviceCaps(hdcSource, VERTRES);

  HBITMAP hBitmap = CreateCompatibleBitmap(hdcSource, w, h);
  HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMemory, hBitmap);

  BitBlt(hdcMemory, 0, 0, w, h, hdcSource, x, y, SRCCOPY);
  hBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmapOld);

  DeleteDC(hdcSource);
  DeleteDC(hdcMemory);

  HPALETTE hpal = NULL;
  if (SaveToFile(hBitmap, fname))
  {
    return 1;
  }

  return 0;
}