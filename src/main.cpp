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


float playerHP = 100.0f;

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

struct Enemy {
    float x, y;
    bool alive;
    float speed = 1.5f;
};

std::vector<Enemy> enemies = {
    {2.0f, 2.0f, true},
    {5.0f, 2.0f, true},
    {5.0f, 5.0f, true},
};

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Ma fenetre SFML 3");
    
    float playerX = 3.0f;
    float playerY = 3.0f;
    float angle = 0.0f;


    sf::Clock clock;

    sf::Texture texture;
    if (!texture.resize(sf::Vector2u(SCREEN_W, SCREEN_H)))
    return -1;
    std::vector<uint8_t> pixels(SCREEN_W * SCREEN_H * 4, 255);
    std::vector<float> zBuffer(SCREEN_W, 0.0f);
    sf::Sprite sprite(texture);

    sf::Image wallTexture;
    if (!wallTexture.loadFromFile("wall.png"))
        return -1;
    sf::Vector2u texSize = wallTexture.getSize();

    // Gun
    enum GunState { IDLE, SHOOT, RELOAD };
    GunState gunState = IDLE;
    float gunY = 0.0f;        // offset vertical du fusil (0 = position normale)
    float gunTimer = 0.0f;    // timer pour l'animation

    window.setMouseCursorVisible(false);
    //game loop
    while (window.isOpen())
    {
        
        float dt = clock.restart().asSeconds();
        window.setTitle("x: " + std::to_string(playerX) + " y: " + std::to_string(playerY) + " angle: " + std::to_string(angle));

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        // Tir
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && gunState == IDLE)
        {
            gunState = SHOOT;
            gunTimer = 0.0f;

            // Détection du tir
            for (auto& e : enemies)
            {
                if (!e.alive) continue;

                float dirX = std::cos(angle);
                float dirY = std::sin(angle);
                float planeX = -std::sin(angle) * 0.66f;
                float planeY =  std::cos(angle) * 0.66f;

                float ex = e.x - playerX;
                float ey = e.y - playerY;

                float invDet = 1.0f / (planeX * dirY - dirX * planeY);
                float transformX = invDet * (dirY * ex - dirX * ey);
                float transformY = invDet * (-planeY * ex + planeX * ey);

                if (transformY <= 0) continue;

                // Position à l'écran de l'ennemi
                int spriteScreenX = (int)((SCREEN_W / 2) * (1 + transformX / transformY));

                // Est-il au centre et pas derrière un mur ?
                int centerTolerance = 60; // largeur du viseur
                if (std::abs(spriteScreenX - SCREEN_W / 2) < centerTolerance
                    && transformY < zBuffer[spriteScreenX])
                {
                    e.alive = false;
                    break; // delete if you want to enable multi-kill per shot 
                }
            }
        }

        // Animation
        if (gunState == SHOOT)
        {
            gunY -= 800.0f * dt;  // remonte vite
            gunTimer += dt;
            if (gunTimer > 0.08f) // après 80ms passe en reload
                gunState = RELOAD;
        }
        else if (gunState == RELOAD)
        {
            gunY += 400.0f * dt;  // redescend doucement
            if (gunY >= 0.0f)
            {
                gunY = 0.0f;
                gunState = IDLE;
            }
        }
        
        // Player Rotation
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            angle -= ROTATE_SPEED * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            angle += ROTATE_SPEED * dt;
        // Rotation mouse
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Mouse::setPosition(sf::Vector2i(SCREEN_W / 2, SCREEN_H / 2), window);
        int mouseDeltaX = mousePos.x - SCREEN_W / 2;
        angle += mouseDeltaX * 0.002f;
        float dx = std::cos(angle) * MOVE_SPEED * dt;
        float dy = std::sin(angle) * MOVE_SPEED * dt;

        // Strafe gauche/droite (perpendiculaire à l'angle)
        float strafeX = std::cos(angle + 3.14159f / 2) * MOVE_SPEED * dt;
        float strafeY = std::sin(angle + 3.14159f / 2) * MOVE_SPEED * dt;

        // Player Movement
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z))
        {
            if (map[int(playerY)][int(playerX + dx)] == 0)
                playerX += dx;
            if (map[int(playerY + dy)][int(playerX)] == 0)
                playerY += dy; 
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        {
            if (map[int(playerY)][int(playerX - dx)] == 0)
                playerX -= dx;
            if (map[int(playerY - dy)][int(playerX)] == 0)
                playerY -= dy;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
        {
            if (map[int(playerY)][int(playerX - strafeX)] == 0) playerX -= strafeX;
            if (map[int(playerY - strafeY)][int(playerX)] == 0) playerY -= strafeY;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        {
            if (map[int(playerY)][int(playerX + strafeX)] == 0) playerX += strafeX;
            if (map[int(playerY + strafeY)][int(playerX)] == 0) playerY += strafeY;
        }
       
        // Enemy Movement
        for (auto& e : enemies)
        {
            if (!e.alive) continue;

            // Direction vers le joueur
            float dx = playerX - e.x;
            float dy = playerY - e.y;

            // Distance
            float dist = std::sqrt(dx * dx + dy * dy);

            // Normalise la direction
            if (dist > 0.5f) // s'arrête à 0.5 case du joueur
            {
                dx /= dist;
                dy /= dist;

                // Bouge avec collision
                float newX = e.x + dx * e.speed * dt;
                float newY = e.y + dy * e.speed * dt;

                if (map[int(e.y)][int(newX)] == 0) e.x = newX;
                if (map[int(newY)][int(e.x)] == 0) e.y = newY;
            }

            // Enemy damage
            if (dist < 0.5f)
                playerHP -= 20.0f * dt; // 20 dégâts par seconde
            playerHP = std::max(0.0f, playerHP);
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
            // Où le rayon a touché le mur
            float wallX;
            if (side == 0)
                wallX = playerY + dist * rayDirY;
            else
                wallX = playerX + dist * rayDirX;
            wallX -= std::floor(wallX);

            // Colonne de texture correspondante
            int texX = (int)(wallX * texSize.x);
            if (side == 0 && rayDirX > 0) texX = texSize.x - texX - 1;
            if (side == 1 && rayDirY < 0) texX = texSize.x - texX - 1;

        

            int wallH = (int)(SCREEN_H / dist);
            int top_real = (SCREEN_H - wallH) / 2;  // peut être négatif
            int top = std::max(0, top_real);          // clampé pour le dessin
            int bot = std::min(SCREEN_H, (SCREEN_H + wallH) / 2);

            // Après le calcul de dist, avant le dessin
            zBuffer[col] = dist;

            // Column drawing
            for (int y = 0; y < SCREEN_H; y++)
            {
                int index = (y * SCREEN_W + col) * 4;
                if (y < top)
                {
                    pixels[index]=50; pixels[index+1]=50; pixels[index+2]=50;
                }
                else if (y < bot)
                {
                    // Quelle ligne de texture ?
                    int texY = ((y - top_real) * (int)texSize.y) / wallH;
                    texY = std::max(0, std::min((int)texSize.y - 1, texY));

                    sf::Color color = wallTexture.getPixel(sf::Vector2u(texX, texY));

                    // Assombrir si mur horizontal
                    if (side == 1)
                    {
                        color.r /= 2;
                        color.g /= 2;
                        color.b /= 2;
                    }

                    pixels[index]   = color.r;
                    pixels[index+1] = color.g;
                    pixels[index+2] = color.b;
                }
                else
                {
                    pixels[index]=100; pixels[index+1]=100; pixels[index+2]=100;
                }
                pixels[index+3] = 255;
            }
        }
        // Enemies
        for (auto& e : enemies)
        {
            if (!e.alive) continue;
            // Direction et plan caméra corrects
            float dirX = std::cos(angle);
            float dirY = std::sin(angle);
            float planeX = -std::sin(angle) * std::tan(FOV / 2.0f);
            float planeY =  std::cos(angle) * std::tan(FOV / 2.0f);

            float ex = e.x - playerX;
            float ey = e.y - playerY;

            float invDet = 1.0f / (planeX * dirY - dirX * planeY);
            float transformX = invDet * (dirY * ex - dirX * ey);
            float transformY = invDet * (-planeY * ex + planeX * ey);

            // Si l'ennemi est derrière le joueur, skip
            if (transformY <= 0) continue;

            // Position à l'écran
            int spriteScreenX = (int)((SCREEN_W / 2) * (1 + transformX / transformY));

            // Taille du sprite
            int spriteH = std::abs((int)(SCREEN_H / transformY));
            int spriteW = spriteH;

            int topY = (SCREEN_H - spriteH) / 2;
            int botY = (SCREEN_H + spriteH) / 2;
            int leftX = spriteScreenX - spriteW / 2;
            int rightX = spriteScreenX + spriteW / 2;

            for (int x = leftX; x < rightX; x++)
            {
                // Hors écran ou derrière un mur
                if (x < 0 || x >= SCREEN_W) continue;
                if (transformY >= zBuffer[x]) continue;

                for (int y = topY; y < botY; y++)
                {
                    if (y < 0 || y >= SCREEN_H) continue;

                    int index = (y * SCREEN_W + x) * 4;

                    // Couleur simple verte pour l'instant
                    pixels[index]   = 0;
                    pixels[index+1] = 200;
                    pixels[index+2] = 0;
                    pixels[index+3] = 255;
                }
            }
        }

        // Gun drawing
        int gunW = 120;
        int gunH = 80;
        int gunBaseX = SCREEN_W / 2 - gunW / 2;
        int gunBaseY = (int)(SCREEN_H - gunH + gunY);

        for (int y = 0; y < gunH; y++)
        {
            for (int x = 0; x < gunW; x++)
            {
                int screenX = gunBaseX + x;
                int screenY = gunBaseY + y;

                if (screenX < 0 || screenX >= SCREEN_W || screenY < 0 || screenY >= SCREEN_H)
                    continue;

                int index = (screenY * SCREEN_W + screenX) * 4;

                // Canon
                if (x >= 50 && x <= 70 && y <= 20)
                {
                    pixels[index] = 80; pixels[index+1] = 80; pixels[index+2] = 80;
                }
                // Corps du fusil
                else if (x >= 20 && x <= 100 && y >= 15 && y <= 50)
                {
                    pixels[index] = 60; pixels[index+1] = 40; pixels[index+2] = 20;
                }
                // Crosse
                else if (x >= 60 && x <= 110 && y >= 45 && y <= 75)
                {
                    pixels[index] = 80; pixels[index+1] = 50; pixels[index+2] = 25;
                }
                else
                    continue; // transparent

                pixels[index+3] = 255;
            }
        }

        // Health bar background
        int barW = 200;
        int barH = 15;
        int barX = 10;
        int barY = SCREEN_H - 25;

        for (int y = barY; y < barY + barH; y++)
        {
            for (int x = barX; x < barX + barW; x++)
            {
                int index = (y * SCREEN_W + x) * 4;
                pixels[index]   = 60;
                pixels[index+1] = 0;
                pixels[index+2] = 0;
                pixels[index+3] = 255;
            }
        }

        // Health bar
        int hpW = (int)(barW * playerHP / 100.0f);
        for (int y = barY; y < barY + barH; y++)
        {
            for (int x = barX; x < barX + hpW; x++)
            {
                int index = (y * SCREEN_W + x) * 4;
                pixels[index]   = 200;
                pixels[index+1] = 0;
                pixels[index+2] = 0;
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

        // Enemies on minimap
        for (auto& e : enemies)
        {
            if (!e.alive) continue;
            sf::CircleShape dot(4);
            dot.setFillColor(sf::Color::Green);
            dot.setPosition(sf::Vector2f(e.x * TILE - 4, e.y * TILE - 4));
            window.draw(dot);
        }

        
        window.display();
    }


    return 0;
}