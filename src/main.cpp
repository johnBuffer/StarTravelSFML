#include <SFML/Graphics.hpp>
#include <random>
#include <iostream>
#include "events.hpp"
#include "configuration.hpp"
#include "star.hpp"

std::vector<Star> createStars(uint32_t count, float scale)
{
    std::vector<Star> stars;
    stars.reserve(count);

    // Random numbers generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    // Define a star free zone
    sf::Vector2f const window_world_size = conf::window_size_f * conf::near;
    sf::FloatRect const star_free_zone = {-window_world_size * 0.25f, window_world_size * 0.5f};

    // Create randomly distributed stars on the screen
    for (size_t i{count}; i--;)
    {
        //float const t = (i - start);
        float const x = (dis(gen) - 0.5f) * conf::window_size_f.x * scale;
        float const y = (dis(gen) - 0.5f) * conf::window_size_f.y * scale;
        float const z = (conf::far - conf::near) * dis(gen) + conf::near;

        // Discard any star that falls in the zone
        if (star_free_zone.contains(x, y))
        {
            ++i;
            continue;
        }

        // Else add it in the vector
        stars.push_back({{x, y}, z});
    }

    // Depth ordering
    std::sort(stars.begin(), stars.end(), [](Star const& s_1, Star const& s_2) {
        return s_1.z > s_2.z;
    });

    return stars;
}


float fastPow(float x, uint32_t p)
{
    float res = 1.0f;

    for (uint32_t i{p}; i--;)
    {
        res *= x;
    }

    return res;
}


void updateGeometry(uint32_t idx, Star const& s, sf::VertexArray& va)
{
    float const scale = 1.0f / s.z;
    float const depth_ratio = (s.z - conf::near) / (conf::far - conf::near);
    float const color_ratio = 1.0f - depth_ratio;
    auto const c = static_cast<uint8_t>(fastPow(color_ratio, 1) * 255.0f);

    sf::Vector2f const p = s.position * scale;
    float const r = conf::radius * scale;
    uint32_t const i = 4 * idx;

    va[i + 0].position = {p.x - r, p.y - r};
    va[i + 1].position = {p.x + r, p.y - r};
    va[i + 2].position = {p.x + r, p.y + r};
    va[i + 3].position = {p.x - r, p.y + r};

    sf::Color const color{c, c, c};
    va[i + 0].color = color;
    va[i + 1].color = color;
    va[i + 2].color = color;
    va[i + 3].color = color;
}


int main()
{
    auto window = sf::RenderWindow{ {conf::window_size.x, conf::window_size.y}, "CMake SFML Project", sf::Style::Fullscreen };
    window.setFramerateLimit(conf::max_framerate);
    window.setMouseCursorVisible(false);

    sf::Texture texture;
    texture.loadFromFile("res/star.png");
    texture.setSmooth(true);
    texture.generateMipmap();

    std::vector<Star> stars = createStars(conf::count, conf::far);

    sf::VertexArray va{sf::PrimitiveType::Quads, 4 * conf::count};
    // Pre fill texture coords as they will remain constant
    auto const texture_size_f = static_cast<sf::Vector2f>(texture.getSize());
    for (uint32_t idx{conf::count}; idx--;)
    {
        uint32_t const i = 4 * idx;
        va[i + 0].texCoords = {0.0f, 0.0f};
        va[i + 1].texCoords = {texture_size_f.x, 0.0f};
        va[i + 2].texCoords = {texture_size_f.x, texture_size_f.y};
        va[i + 3].texCoords = {0.0f, texture_size_f.y};
    }

    uint32_t first = 0;
    while (window.isOpen())
    {
        processEvents(window);

        // Fake travel toward increasing Z
        for (uint32_t i{conf::count}; i--;)
        {
            Star& s = stars[i];
            s.z -= conf::speed * conf::dt;
            if (s.z < conf::near)
            {
                s.z = conf::far - (conf::near - s.z);
                first = i;
            }
        }

        // Rendering occurs here
        window.clear();

        sf::CircleShape shape{conf::radius};
        shape.setOrigin(conf::radius, conf::radius);
        for (uint32_t i{0}; i < conf::count; ++i)
        {
            uint32_t const idx = (i + first) % conf::count;
            Star const& s = stars[idx];
            updateGeometry(i, s, va);
        }

        sf::RenderStates states;
        states.transform.translate(conf::window_size_f * 0.5f);
        states.texture = &texture;
        window.draw(va, states);

        window.display();
    }
}