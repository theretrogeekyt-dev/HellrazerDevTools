#ifndef WOLF3D_H
#define WOLF3D_H

// Core data structures for a Wolfenstein 3D-like renderer

typedef struct {
    float x;
    float y;
} Vector2;

typedef struct {
    int width;
    int height;
    unsigned char *data;
} Texture;

typedef struct {
    Vector2 position;
    Vector2 direction;
    float fov;
} Camera;

typedef struct {
    Texture *textures;
    int texture_count;
} World;

#endif // WOLF3D_H
