#include <SFML/Graphics.hpp>
#include <CMath>
#include <string>

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

    while (window.isOpen())
    {
        window.clear(sf::Color::Blue);
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
        // Angle of the ray depending on column
        float rayAngle = angle - FOV /2.0f + FOV * col / float(SCREEN_W);
        
        // Direction vector of the ray
        float rayDirX = std::cos(rayAngle);
        float rayDirY = std::sin(rayAngle);

        bool hit = false;
        float dist = 0.0f;
        float rayX = 0.0f;
        float rayY = 0.0f;

        while (!hit && dist < MAX_DEATH){
            dist += 0.05f;
            rayX = playerX + rayDirX * dist;
            rayY = playerY + rayDirY * dist;

            if (map[int(rayY)][int(rayX)] == 1)
                hit = true;
            
        }
        
        // Remove fish-eye effect
        dist *= std::cos(rayAngle - angle); 

        // Wall height on screen
        int wallH = (int)(SCREEN_H / dist);
        int top = (SCREEN_H - wallH) / 2;
        int bot = (SCREEN_H + wallH) / 2;

        // Ceiling drawing
        sf::RectangleShape ceiling(sf::Vector2f(1, top));
        ceiling.setPosition(sf::Vector2f(col, 0));
        ceiling.setFillColor(sf::Color(50, 50, 50));
        window.draw(ceiling);

        // Wall drawing
        int brightness = (int)(255 / (1 + dist * dist * 0.1f));
        brightness = std::max(0, std::min(255, brightness)); // To be sure it is between 0 and 255
        sf::RectangleShape wall(sf::Vector2f(1, wallH));
        wall.setPosition(sf::Vector2f(col, top));
        wall.setFillColor(sf::Color(brightness, brightness, brightness));
        window.draw(wall);

        // Floor drawing
        sf::RectangleShape floor(sf::Vector2f(1, SCREEN_H - bot));
        floor.setPosition(sf::Vector2f(col, bot));
        floor.setFillColor(sf::Color(100, 100, 100));
        window.draw(floor);

        
        
    }
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