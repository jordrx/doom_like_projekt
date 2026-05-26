#include <SFML/Graphics.hpp>
#include <CMath>
#include <string>
#include <cstdint>

const int MAP_W = 8;
const int MAP_H = 8;

const float MOVE_SPEED = 3.0f;
const float ROTATE_SPEED = 3.0f;

const float FOV = 3.14159f / 3.0f; // fov 60°
const int SCREEN_W = 800;
const int SCREEN_H = 600;

const float MAX_DEATH = 20.0f;



int map[MAP_H][MAP_W] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}
};



int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Ma fenetre SFML 3");
    
    float playerX = 3.0f;
    float playerY = 3.0f;
    float angle = 0.0f;


    sf::Clock clock;

    sf::Texture texture;
    texture.resize(sf::Vector2u(SCREEN_W, SCREEN_H));
    if (!texture.resize(sf::Vector2u(SCREEN_W, SCREEN_H)))
    return -1;
    std::vector<uint8_t> pixels(SCREEN_W * SCREEN_H * 4, 255);
    sf::Sprite sprite(texture);

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        window.setTitle("x: " + std::to_string(playerX) + " y: " + std::to_string(playerY) + " angle: " + std::to_string(angle));

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        
    // Player Rotation
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        angle -= ROTATE_SPEED * dt;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        angle += ROTATE_SPEED * dt;

    float dx = std::cos(angle) * MOVE_SPEED * dt;
    float dy = std::sin(angle) * MOVE_SPEED * dt;

    // Player Movement
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    {
        if (map[int(playerY)][int(playerX + dx)] == 0)
            playerX += dx;
        if (map[int(playerY + dy)][int(playerX)] == 0)
            playerY += dy; 
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    {
        if (map[int(playerY)][int(playerX - dx)] == 0)
            playerX -= dx;
        if (map[int(playerY - dy)][int(playerX)] == 0)
            playerY -= dy;
    }

    // Raycasting
    for (int col = 0; col < SCREEN_W; col++)
    {
        float rayAngle = angle - FOV / 2.0f + FOV * col / float(SCREEN_W);
        float rayDirX = std::cos(rayAngle);
        float rayDirY = std::sin(rayAngle);

        // Case actuelle du joueur
        int mapX = (int)playerX;
        int mapY = (int)playerY;

        // Distance pour traverser une case entière
        float deltaDistX = std::abs(1.0f / rayDirX);
        float deltaDistY = std::abs(1.0f / rayDirY);

        // Direction de progression dans la map
        int stepX, stepY;

        // Distance initiale jusqu'au premier bord de case
        float sideDistX, sideDistY;

        if (rayDirX < 0)
        {
            stepX = -1;
            sideDistX = (playerX - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1;
            sideDistX = (mapX + 1.0f - playerX) * deltaDistX;
        }

        if (rayDirY < 0)
        {
            stepY = -1;
            sideDistY = (playerY - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1;
            sideDistY = (mapY + 1.0f - playerY) * deltaDistY;
        }

        // DDA — on saute de case en case
        bool hit = false;
        int side; // 0 = mur vertical, 1 = mur horizontal

        while (!hit)
        {
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            if (map[mapY][mapX] == 1)
                hit = true;
        }

        // Distance corrigée (pas besoin de fish-eye avec DDA !)
        float dist;
        if (side == 0)
            dist = (mapX - playerX + (1 - stepX) / 2.0f) / rayDirX;
        else
            dist = (mapY - playerY + (1 - stepY) / 2.0f) / rayDirY;

        // Ombrage — murs horizontaux plus sombres
        int brightness = std::max(0, std::min(255, (int)(255 / (1 + dist * dist * 0.1f))));
        if (side == 1) brightness /= 2; // côté sombre

        int wallH = (int)(SCREEN_H / dist);
        int top = std::max(0, (SCREEN_H - wallH) / 2);
        int bot = std::min(SCREEN_H, (SCREEN_H + wallH) / 2);

        for (int y = 0; y < SCREEN_H; y++)
        {
            int index = (y * SCREEN_W + col) * 4;
            if (y < top)
            {
                pixels[index]=50; pixels[index+1]=50; pixels[index+2]=50;
            }
            else if (y < bot)
            {
                pixels[index]=brightness; pixels[index+1]=brightness; pixels[index+2]=brightness;
            }
            else
            {
                pixels[index]=100; pixels[index+1]=100; pixels[index+2]=100;
            }
            pixels[index+3] = 255;
        }
    }
    texture.update(pixels.data());
    window.draw(sprite);
    // Minimap
        const int TILE = 10; //size of the tile in pixels

        for (int y = 0; y < MAP_H; y++)
        {
            for (int x = 0; x < MAP_W; x++)
            {
                sf::RectangleShape tile(sf::Vector2f(TILE - 1, TILE - 1));
                tile.setPosition(sf::Vector2f(x * TILE, y * TILE));
                tile.setFillColor(map[y][x] == 1 ? sf::Color::White : sf::Color::Black);
                window.draw(tile);
            }
        }

        // Player on minimap
        sf::CircleShape player(5);
        player.setPosition(sf::Vector2f(playerX * TILE - 5, playerY * TILE - 5));
        player.setFillColor(sf::Color::Red);
        window.draw(player);

        
        window.display();
    }


    return 0;
}