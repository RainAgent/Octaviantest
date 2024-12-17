#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <map>
#include <iostream>

// Определение символов на барабанах
std::vector<std::string> symbols = { "A", "B", "C", "D", "E" };

const int rows = 3;
const int cols = 5;
const int symbolSize = 150; // Размер символов в пикселях (предполагается квадрат)
const int frameWidth = cols * symbolSize;
const int frameHeight = rows * symbolSize;
const float maxSpeed = 600.0f;
const float acceleration = 200.0f;
const float deceleration = 50.0f; // Уменьшение замедления для более плавной остановки
const float minSpeed = 100.0f; // Минимальная скорость для плавной остановки

enum class State {
    Waiting,
    Spinning,
    Decelerating,
    Stopped
};

// Генерация случайного результата спина
std::vector<std::vector<std::string>> spinReels() {
    std::vector<std::vector<std::string>> reels(rows, std::vector<std::string>(cols));
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int index = rand() % symbols.size();
            reels[row][col] = symbols[index];
        }
    }
    return reels;
}

int calculatePayout(const std::vector<std::vector<std::string>>& reels) {
    int payout = 0;
    // Проверка совпадений в строках
    for (int row = 0; row < rows; ++row) {
        bool rowMatch = true;
        for (int col = 1; col < cols; ++col) {
            if (reels[row][col] != reels[row][0]) {
                rowMatch = false;
                break;
            }
        }
        if (rowMatch) {
            payout += 50;  // Выплата за совпадение в строке
        }
    }

    // Проверка совпадений в столбцах
    for (int col = 0; col < cols; ++col) {
        bool colMatch = true;
        for (int row = 1; row < rows; ++row) {
            if (reels[row][col] != reels[0][col]) {
                colMatch = false;
                break;
            }
        }
        if (colMatch) {
            payout += 50;  // Выплата за совпадение в столбце
        }
    }

    return payout;
}

int main() {
    srand(static_cast<unsigned>(time(0)));

    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Slot Machine");
    int balance = 100;
    int bet = 10;

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        std::cerr << "Ошибка загрузки шрифта." << std::endl;
        return -1;
    }

    // Текст баланса и выплат
    sf::Text balanceText("Balance: " + std::to_string(balance), font, 24);
    balanceText.setFillColor(sf::Color::Black);
    balanceText.setPosition(10, 10);

    sf::Text resultText("", font, 24);
    resultText.setFillColor(sf::Color::Black);
    resultText.setPosition(10, 50);

    // Загрузка текстур
    std::map<std::string, sf::Texture> textures;
    for (const auto& symbol : symbols) {
        sf::Texture texture;
        if (!texture.loadFromFile(symbol + ".png")) {
            std::cerr << "Ошибка загрузки текстуры: " << symbol << ".png" << std::endl;
            return -3;
        }
        textures[symbol] = std::move(texture);
    }

    // Создание барабанов
    std::vector<std::vector<sf::Sprite>> reelSprites(rows + 2, std::vector<sf::Sprite>(cols));
    std::vector<std::vector<std::string>> reels = spinReels();

    for (int row = 0; row < rows + 2; ++row) {
        for (int col = 0; col < cols; ++col) {
            int textureIndex = rand() % symbols.size();
            reelSprites[row][col].setTexture(textures[symbols[textureIndex]]);
            reelSprites[row][col].setPosition(100 + col * symbolSize, 150 + (row - 1) * symbolSize);
        }
    }

    // Создание рамки
    sf::RectangleShape frame(sf::Vector2f(frameWidth, frameHeight));
    frame.setPosition(100, 150);
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineColor(sf::Color::Black);
    frame.setOutlineThickness(5);

    // Создание маски
    sf::RenderTexture renderTexture;
    renderTexture.create(frameWidth, frameHeight);

    sf::Sprite maskedSprite(renderTexture.getTexture());
    maskedSprite.setPosition(100, 150);

    // Кнопки
    sf::RectangleShape startButton(sf::Vector2f(100, 50));
    startButton.setFillColor(sf::Color::Green);
    startButton.setPosition(1650, 200);

    sf::Text startButtonText("START", font, 24);
    startButtonText.setFillColor(sf::Color::Black);
    startButtonText.setPosition(1660, 210);

    sf::RectangleShape stopButton(sf::Vector2f(100, 50));
    stopButton.setFillColor(sf::Color::Red);
    stopButton.setPosition(1650, 500);

    sf::Text stopButtonText("STOP", font, 24);
    stopButtonText.setFillColor(sf::Color::Black);
    stopButtonText.setPosition(1660, 510);

    State state = State::Waiting;
    sf::Clock clock;
    sf::Clock autoStopClock; // Часы для автоматической остановки
    float currentSpeed = 0.0f; // Текущая скорость
    float offset = 0.0f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (startButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos)) && state == State::Waiting) {
                    state = State::Spinning; // Сразу начинаем вращение
                    clock.restart();
                    autoStopClock.restart(); // Сброс часов для автоматической остановки
                    offset = 0.0f;
                    reels = spinReels(); // Генерация новых символов
                    currentSpeed = maxSpeed; // Устанавливаем максимальную скорость сразу
                }
                if (stopButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos)) && state == State::Spinning) {
                    state = State::Decelerating;
                }
            }
        }

        if (state == State::Spinning) {
            float delta = clock.restart().asSeconds();
            offset += currentSpeed * delta;

            if (offset >= symbolSize) {
                offset -= symbolSize;

                // Сдвигаем символы циклически
                for (int col = 0; col < cols; ++col) {
                    for (int row = rows + 1; row > 0; --row) {
                        reelSprites[row][col].setTexture(*reelSprites[row - 1][col].getTexture());
                    }
                    // Устанавливаем новый символ сверху
                    reelSprites[0][col].setTexture(textures[symbols[rand() % symbols.size()]]);
                }
            }

            if (autoStopClock.getElapsedTime().asSeconds() >= 30) { // Увеличено до 30 секунд
                state = State::Decelerating;
            }
        }

        if (state == State::Decelerating) {
            float delta = clock.restart().asSeconds();
            currentSpeed -= deceleration * delta;

            if (currentSpeed <= minSpeed) {
                currentSpeed = minSpeed;
                state = State::Stopped;
            }

            // Плавное замедление с сохранением смещения
            offset += currentSpeed * delta;
            if (offset >= symbolSize) {
                offset -= symbolSize;
                // Сдвигаем символы циклически
                for (int col = 0; col < cols; ++col) {
                    for (int row = rows + 1; row > 0; --row) {
                        reelSprites[row][col].setTexture(*reelSprites[row - 1][col].getTexture());
                    }
                    // Устанавливаем новый символ сверху
                    reelSprites[0][col].setTexture(textures[symbols[rand() % symbols.size()]]);
                }
            }
        }
        if (state == State::Stopped) {
            balance -= bet;
            int payout = calculatePayout(reels);
            balance += payout;
            resultText.setString("Payout: " + std::to_string(payout));
            balanceText.setString("Balance: " + std::to_string(balance));

            // Возвращаем символы на заданные линии
            for (int row = 0; row < rows; ++row) {
                for (int col = 0; col < cols; ++col) {
                    reelSprites[row + 1][col].setTexture(textures[reels[row][col]]);
                }
            }

            // Устанавливаем символы точно по позициям
            for (int col = 0; col < cols; ++col) {
                for (int row = 0; row < rows + 2; ++row) {
                    reelSprites[row][col].setPosition(100 + col * symbolSize, 150 + (row - 1) * symbolSize);
                }
            }

            state = State::Waiting;
        }

        window.clear(sf::Color::White);
        window.draw(balanceText);
        window.draw(resultText);

        // Ограничение для рамки
        renderTexture.clear(sf::Color::Transparent);
        for (int row = 0; row < rows + 2; ++row) {
            for (int col = 0; col < cols; ++col) {
                reelSprites[row][col].setPosition(col * symbolSize, (row - 1) * symbolSize - offset);
                renderTexture.draw(reelSprites[row][col]);
            }
        }
        renderTexture.display();

        window.draw(maskedSprite);
        window.draw(frame);  // Отрисовка рамки

        window.draw(startButton);
        window.draw(startButtonText);
        window.draw(stopButton);
        window.draw(stopButtonText);

        window.display();
    }

    return 0;
}
