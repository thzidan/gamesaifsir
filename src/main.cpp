#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <random>
#include "raylib.h"
#include <bits/stdc++.h>

static const int screenWidth = 600;
static const int screenHeight = 800;
static const float scrollingSpeed = 2.0f;
static const int maxLives = 3;
static const int maxEnemies = 10;

// Function to generate a random float in a specified range
float GetRandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min);
}

class Background {
private:
    Texture2D background;
    float scrollingBack;

public:
    Background() {
        background = LoadTexture("assets/background.png");
        scrollingBack = 0.0f;
    }

    void Draw() {
        scrollingBack += scrollingSpeed;

        if (scrollingBack >= background.height) {
            scrollingBack = 0;
        }

        DrawTextureEx(background, (Vector2){0, scrollingBack}, 0.0f, 1.0f, WHITE);
        DrawTextureEx(background, (Vector2){0, scrollingBack - background.height}, 0.0f, 1.0f, WHITE);
    }

    ~Background() {
        UnloadTexture(background);
    }
};

class Player {
private:
    float initialX;
    float initialY;
    float speedX;

public:
    int lives;
    Texture2D playerimg;

    Player() {
        playerimg = LoadTexture("assets/spaceship.png");
        initialX = screenWidth / 2;
        initialY = screenHeight - playerimg.height - 150;
        speedX = 10;
        lives = maxLives;
    }

    void Draw() {
        if (IsKeyDown(KEY_RIGHT)) initialX += speedX;
        if (IsKeyDown(KEY_LEFT)) initialX -= speedX;

        if (initialX <= 0) initialX = 0;
        if (initialX + playerimg.width >= screenWidth) initialX = screenWidth - playerimg.width;

        DrawTextureEx(playerimg, (Vector2){initialX, initialY}, 0.0f, 1.0f, WHITE);
    }

    float GetX() const {
        return initialX;
    }

    float GetY() const {
        return initialY;
    }

    void LoseLife() {
        lives--;
    }

    bool IsAlive() const {
        return lives > 0;
    }

    void Reset() {
        initialX = screenWidth / 2;
        initialY = screenHeight - playerimg.height - 150;
        lives = maxLives;
    }

    ~Player() {
        UnloadTexture(playerimg);
    }
};

class Bullet {
public:
    Vector2 position;
    float speed;
    bool active;

    Bullet(float x, float y, float bulletSpeed) {
        position.x = x;
        position.y = y;
        speed = bulletSpeed;
        active = true;
    }

    void Update() {
        position.y -= speed;

        if (position.y < 0) {
            active = false;
        }
    }

    void Draw() {
        if (active) {
            DrawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), 5, 10, RED);
        }
    }
};

class EnemyShip {
private:
    Vector2 position;
    float speedY;
    bool active;
    int lives;

public:
    Texture2D enemyTexture;

    EnemyShip(const char* texturePath, float x, float y, float enemySpeed) {
        enemyTexture = LoadTexture(texturePath);
        position.x = x;
        position.y = y;
        speedY = enemySpeed;
        active = true;
        lives = 1;
    }

    void Update() {
        position.y += speedY;

        if (position.y > screenHeight) {
            active = false;
        }
    }

    void Draw() {
        if (active) {
            DrawTextureEx(enemyTexture, position, 0.0f, 1.0f, WHITE);
        }
    }

    bool IsActive() const {
        return active;
    }

    Vector2 GetPosition() const {
        return position;
    }

    void LoseLife() {
        lives--;
        if (lives <= 0) {
            active = false;
        }
    }
};

class Game {
public:
    Player player = Player();
    bool running = true;
    std::vector<Bullet> playerBullets;
    std::vector<EnemyShip> enemies;
    double lastEnemySpawnTime = 0.0;
    double enemySpawnInterval = 2.0; // Adjust this value for enemy spawn rate
    double lastEnemyShootTime = 0.0;
    double enemyShootInterval = 1.5; // Adjust this value for enemy shooting rate

    Game() {
    }

    ~Game() {
    }

    void Draw() {
        player.Draw();

        // Draw player bullets
        for (auto& bullet : playerBullets) {
            bullet.Draw();
        }

        // Draw enemies
        for (auto& enemy : enemies) {
            enemy.Draw();
        }

        // Draw player lives
        DrawText(("Lives: " + std::to_string(player.lives)).c_str(), 10, 10, 20, WHITE);
    }

    void Update() {
        if (running) {
            player.Draw();

            // Update player bullets
            for (auto& bullet : playerBullets) {
                bullet.Update();
            }

            // Update enemies
            for (auto& enemy : enemies) {
                enemy.Update();

                // Check if enemy collides with player
                if (CheckCollisionCircles(enemy.GetPosition(), 16, (Vector2){player.GetX() + player.playerimg.width / 2, player.GetY() + player.playerimg.height / 2}, 20) && player.IsAlive()) {
                    player.LoseLife();
                    enemy.LoseLife();
                }
            }

            // Remove inactive bullets from the vector
            playerBullets.erase(std::remove_if(playerBullets.begin(), playerBullets.end(), [](const Bullet& bullet) {
                return !bullet.active;
            }), playerBullets.end());

            // Remove inactive enemies from the vector
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const EnemyShip& enemy) {
                return !enemy.IsActive();
            }), enemies.end());

            // Spawn new enemies at regular intervals
            double currentTime = GetTime();
            if (currentTime - lastEnemySpawnTime >= enemySpawnInterval) {
                SpawnEnemy();
                lastEnemySpawnTime = currentTime;
            }

            // Enemy shooting logic
            for (auto& enemy : enemies) {
                if (currentTime - lastEnemyShootTime >= enemyShootInterval) {
                    ShootEnemyBullet(enemy);
                    lastEnemyShootTime = currentTime;
                }
            }
        }

        // Game over logic
        if (!player.IsAlive()) {
            running = false;
        }

        // Check bullet-enemy collisions
        CheckBulletEnemyCollisions();
        // Call SpawnEnemy here to create new enemies
        SpawnEnemy();
    }

    void SpawnEnemy() {
        if (enemies.size() < maxEnemies) {
            // Randomly choose an enemy ship texture
            const char* enemyTextures[] = {"assets/enemy1.png", "assets/enemy2.png", "assets/enemy3.png", "assets/enemy4.png"};
            const int numEnemyTextures = sizeof(enemyTextures) / sizeof(enemyTextures[0]);
            int randomTextureIndex = rand() % numEnemyTextures;

            float randomX = static_cast<float>(rand() % (screenWidth - 50));
            enemies.push_back(EnemyShip(enemyTextures[randomTextureIndex], randomX, -50, 1.0f));
        }
    }

    void ShootEnemyBullet(const EnemyShip& enemy) {
        float enemyX = enemy.GetPosition().x + enemy.enemyTexture.width / 2;
        float enemyY = enemy.GetPosition().y + enemy.enemyTexture.height / 2;
        float bulletSpeed = 5.0f;
        playerBullets.push_back(Bullet(enemyX, enemyY, bulletSpeed));
    }

    // Add a function to check bullet-enemy collisions
    void CheckBulletEnemyCollisions() {
        for (auto& bullet : playerBullets) {
            if (bullet.active) {
                for (auto& enemy : enemies) {
                    if (enemy.IsActive() && CheckCollisionRecs(
                        {(float)bullet.position.x, (float)bullet.position.y, 5, 10},
                        {(float)enemy.GetPosition().x, (float)enemy.GetPosition().y, enemy.enemyTexture.width, enemy.enemyTexture.height})) {
                        bullet.active = false;
                        enemy.LoseLife();
                    }
                }
            }
        }
    }
};

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "Starting the game..." << std::endl;
    InitWindow(screenWidth, screenHeight, "Space Invaders");
    SetTargetFPS(60);

    Background bg = Background();
    Game game = Game();

    while (!WindowShouldClose()) {
        game.Update();

        // Player shooting logic
        if (game.running && IsKeyPressed(KEY_SPACE)) {
            float playerX = game.player.GetX() + game.player.playerimg.width / 2;
            float playerY = game.player.GetY();
            float bulletSpeed = 5.0f;
            game.playerBullets.push_back(Bullet(playerX, playerY, bulletSpeed));
        }

        BeginDrawing();
        ClearBackground(DARKBLUE);
        bg.Draw();
        game.Draw();

        // Game over screen
        if (!game.running) {
            DrawText("Game Over", screenWidth / 2 - MeasureText("Game Over", 40) / 2, screenHeight / 2 - 40, 40, RED);
            DrawText(("Your Score: " + std::to_string(maxLives - game.player.lives)).c_str(), screenWidth / 2 - MeasureText(("Your Score: " + std::to_string(maxLives - game.player.lives)).c_str(), 20) / 2, screenHeight / 2 + 10, 20, WHITE);
            DrawText("Press R to Restart", screenWidth / 2 - MeasureText("Press R to Restart", 20) / 2, screenHeight / 2 + 40, 20, GREEN);
        }

        EndDrawing();

        // Restart the game when 'R' is pressed
        if (!game.running && IsKeyPressed(KEY_R)) {
            game = Game(); // Reset the game
        }
    }

    CloseWindow();
    return 0;
}
