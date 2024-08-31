#include <SFML/Graphics.hpp>
#include <random>


namespace conf
{
    sf::Vector2u const window_size = {2560, 1440};
    float const        far         = 100.0f;
    float const        near        = 0.1f;
    float const        radius      = 100.0f;
    float const        speed       = 1.0f;
    uint32_t const     stars_count = 600000;

    uint32_t const max_fps = 60;
    float const    dt      = 1.0f / max_fps;
}


struct Star
{
    sf::Vector3f position;
};


float fastPow(float x, uint32_t p)
{
    float res = 1.0f;
    for (uint32_t i{p}; i--;) {
        res *= x;
    }
    return res;
}


void processEvents(sf::Window& window)
{
    sf::Event event{};
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed) {
            window.close();
        } else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
        }
    }
}

void updateGeometry(uint32_t idx, Star const& s, sf::VertexArray& va)
{
    uint32_t const i{4 * idx};
    float const scale   = 1.0f / s.position.z;
    float const z_ratio = (s.position.z - conf::near) / (conf::far - conf::near);
    float const color_ratio = fastPow(1.0f - z_ratio, 2);
    auto const c = static_cast<uint8_t>(color_ratio * 255);

    sf::Vector2f const p{sf::Vector2f{s.position.x, s.position.y} * scale};
    float const radius = conf::radius * scale;
    va[i + 0].position = {p.x - radius, p.y - radius};
    va[i + 1].position = {p.x + radius, p.y - radius};
    va[i + 2].position = {p.x + radius, p.y + radius};
    va[i + 3].position = {p.x - radius, p.y + radius};

    sf::Color const color{c, c, c};
    va[i + 0].color = color;
    va[i + 1].color = color;
    va[i + 2].color = color;
    va[i + 3].color = color;
}

std::vector<Star> createStars(uint32_t count, float scale)
{
    std::vector<Star> stars;

    // Random numbers generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    sf::Vector2f const window_size_f = static_cast<sf::Vector2f>(conf::window_size);
    sf::FloatRect const dead_zone{-window_size_f * conf::near * 0.5f, window_size_f * conf::near};

    for (uint32_t i{count}; i--;) {
        float const x{(dis(gen) - 0.5f) * window_size_f.x * scale};
        float const y{(dis(gen) - 0.5f) * window_size_f.y * scale};

        if (dead_zone.contains(x, y)) {
            ++i;
            continue;
        }

        float const z{conf::near + dis(gen) * (conf::far - conf::near)};
        stars.push_back({{x, y, z}});
    }

    std::sort(stars.begin(), stars.end(), [](Star const& s_1, Star const& s_2) {
       return s_1.position.z > s_2.position.z;
    });

    return stars;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(conf::window_size.x, conf::window_size.y), "Star", sf::Style::Fullscreen);
    window.setFramerateLimit(0);

    sf::Texture star_texture;
    star_texture.loadFromFile("res/star.png");
    star_texture.setSmooth(true);
    star_texture.generateMipmap();

    std::vector<Star> stars = createStars(conf::stars_count, conf::far);

    auto const texture_size = static_cast<sf::Vector2f>(star_texture.getSize());
    sf::VertexArray va{sf::PrimitiveType::Quads, 4 * conf::stars_count};
    for (uint32_t i{0}; i < conf::stars_count; ++i) {
        va[4 * i + 0].texCoords = {0.0f, 0.0f};
        va[4 * i + 1].texCoords = {texture_size.x, 0.0f};
        va[4 * i + 2].texCoords = {texture_size.x, texture_size.y};
        va[4 * i + 3].texCoords = {0.0f, texture_size.y};
    }

    uint32_t first = 0;
    while (window.isOpen())
    {
        // Events
        processEvents(window);

        // Update
        for (auto& s : stars) {
            s.position.z -= conf::speed * conf::dt;
            // Move the star to the back
            if (s.position.z < conf::near) {
                s.position.z = conf::far - (conf::near - s.position.z);
                if (first > 0) {
                    first--;
                } else {
                    first = conf::stars_count - 1;
                }
            }
        }

        // Rendering
        window.clear();

        for (uint32_t i{0}; i < conf::stars_count; ++i) {
            uint32_t const idx{(first + i) % conf::stars_count};
            auto& s = stars[idx];
            updateGeometry(i, s, va);
        }

        sf::RenderStates states;
        states.texture = &star_texture;
        states.transform.translate(static_cast<sf::Vector2f>(conf::window_size / 2u));
        window.draw(va, states);

        window.display();
    }

    return 0;
}