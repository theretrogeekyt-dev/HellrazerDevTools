import math
import sys
from dataclasses import dataclass

import pygame
from OpenGL.GL import (
    GL_COLOR_BUFFER_BIT,
    GL_DEPTH_BUFFER_BIT,
    GL_QUADS,
    glBegin,
    glClear,
    glColor3f,
    glEnd,
    glOrtho,
    glVertex2f,
)
from OpenGL.GL import glClearColor
from pygame.locals import DOUBLEBUF, OPENGL

MAP_DATA = [
    "111111111111",
    "1..........1",
    "1..111.....1",
    "1..1.......1",
    "1..1.......1",
    "1..1.......1",
    "1..111111..1",
    "1..........1",
    "1..........1",
    "1......1...1",
    "1..........1",
    "111111111111",
]


@dataclass
class Player:
    x: float
    y: float
    angle: float
    move_speed: float = 2.6
    turn_speed: float = 2.2


@dataclass
class RayHit:
    distance: float
    hit_vertical: bool


def cell_is_wall(x: int, y: int) -> bool:
    if y < 0 or y >= len(MAP_DATA):
        return True
    if x < 0 or x >= len(MAP_DATA[0]):
        return True
    return MAP_DATA[y][x] == "1"


def cast_ray(player: Player, ray_angle: float, max_depth: int = 24) -> RayHit:
    sin_a = math.sin(ray_angle)
    cos_a = math.cos(ray_angle)
    distance = 0.0
    hit_vertical = False

    while distance < max_depth:
        distance += 0.02
        target_x = player.x + cos_a * distance
        target_y = player.y + sin_a * distance
        if cell_is_wall(int(target_x), int(target_y)):
            offset_x = target_x - int(target_x)
            offset_y = target_y - int(target_y)
            hit_vertical = offset_x < 0.02 or offset_x > 0.98
            if not hit_vertical:
                hit_vertical = offset_y < 0.02 or offset_y > 0.98
            break
    return RayHit(distance=distance, hit_vertical=hit_vertical)


def draw_quad(x1: float, y1: float, x2: float, y2: float, color: tuple[float, float, float]) -> None:
    glColor3f(*color)
    glBegin(GL_QUADS)
    glVertex2f(x1, y1)
    glVertex2f(x2, y1)
    glVertex2f(x2, y2)
    glVertex2f(x1, y2)
    glEnd()


def render_scene(player: Player, width: int, height: int, fov: float) -> None:
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    draw_quad(0, 0, width, height / 2, (0.25, 0.3, 0.4))
    draw_quad(0, height / 2, width, height, (0.15, 0.15, 0.15))

    num_rays = width
    half_height = height / 2
    for ray in range(num_rays):
        ray_angle = player.angle - fov / 2 + (ray / num_rays) * fov
        hit = cast_ray(player, ray_angle)
        distance = hit.distance * math.cos(ray_angle - player.angle)
        distance = max(distance, 0.001)
        wall_height = min(1.0 / distance * height * 0.6, height)
        wall_top = half_height - wall_height / 2
        wall_bottom = half_height + wall_height / 2
        shade = 0.8 if hit.hit_vertical else 1.0
        color = (0.55 * shade, 0.5 * shade, 0.45 * shade)
        draw_quad(ray, wall_top, ray + 1, wall_bottom, color)

    pygame.display.flip()


def handle_input(player: Player, delta: float) -> None:
    keys = pygame.key.get_pressed()
    rotation = 0.0
    move_step = 0.0
    strafe_step = 0.0

    if keys[pygame.K_a]:
        rotation -= player.turn_speed * delta
    if keys[pygame.K_d]:
        rotation += player.turn_speed * delta
    if keys[pygame.K_w]:
        move_step += player.move_speed * delta
    if keys[pygame.K_s]:
        move_step -= player.move_speed * delta
    if keys[pygame.K_q]:
        strafe_step -= player.move_speed * delta
    if keys[pygame.K_e]:
        strafe_step += player.move_speed * delta

    player.angle = (player.angle + rotation) % (math.tau)

    dx = math.cos(player.angle) * move_step + math.cos(player.angle + math.pi / 2) * strafe_step
    dy = math.sin(player.angle) * move_step + math.sin(player.angle + math.pi / 2) * strafe_step
    next_x = player.x + dx
    next_y = player.y + dy

    if not cell_is_wall(int(next_x), int(player.y)):
        player.x = next_x
    if not cell_is_wall(int(player.x), int(next_y)):
        player.y = next_y


def init_display(width: int, height: int) -> None:
    pygame.init()
    pygame.display.set_mode((width, height), DOUBLEBUF | OPENGL)
    glClearColor(0.05, 0.05, 0.05, 1.0)
    glOrtho(0, width, height, 0, -1, 1)


def main() -> None:
    width, height = 800, 500
    fov = math.radians(60)
    init_display(width, height)

    clock = pygame.time.Clock()
    player = Player(x=3.5, y=3.5, angle=0.0)

    running = True
    while running:
        delta = clock.tick(60) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                running = False

        handle_input(player, delta)
        render_scene(player, width, height, fov)

    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
