#include <dos.h>

// Function to initialize graphics mode 13h
void initVGA() {
    union REGS regs;
    regs.h.ah = 0;
    regs.h.al = 13;  // Set to mode 13h
    int86(0x10, &regs, &regs);
}

// Function to set a pixel at (x, y) with a specific color
void setPixel(int x, int y, int color) {
    asm {
        mov ax, 0xA000
        mov es, ax        // Segment for video memory
        mov di, y
        shl di, 8
        shl di, 6        // Multiply by 320 (screen width)
        add di, x        // Add x coordinate
        mov byte ptr es:[di], color
    }
}

// Function to draw a vertical line from (x, y1) to (y2) with a specific color
void drawVerticalLine(int x, int y1, int y2, int color) {
    for (int y = y1; y <= y2; y++) {
        setPixel(x, y, color);
    }
}

// Function to clear the screen
void clearScreen() {
    memset((void*)0xA0000, 0, 64000);  // Clear all pixels to black
}

// Function to restore text mode
void restoreTextMode() {
    union REGS regs;
    regs.h.ah = 0;
    regs.h.al = 3;  // Set to text mode
    int86(0x10, &regs, &regs);
}
