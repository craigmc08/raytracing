#include <SFML/Graphics.hpp>
#include <string.h>

#define WIDTH 800
#define HEIGHT 800

int main()
{
	if (!sf::Shader::isAvailable()) {
		printf("Shaders are not available. The program will exit.");
		return -1;
	}

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "GPU Raytracer");
	window.setFramerateLimit(60);
	sf::RectangleShape quad;
	quad.setSize(sf::Vector2f(WIDTH, HEIGHT));

	sf::Shader renderShader;
	if (!renderShader.loadFromFile("raytracer.vert", "raytracer.frag"))
	{
		printf("Error loading raytracer shader. The program will exit.");
		return -1;
	}

	float cx = 0;
	float cy = 1;
	float cz = -5;

	float theta = 0;
	float phi = 0;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
		}

		const float speed = 1. / 60.;
		const float aspeed = 1. / 60.;

		sf::Glsl::Vec3 axisForward(sinf(phi), 0, cosf(phi));
		sf::Glsl::Vec3 forward = axisForward * cosf(theta) + sf::Glsl::Vec3(0, 1, 0) * sinf(theta);
		sf::Glsl::Vec3 up = axisForward * -sinf(theta) + sf::Glsl::Vec3(0, 1, 0) * cosf(theta);
		// forward cross up
		sf::Glsl::Vec3 right(
			forward.y*up.z - forward.z*up.y,
			forward.z*up.x - forward.x*up.z,
			forward.x*up.y - forward.y*up.x
		);

		renderShader.setUniform("forward", forward);
		renderShader.setUniform("up", up);
		renderShader.setUniform("right", right);

		float dcx=0, dcy=0, dcz=0;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dcz += speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dcz -= speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dcx -= speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dcx += speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) dcy += speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) dcy -= speed;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) theta += aspeed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) theta -= aspeed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) phi += aspeed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) phi -= aspeed;

		cx += right.x * dcx + forward.x * dcz + up.x * dcy;
		cy += right.y * dcx + forward.y * dcz + up.y * dcy;
		cz += right.z * dcx + forward.z * dcz + up.z * dcy;

		renderShader.setUniform("startPos", sf::Glsl::Vec3(cx, cy, cz));

		window.clear();
		window.draw(quad, &renderShader);
		window.display();
	}

	return 0;
}