#include <SFML/Graphics.hpp>
#include <CMath>
#include <string>
#include <cstdint>
#include <algorithm> // for std::remove_if

const int MAP_W = 8;
const int MAP_H = 8;

const float MOVE_SPEED = 3.0f;
const float ROTATE_SPEED = 3.0f;

const float FOV = 3.14159f / 3.0f; // fov 60°
const int SCREEN_W = 800;
const int SCREEN_H = 600;

const float MAX_DEATH = 20.0f;

//  Config sprite sheets weapon
// Each file is a horizontal strip of N frames side by side
const int   IDLE_FRAMES       = 2;     // frames dans weapon_idle.png
const int   SHOOT_FRAMES      = 3;     // frames dans weapon_shoot.png
const int   RELOAD_FRAMES     = 4;     // frames dans weapon_reload.png
const float IDLE_FRAME_DUR    = 0.20f;
const float SHOOT_FRAME_DUR   = 0.06f;
const float RELOAD_FRAME_DUR  = 0.09f;
const int   GUN_DISPLAY_W     = 200;   // taille d'affichage à l'écran (pixels)
const int   GUN_DISPLAY_H     = 150;

// Projectile config
const float PROJ_SPEED = 12.0f; // cases/seconde
const float PROJ_SCALE = 0.25f; // facteur de taille du billboard


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

struct Projectile {
    float x, y;   // position monde
    float dx, dy; // direction normalisée
    bool  alive;
};


std::vector<Enemy> enemies = {
    {2.0f, 2.0f, true},
    {5.0f, 2.0f, true},
    {5.0f, 5.0f, true},
};

// draw weapon sprite from the given sheet and frame, with vertical offset
std::vector<Projectile> projectiles;

void drawWeaponSprite(
    std::vector<uint8_t>& pixels,
    const sf::Image& sheet,
    int frame, int nFrames,
    float yOffset)
{
    sf::Vector2u sz = sheet.getSize();
    int frameW = sz.x / nFrames;
    int frameH = sz.y;

    int baseX = SCREEN_W / 2 - GUN_DISPLAY_W / 2;
    int baseY = (int)(SCREEN_H - GUN_DISPLAY_H + yOffset);

    for (int y = 0; y < GUN_DISPLAY_H; y++)
    {
        for (int x = 0; x < GUN_DISPLAY_W; x++)
        {
            int sx = baseX + x;
            int sy = baseY + y;
            if (sx < 0 || sx >= SCREEN_W || sy < 0 || sy >= SCREEN_H) continue;

            int texX = frame * frameW + x * frameW / GUN_DISPLAY_W;
            int texY = y * frameH / GUN_DISPLAY_H;
            texX = std::max(0, std::min((int)sz.x - 1, texX));
            texY = std::max(0, std::min((int)sz.y - 1, texY));

            sf::Color c = sheet.getPixel(sf::Vector2u(texX, texY));
            if (c.a < 128) continue; // transparent → skip

            int index = (sy * SCREEN_W + sx) * 4;
            pixels[index]   = c.r;
            pixels[index+1] = c.g;
            pixels[index+2] = c.b;
            pixels[index+3] = 255;
        }
    }
}

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

    // Wall texture
    sf::Image wallTexture;
    if (!wallTexture.loadFromFile("wall.png"))
        return -1;
    sf::Vector2u texSize = wallTexture.getSize();

    // Enemy texture
    sf::Image enemyTexture;
    if (!enemyTexture.loadFromFile("enemy.png"))
        return -1;
    sf::Vector2u enemyTexSize = enemyTexture.getSize();

    // Projectile texture
    sf::Image projTexture;
    if (!projTexture.loadFromFile("projectile.png"))
        return -1;
    sf::Vector2u projTexSize = projTexture.getSize();

    // Sprite sheets of the gun
    sf::Image sheetIdle, sheetShoot, sheetReload;
    if (!sheetIdle  .loadFromFile("weapon_idle.png"))   return -1;
    if (!sheetShoot .loadFromFile("weapon_shoot.png"))  return -1;
    if (!sheetReload.loadFromFile("weapon_reload.png")) return -1;

    // Gun
    enum GunState { IDLE, SHOOT, RELOAD };
    GunState gunState = IDLE;
    float gunY = 0.0f;        // vertical offset for the gun bob/shoot animation
    float gunTimer = 0.0f;    // animation timer
    int   gunFrame = 0; // actual frame of the current animation

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

        // Shoot detection
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && gunState == IDLE)
        {
            gunState = SHOOT;
            gunFrame = 0; // AJOUT : repart de la première frame
            gunTimer = 0.0f;

            // AJOUT : spawn un projectile dans la direction du regard
            Projectile p;
            p.x     = playerX;
            p.y     = playerY;
            p.dx    = std::cos(angle);
            p.dy    = std::sin(angle);
            p.alive = true;
            projectiles.push_back(p);
        }

        // Animation
        gunTimer += dt; // MODIF : déplacé ici pour être commun aux 3 états
        if (gunState == SHOOT)
        {
            gunY += 300.0f * dt;  // recul vers le bas
            if (gunTimer >= SHOOT_FRAME_DUR)
            {
                gunTimer -= SHOOT_FRAME_DUR;
                gunFrame++;
                if (gunFrame >= SHOOT_FRAMES)
                {
                    gunState = RELOAD;
                    gunFrame = 0;
                    gunTimer = 0.0f;
                }
            }
        }
        else if (gunState == RELOAD)
        {
            gunY -= 200.0f * dt;  // redescend doucement
            if (gunY < 0.0f) gunY = 0.0f;
            if (gunTimer >= RELOAD_FRAME_DUR)
            {
                gunTimer -= RELOAD_FRAME_DUR;
                gunFrame++;
                if (gunFrame >= RELOAD_FRAMES)
                {
                    gunY = 0.0f;
                    gunState = IDLE;
                    gunFrame = 0;
                    gunTimer = 0.0f;
                }
            }
        }
        else // IDLE
        {
            // Légère oscillation de la main (bob)
            gunY = std::sin(gunTimer * 3.0f) * 5.0f;
            if (gunTimer >= IDLE_FRAME_DUR)
            {
                gunTimer -= IDLE_FRAME_DUR;
                gunFrame = (gunFrame + 1) % IDLE_FRAMES;
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
       
        // AJOUT : mise à jour des projectiles
        for (auto& p : projectiles)
        {
            if (!p.alive) continue;

            p.x += p.dx * PROJ_SPEED * dt;
            p.y += p.dy * PROJ_SPEED * dt;

            // Collision mur
            if (map[(int)p.y][(int)p.x] == 1)
            {
                p.alive = false;
                continue;
            }

            // Collision ennemis
            for (auto& e : enemies)
            {
                if (!e.alive) continue;
                float ex = e.x - p.x;
                float ey = e.y - p.y;
                if (ex * ex + ey * ey < 0.3f * 0.3f)
                {
                    e.alive = false;
                    p.alive = false;
                    break;
                }
            }
        }
        // Nettoyage des projectiles morts
        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                [](const Projectile& p){ return !p.alive; }),
            projectiles.end());
       
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
        // sort enemies by distance to player (farther first)
        std::sort(enemies.begin(), enemies.end(), [&](const Enemy& a, const Enemy& b)
        {
            float da = (a.x - playerX) * (a.x - playerX) + (a.y - playerY) * (a.y - playerY);
            float db = (b.x - playerX) * (b.x - playerX) + (b.y - playerY) * (b.y - playerY);
            return da > db; // plus loin d'abord
        });
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

                // Quelle colonne de texture ?
                int texX = (int)((x - leftX) * enemyTexSize.x / spriteW);
                int texXClamped = std::max(0, std::min((int)enemyTexSize.x - 1, texX));

                for (int y = topY; y < botY; y++)
                {
                    if (y < 0 || y >= SCREEN_H) continue;

                    int texY = (int)((y - topY) * enemyTexSize.y / spriteH);
                    texY = std::max(0, std::min((int)enemyTexSize.y - 1, texY));

                    sf::Color color = enemyTexture.getPixel(sf::Vector2u(texXClamped, texY)); // ✅ utilise texXClamped
                    if (color.a < 128) continue;

                    int index = (y * SCREEN_W + x) * 4;
                    pixels[index]   = color.r;
                    pixels[index+1] = color.g;
                    pixels[index+2] = color.b;
                    pixels[index+3] = 255;
                }
            }
        }
        
        // AJOUT : Projectiles (billboard, même logique que les ennemis)
        for (auto& p : projectiles)
        {
            if (!p.alive) continue;

            float dirX = std::cos(angle);
            float dirY = std::sin(angle);
            float planeX = -std::sin(angle) * std::tan(FOV / 2.0f);
            float planeY =  std::cos(angle) * std::tan(FOV / 2.0f);

            float ex = p.x - playerX;
            float ey = p.y - playerY;

            float invDet = 1.0f / (planeX * dirY - dirX * planeY);
            float transformX = invDet * (dirY * ex - dirX * ey);
            float transformY = invDet * (-planeY * ex + planeX * ey);

            if (transformY <= 0) continue;

            int spriteScreenX = (int)((SCREEN_W / 2) * (1 + transformX / transformY));

            // PROJ_SCALE rend le poisson plus petit que les ennemis
            int spriteH = std::abs((int)(SCREEN_H / transformY * PROJ_SCALE));
            int spriteW = spriteH;

            int topY  = (SCREEN_H - spriteH) / 2;
            int botY  = (SCREEN_H + spriteH) / 2;
            int leftX = spriteScreenX - spriteW / 2;
            int rightX= spriteScreenX + spriteW / 2;

            for (int x = leftX; x < rightX; x++)
            {
                if (x < 0 || x >= SCREEN_W) continue;
                if (transformY >= zBuffer[x]) continue;

                int texX = (int)((x - leftX) * projTexSize.x / spriteW);
                int texXClamped = std::max(0, std::min((int)projTexSize.x - 1, texX));

                for (int y = topY; y < botY; y++)
                {
                    if (y < 0 || y >= SCREEN_H) continue;

                    int texY = (int)((y - topY) * projTexSize.y / spriteH);
                    texY = std::max(0, std::min((int)projTexSize.y - 1, texY));

                    sf::Color color = projTexture.getPixel(sf::Vector2u(texXClamped, texY));
                    if (color.a < 128) continue;

                    int index = (y * SCREEN_W + x) * 4;
                    pixels[index]   = color.r;
                    pixels[index+1] = color.g;
                    pixels[index+2] = color.b;
                    pixels[index+3] = 255;
                }
            }
        }
        // Crosshair
        int cx = SCREEN_W / 2;
        int cy = SCREEN_H / 2;
        int size = 10;

        for (int x = cx - size; x <= cx + size; x++)
        {
            if (x < 0 || x >= SCREEN_W) continue;
            int index = (cy * SCREEN_W + x) * 4;
            pixels[index] = 255; pixels[index+1] = 255; pixels[index+2] = 255; pixels[index+3] = 255;
        }
        for (int y = cy - size; y <= cy + size; y++)
        {
            if (y < 0 || y >= SCREEN_H) continue;
            int index = (y * SCREEN_W + cx) * 4;
            pixels[index] = 255; pixels[index+1] = 255; pixels[index+2] = 255; pixels[index+3] = 255;
        }


        // MODIF : Gun drawing — remplace le dessin procédural par le sprite sheet
        {
            const sf::Image* sheet = &sheetIdle;
            int nFrames = IDLE_FRAMES;
            if      (gunState == SHOOT)  { sheet = &sheetShoot;  nFrames = SHOOT_FRAMES;  }
            else if (gunState == RELOAD) { sheet = &sheetReload; nFrames = RELOAD_FRAMES; }

            int safeFrame = std::min(gunFrame, nFrames - 1);
            drawWeaponSprite(pixels, *sheet, safeFrame, nFrames, gunY);
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

        // AJOUT : Projectiles on minimap
        for (auto& p : projectiles)
        {
            if (!p.alive) continue;
            sf::CircleShape dot(2);
            dot.setFillColor(sf::Color::Yellow);
            dot.setPosition(sf::Vector2f(p.x * TILE - 2, p.y * TILE - 2));
            window.draw(dot);
        }

        
        window.display();
    }


    return 0;
}