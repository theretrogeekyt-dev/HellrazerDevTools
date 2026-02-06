// graphics.h

#ifndef GRAPHICS_H
#define GRAPHICS_H

void initVGA();
void setPixel(int x, int y, unsigned char color);
unsigned char getPixel(int x, int y);
void clearScreen(unsigned char color);
void drawRectangle(int x, int y, int width, int height, unsigned char color);
void drawLine(int x1, int y1, int x2, int y2, unsigned char color);

#endif // GRAPHICS_H