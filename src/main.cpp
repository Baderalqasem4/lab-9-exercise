#include <iostream>
#include <vector>

#include "camera.h"
#include "dynamic_camera.h"
#include <SFML/Graphics.hpp>

constexpr size_t WINDOW_WIDTH = 1280;
constexpr size_t WINDOW_HEIGHT = 720;
constexpr float LINE_WIDTH = 4;
constexpr float PLAYER_WIDTH = 50;
constexpr float PLAYER_SPEED = 20;
constexpr float GRAVITY = 4;
constexpr float JUMP_SPEED = -50;

constexpr float FLOOR_HEIGHT = 140;
constexpr float WORLD_TEX_WIDTH = 576;
constexpr float WORLD_TEX_HEIGHT = 160;
constexpr float WORLD_WIDTH = WORLD_TEX_WIDTH * (WINDOW_HEIGHT / WORLD_TEX_HEIGHT);
constexpr float WORLD_HEIGHT = WINDOW_HEIGHT;
constexpr float SOFT_LIMIT_PERCENT = 1.f / 3;
constexpr float HARD_LIMIT_PERCENT = 1.f / 5;

constexpr float SOFT_CAMERA_ALPHA = 0.1f;

struct Player {
    Player()
        : position(WINDOW_WIDTH / 2, WINDOW_HEIGHT - FLOOR_HEIGHT),
          velocity{},
          shape({PLAYER_WIDTH, PLAYER_WIDTH}),
          isJumping{false} {
        shape.setFillColor(sf::Color::Green);
        shape.setOrigin({PLAYER_WIDTH / 2, PLAYER_WIDTH});
        shape.setPosition({WINDOW_WIDTH / 2, WINDOW_HEIGHT - FLOOR_HEIGHT});
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        target.draw(shape, states);
    }

    // should have a deltaTime here if we didn't have fixed fps.
    void update() {
        position += velocity;
        position.x = std::clamp(position.x, PLAYER_WIDTH * 0.5f, WORLD_WIDTH - PLAYER_WIDTH * 0.5f);
        if (position.y >= WINDOW_HEIGHT - FLOOR_HEIGHT) {
            position.y = WINDOW_HEIGHT - FLOOR_HEIGHT;
        }

        shape.setPosition(position);
    }

    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::RectangleShape shape;
    bool isJumping;
};

int main() {
    sf::RenderWindow window(sf::VideoMode{{WINDOW_WIDTH, WINDOW_HEIGHT}}, "Cameras");
    window.setFramerateLimit(60);

    // Load background
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("assets/grunge-tileset.png")) {
        return 1;
    }
    sf::Sprite backgroundSprite(backgroundTexture);
    float backgroundScaleFactor = WORLD_HEIGHT / WORLD_TEX_HEIGHT;
    backgroundSprite.setScale({backgroundScaleFactor, backgroundScaleFactor});

    sf::RectangleShape leftSoftLine({4, 720});
    leftSoftLine.setFillColor(sf::Color(0, 255, 0, 128));
    leftSoftLine.setOrigin({LINE_WIDTH / 2, 0});
    leftSoftLine.setPosition({WINDOW_WIDTH / 3.f, 0});

    sf::RectangleShape rightSoftLine({4, 720});
    rightSoftLine.setFillColor(sf::Color(0, 255, 0, 128));
    rightSoftLine.setOrigin({LINE_WIDTH / 2, 0});
    rightSoftLine.setPosition({WINDOW_WIDTH * 2.f / 3.f, 0});

    sf::RectangleShape leftHardLine({4, 720});
    leftHardLine.setFillColor(sf::Color(255, 0, 0, 128));
    leftHardLine.setOrigin({LINE_WIDTH / 2, 0});
    leftHardLine.setPosition({WINDOW_WIDTH * HARD_LIMIT_PERCENT, 0});

    sf::RectangleShape rightHardLine({4, 720});
    rightHardLine.setFillColor(sf::Color(255, 0, 0, 128));
    rightHardLine.setOrigin({LINE_WIDTH / 2, 0});
    rightHardLine.setPosition({WINDOW_WIDTH * (1.f - HARD_LIMIT_PERCENT), 0});

    sf::RectangleShape target({10, 10});
    target.setFillColor(sf::Color::Yellow);
    target.setOrigin({5.f, 5.f});
    target.setPosition({-10.f, -10.f});  // hide at first.

    Player player;

    sf::View worldView = window.getView();
    sf::View guiView = window.getView();
    Camera camera(sf::Vector2f(window.getSize()), {WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f});
    DynamicCamera dynamicCam{.alpha = SOFT_CAMERA_ALPHA,
                             .deadZoneX = {SOFT_LIMIT_PERCENT, 1.f - SOFT_LIMIT_PERCENT},
                             .softZoneX = {HARD_LIMIT_PERCENT, 1.f - HARD_LIMIT_PERCENT},
                             .lookAhead = 0.3f,
                             .cameraUnadjustedCenter = camera.getCameraCenter()};
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                return 0;
            } else if (auto keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scan::Q) {
                    return 0;
                }
            }
        }

        float ax = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) {
            ax += PLAYER_SPEED;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) {
            ax -= PLAYER_SPEED;
        }
        player.velocity.x = ax;
        if ((player.position.y >= WINDOW_HEIGHT - FLOOR_HEIGHT)) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) {
                player.velocity.y += JUMP_SPEED;
            }
        } else {
            player.velocity.y += GRAVITY;
        }
        player.update();
        // camera
        dynamicCam.update(camera, player.position, player.velocity);
        // update gui
        leftSoftLine.setPosition({(float)WINDOW_WIDTH * dynamicCam.deadZoneX.min, 0.f});
        leftHardLine.setPosition({(float)WINDOW_WIDTH * dynamicCam.softZoneX.min, 0.f});
        rightHardLine.setPosition({(float)WINDOW_WIDTH * dynamicCam.softZoneX.max, 0.f});
        rightSoftLine.setPosition({(float)WINDOW_WIDTH * dynamicCam.deadZoneX.max, 0.f});

        worldView.setCenter(camera.getCameraCenter());
        window.clear(sf::Color::Black);
        // world rendering
        window.setView(worldView);
        window.draw(backgroundSprite);
        player.draw(window, sf::RenderStates{});
        window.draw(target);
        // gui rendering
        window.setView(guiView);
        window.draw(leftSoftLine);
        window.draw(rightSoftLine);
        window.draw(leftHardLine);
        window.draw(rightHardLine);
        // display to screen
        window.display();
    }
    return 0;
}
