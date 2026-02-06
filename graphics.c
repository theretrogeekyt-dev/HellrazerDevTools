// graphics.c - VGA Graphics Operations for MS-DOS Mode 13h

#include <dos.h>
#include <conio.h>

void set_mode(int mode) {
    union REGS regs;
    regs.h.ah = 0x00; // Function to set video mode
    regs.h.al = mode; // Mode 13h
    int86(0x10, &regs, &regs);
}

void clear_screen() {
    asm {
        mov ax, 0x00; // Function to Clear the Screen
        mov bh, 0x00; // Attribute
        mov cx, 0x0000; // Top-left corner (0,0)
        mov dx, 0xFFFF; // Bottom-right corner (319,199)
        int 0x10;
    }
}

void put_pixel(int x, int y, int color) {
    int offset = (y * 320) + x;
    *((unsigned char*)0xA0000000 + offset) = color;
}

void draw_line(int x1, int y1, int x2, int y2, int color) {
    // Simple Bresenham's line algorithm implementation
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void wait_for_key() {
    while (!kbhit()); // Wait for key hit
    getch(); // Clear the key from the buffer
}

void main() {
    set_mode(0x13); // Set mode 13h
    clear_screen(); // Clear screen
    draw_line(100, 100, 200, 150, 15); // Draw a line
    wait_for_key(); // Wait for key press to exit
    set_mode(0x03); // Return to text mode
}