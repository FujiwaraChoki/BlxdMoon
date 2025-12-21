#ifndef SCREEN_H
#define SCREEN_H

#include <stdio.h>
#include <windows.h>

BOOL SaveToFile(HBITMAP hBitmap3, LPCTSTR lpszFileName);
int screenCapture(int x, int y, int w, int h, LPCTSTR fname);

#endif