#include <SFML/Graphics.hpp>
#include <CMath>

const int MAP_W = 8;
const int MAP_H = 8;

const float MOVE_SPEED = 3.0f;
const float ROTATE_SPEED = 3.0f;


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
        float dt = clock.restart().asSeconds();
        window.setTitle("x: " + std::to_string(playerX) + " y: " + std::to_string(playerY) + " angle: " + std::to_string(angle));

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        
    //Rotation et déplacement du joueur
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        angle -= ROTATE_SPEED * dt;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        angle += ROTATE_SPEED * dt;

    float dx = std::cos(angle) * MOVE_SPEED * dt;
    float dy = std::sin(angle) * MOVE_SPEED * dt;

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

        window.clear(sf::Color::Blue);
        window.display();
    }


    return 0;
}