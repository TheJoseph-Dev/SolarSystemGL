
#include <vector>
//#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>

#define WINDOW_WIDTH 1920.0f//960.0f
#define WINDOW_HEIGHT 1080.0f//540.0f

#include "Utils.h"
#include "Models/Shader.h"
#include "Models/Texture.h"
#include "Models/Camera.h"
#include "Models/OBJLoader.h"
#include "Models/Framebuffer.h"
#include "Models/Skybox.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

/*! @brief Loads, compiles and returns the vertex shader
* 
* @param[in] Filepath to the shader
* @ingroup window
*/
//

double m_xpos, m_ypos;
glm::vec2 mouseOffset = glm::vec2(0);
void mouseInputHandler(GLFWwindow* window) {
	double tempXpos = m_xpos, tempYPos = m_ypos;
	glfwGetCursorPos(window, &m_xpos, &m_ypos);
	mouseOffset = -glm::vec2(tempXpos-m_xpos, tempYPos-m_ypos)/200.0f;
}

glm::vec3 keyInputTransform = glm::vec3(0);
glm::vec3 keyDir = glm::vec3(0);
float velocity = 0.05f;
void keyInputHandler(GLFWwindow* window, float deltaTime) {
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { keyInputTransform += glm::vec3(0, 0, 1); }
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { keyInputTransform += glm::vec3(1, 0, 0); }
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { keyInputTransform += glm::vec3(0, 0, -1); }
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { keyInputTransform += glm::vec3(-1, 0, 0); }
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) { keyInputTransform += glm::vec3(0, -1, 0); }
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { keyInputTransform += glm::vec3(0, 1, 0); }
	keyDir = keyInputTransform;
	keyInputTransform *= velocity;
	keyInputTransform /= 1.2f;
}

// Configs
const unsigned int sunScale = 109;
float sunFixedScale = 0.1f;
float planetsSpeed = 1.0f;
float planetsDistance = 1.0f;
float planetsApparentScale = 1.0f;
bool bloom = true;
int bloomMipLevel = 0;
bool tonemap = false;

float currentWidth = WINDOW_WIDTH;
float currentHeight = WINDOW_HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	currentWidth = width; currentHeight = height;
	glViewport(0, 0, width, height);
	//fb.UpdateBounds(currentWidth, currentHeight);
}

void framebuffer_resize(FrameBuffer& fb) {
	if (fb.GetWidth() == currentWidth && fb.GetHeight() == currentHeight) return;
	//fb = FrameBuffer(currentWidth, currentHeight);
	fb.UpdateBounds(currentWidth, currentHeight);
	fb.Unbind();
}

void bloom_framebuffer_resize(BloomFB& fb) {
	if (fb.GetWidth() == currentWidth && fb.GetHeight() == currentHeight) return;
	//fb = FrameBuffer(currentWidth, currentHeight);
	fb.UpdateBounds(currentWidth, currentHeight);
	fb.Unbind();
}


struct MoonData {
	glm::vec3 scale;
	float planetDistance;
	float tVelocity;
	float rVelocity;

	unsigned int parentPlanet;

	unsigned int textureLayer;

	MoonData(unsigned int parentPlanet, glm::vec3 scale, float planetDistance, float tVelocity, float rVelocity, unsigned int textureLayer)
		: parentPlanet(parentPlanet), scale(scale), planetDistance(planetDistance), tVelocity(tVelocity), rVelocity(rVelocity), textureLayer(textureLayer) {}
};

enum class RingType {
	NONE = -1, SATURN = 0, URANUS = 1
};

struct PlanetData {
	glm::vec3 scale;
	float sunDistance;
	float tVelocity;
	float rVelocity;

	unsigned int moonsCount;
	MoonData* moons;

	unsigned int textureLayer;

	RingType ring;

	PlanetData(const glm::vec3& scale, float sunDistance, float tVelocity, float rVelocity, unsigned int textureLayer, MoonData* moons = nullptr, unsigned int moonsCount = 0, RingType ring = RingType::NONE)
		: scale(scale), sunDistance(sunDistance), tVelocity(tVelocity), rVelocity(rVelocity), textureLayer(textureLayer), moons(moons), moonsCount(moonsCount), ring(ring) {}
};

void GetPlanetModel(glm::mat4& model, PlanetData pData, float angle, bool isSun) {
	model = glm::mat4(1.0f);
	if (!isSun) {
		model = glm::rotate(model, glm::radians(angle) * pData.tVelocity * 100.0f * planetsSpeed, glm::vec3(0, 1, 0)); // Orbit
		float scale = (-pData.sunDistance / 2.0f * planetsDistance * 0.1f) * (sunScale * sunFixedScale) - (sunScale * sunFixedScale);
		model = glm::translate(model, glm::vec3(0, 0, scale));
	}

	model = glm::rotate(model, glm::radians(angle) * pData.rVelocity / 10.0f * planetsSpeed, glm::vec3(0, 1, 0));
	model = glm::scale(model, pData.scale * (isSun ? sunFixedScale : planetsApparentScale));
}

float GetOrbitOutlineModel(glm::mat4& model, PlanetData pData) {
	model = glm::mat4(1.0f);
	float scale = (-pData.sunDistance / 2.0f * planetsDistance * 0.1f) * (sunScale * sunFixedScale) - (sunScale * sunFixedScale);
	scale *= 2.02f;
	model = glm::translate(model, glm::vec3(-scale / 2.0f, -0.1, -scale / 2.0f));
	model = glm::scale(model, glm::vec3(scale));

	return scale;
}

void GetRingModel(glm::mat4& model, PlanetData pData, float angle) {
	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle) * pData.tVelocity * 100.0f * planetsSpeed, glm::vec3(0, 1, 0)); // Orbit
	model = glm::translate(model, glm::vec3(0, 0, (-pData.sunDistance / 2.0f * planetsDistance * 0.1f) * (sunScale * sunFixedScale) - (sunScale * sunFixedScale)));
	model = glm::translate(model, -glm::vec3(pData.scale.x, 0, pData.scale.z) * planetsApparentScale * 2.0f); // Centralizate
	model = glm::scale(model, pData.scale * planetsApparentScale * 4.0f);
}


int main()  {
	std::cout << "Hello World!" << std::endl;
	
	// SETUP ---
	// Init GLFW
	if (!glfwInit()) { print("ERROR: Could not Init GLFW");  return -1; }

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SolarSystemGL", NULL, NULL);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Init GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { print("ERROR: Could not Init GLAD"); return -1; }


	// Tell OpenGL the size of the rendering window so OpenGL knows how we want to display the data and coordinates with respect to the window.
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	// Window Resize Callback
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// 3D:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST); // Depth Testing
	glDisable(GL_CULL_FACE);

	// * End Init ---

	Camera camera = Camera(glm::vec3(0.0f, 0.5f, -1.0f));
	float cameraSpeed = 1.0f;
	
	// QUAD
	float quadVertices[] = { 
		0.0, 0.0, 0.0,	 0.0, 0.0,
		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 0.0, 1.0,	 0.0, 1.0,

		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 0.0, 1.0,	 0.0, 1.0,
		1.0, 0.0, 1.0,   1.0, 1.0
	};

	// CUBE
	float cubeVertices[] = {
		// Front
		0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
		1.0f,  1.0f, 1.0f,  1.0f, 1.0f,
		0.0f,  0.0f, 1.0f,  0.0f, 0.0f,

		1.0f,  1.0f, 1.0f,  1.0f, 1.0f,
		0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
		1.0f,  0.0f, 1.0f,  1.0f, 0.0f,

		// Left
		0.0f,  1.0f, 0.0f,   0.0f, 1.0f,
		0.0f,  1.0f, 1.0f,   1.0f, 1.0f,
		0.0f,  0.0f, 0.0f,   0.0f, 0.0f,

		0.0f,  1.0f, 1.0f,   1.0f, 1.0f,
		0.0f,  0.0f, 0.0f,   0.0f, 0.0f,
		0.0f,  0.0f, 1.0f,   1.0f, 0.0f,

		// Back
		0.0f,  1.0f, 0.0f,   0.0f, 1.0f,
		1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		0.0f,  0.0f, 0.0f,   0.0f, 0.0f,

		1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		0.0f,  0.0f, 0.0f,   0.0f, 0.0f,
		1.0f,  0.0f, 0.0f,   1.0f, 0.0f,

		// Right
		1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
		1.0f,  1.0f, 1.0f,  1.0f, 1.0f,
		1.0f,  0.0f, 0.0f,  0.0f, 0.0f,

		1.0f,  1.0f, 1.0f,  1.0f, 1.0f,
		1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
		1.0f,  0.0f, 1.0f,  1.0f, 0.0f,

		// Top
		0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
		1.0f,  1.0f, 1.0f,  1.0f, 1.0f,
		0.0f,  1.0f, 0.0f,  0.0f, 0.0f,

		1.0f,  1.0f, 1.0f,  1.0f, 1.0f,
		0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
		1.0f,  1.0f, 0.0f,  1.0f, 0.0f,

		// Bottom
		0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
		1.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		0.0f,  0.0f, 0.0f,   0.0f, 0.0f,

		1.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		0.0f,  0.0f, 0.0f,   0.0f, 0.0f,
		1.0f,  0.0f, 0.0f,   1.0f, 0.0f,
	};
	

	OBJLoader obj(Resources("OBJs/sphereLowRes.obj"));
	float* vertices = obj.GetVertices();
	int verticesSize = obj.GetVerticesSize();
	print("Vetices: " << obj.GetVerticesSize()/Vertex::GetStride() << "\t" << "Floats: " << obj.GetVerticesSize());

	//for (int i = 0; i < verticesSize; i++) { print(*(vertices+i)); }

	unsigned int VBO;
	glCreateBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesSize*sizeof(float), vertices, GL_STATIC_DRAW);
	

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Vertex::GetStride() * sizeof(float), 0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Vertex::GetStride() * sizeof(float), (void*)(sizeof(float) * 3));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Vertex::GetStride() * sizeof(float), (void*)(sizeof(float) * 5));

	unsigned int quadVBO;
	glCreateBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	unsigned int quadVAO;
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));

	unsigned int cubeVBO;
	glCreateBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	unsigned int cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));

	
	std::string texturePaths[] = {
		Resources("Textures/sun.png"), 
		Resources("Textures/mercury.png"),
		Resources("Textures/venus.png"),
		Resources("Textures/earth.png"),
		Resources("Textures/moon.png"),
		Resources("Textures/mars.png"),
		Resources("Textures/jupter.png"),
		Resources("Textures/saturn.png"),
		Resources("Textures/uranus.png"),
		Resources("Textures/neptune.png"),
	};

	std::string ringsTexturePaths[] = {
		Resources("Textures/saturnring.png"),
		Resources("Textures/uranusring.png")
	};

	const int astroObjs = count(texturePaths, sizeof(std::string)) / 5;
	const int moons = 1;

	MoonData earthMoon = MoonData(3, glm::vec3(0.2f), 10, 0, 0, 4);

	const unsigned int AU = 150;

	PlanetData planetsData[astroObjs - moons]{
		PlanetData(glm::vec3(sunScale), 0, 1, 1, 0), // Sun
		PlanetData(glm::vec3(0.38f), 0.4f*AU, 1.0f/88.0f, 1, 1), // Mercury
		PlanetData(glm::vec3(0.95f), 0.72f*AU, 1.0f/225.0f, 1, 2), // Venus
		PlanetData(glm::vec3(1), AU, 1.0f/365.0f, 1, 3, &earthMoon, 1), // Earth d = 12756km
		PlanetData(glm::vec3(0.53f), 1.52f*AU, 1.0f/687.0f, 1, 5), // Mars
		PlanetData(glm::vec3(11.2f), 5.2f*AU, 1.0f/4331.0f, 1, 6), // Jupter
		PlanetData(glm::vec3(9.44f), 9.5f*AU, 1.0f/10950.0f, 1, 7, nullptr, 0, RingType::SATURN), // Saturn
		PlanetData(glm::vec3(4.0f), 19.8f*AU, 1.0f/30799.0f, 1, 8, nullptr, 0, RingType::URANUS), // Uranus
		PlanetData(glm::vec3(3.88f), 30.0f*AU, 1.0f/60190.0f, 1, 9), // Neptune
	};

	FrameBuffer fb = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT);

	//fb = FrameBuffer(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

	fb.Check();
	fb.Unbind();

	const unsigned short bloomMipChainLegth = 8;
	BloomFB bloomFB = BloomFB(WINDOW_WIDTH, WINDOW_HEIGHT, bloomMipChainLegth);
	bloomFB.Check();
	bloomFB.Unbind();


	FrameBuffer finalBloomFB = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
	finalBloomFB.Check();
	finalBloomFB.Unbind();



	std::string skyboxTextures[] = {
		Resources("Textures/Skybox/left.png"),
		Resources("Textures/Skybox/right.png"),
		Resources("Textures/Skybox/top.png"),
		Resources("Textures/Skybox/bottom.png"),
		Resources("Textures/Skybox/front.png"),
		Resources("Textures/Skybox/back.png")
	};
	Skybox spaceBox = Skybox(skyboxTextures);

	Texture astroTextures = Texture(texturePaths, astroObjs);
	astroTextures.BindArray();
	astroTextures.LoadArrray(1024, 512);

	Texture ringTextures = Texture(ringsTexturePaths, 2);
	ringTextures.BindArray(1);
	ringTextures.LoadArrray();


	// Shader
	Shader orbitShader = Shader(Resources("Shaders/orbitTrace.glsl"));
	orbitShader.Bind();

	Shader quadShader = Shader(Resources("Shaders/defaultQuad.glsl"));
	quadShader.Bind();

	Shader shader = Shader(Resources("Shaders/shader3D.glsl"));
	shader.Bind();

	glm::mat4 mvp = glm::mat4(1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 2500.f); //glm::ortho(0.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f);
	glm::mat4 model = glm::mat4(1.0f);
	
	glm::mat4 planeProjection = glm::ortho(0.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f);
	//fb.SetTransform(planeProjection);

	//glm::vec3 camPos = camera.GetPosition();
	//shader.SetUniform3f("camPos", camPos.x, camPos.y, camPos.z);
	//shader.SetUniform2f("iMouse", 0, 0);
	shader.SetUniformUInt("mainTex", 0);
	shader.SetUniformInt("texLayer", 0);
	
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	ImGui::StyleColorsDark();

	float time = 0;
	float angle = 0.0f;
	// LOOP
	while (!glfwWindowShouldClose(window)) {
		framebuffer_resize(fb);
		bloom_framebuffer_resize(bloomFB);
		framebuffer_resize(finalBloomFB);
		//framebuffer_resize(bloomFB);
		framebuffer_resize(finalBloomFB);
		planeProjection = glm::ortho(0.0f, currentWidth, currentHeight, 0.0f);
		projection = glm::perspective(glm::radians(45.0f), currentWidth / currentHeight, 0.1f, 2500.f);

		fb.SetTransform(planeProjection);
		bloomFB.SetTransform(planeProjection);
		finalBloomFB.SetTransform(planeProjection);
		fb.Bind();
		glEnable(GL_DEPTH_TEST);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.01, 0.02, 0.05, 1.0);
		//glClearColor(1.0, 1.0, 1.0, 1.0);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("SolarSystem Controller");

		ImGui::Text("Naviagtion");
		ImGui::DragFloat("Speed", &cameraSpeed, 1.0f, 1.0f, 200.0f);
		ImGui::Text("Sun Config");
		ImGui::DragFloat("Sun Scale", &sunFixedScale, 0.1f, 0.1f, 1.0f);
		ImGui::Text("Planets Config");
		ImGui::DragFloat("Planet Speed", &planetsSpeed, 0.1f, 0.0f, 2.0f);
		ImGui::DragFloat("Planet Distance", &planetsDistance, 0.1f, 0.1f, 1.0f);
		ImGui::DragFloat("Planet Apparent Scale", &planetsApparentScale, 0.1f, 1.0f, 3.0f);
		ImGui::Text("Screen Effects");
		ImGui::Checkbox("Bloom", &bloom);
		ImGui::DragInt("BloomMipLevel", &bloomMipLevel, 1.0f, 0, bloomMipChainLegth - 1);
		ImGui::Checkbox("Tonemap (ACES Fitted)", &tonemap);

		ImGui::End();

		time = glfwGetTime();

		keyInputHandler(window, 0);
		mouseInputHandler(window);

		camera.Rotate(glm::vec3(mouseOffset.y, mouseOffset.x, 0));
		camera.Move(keyInputTransform*cameraSpeed);
		angle += 0.5f;

		
		for (int i = 0; i < astroObjs-moons; i++) {
			shader.Bind();
			astroTextures.BindArray();
			glBindVertexArray(VAO);
			
			PlanetData pData = planetsData[i];

			mvp = glm::mat4(1.0f);
			model = glm::mat4(1.0f);
			GetPlanetModel(model, pData, angle, i == 0);

			mvp *= projection;
			mvp *= camera.GetView();
			mvp *= model;
			shader.SetUniformMat4("MVP", mvp);
			shader.SetUniformMat4("model", model);
			shader.SetUniformInt("texLayer", pData.textureLayer);		
			shader.SetUniformInt("isEmissive", i == 0);

			shader.SetUniform2f("iResolution", currentWidth, currentHeight);
			shader.SetUniformFloat("iTime", time);

			glDrawArrays(GL_TRIANGLES, 0, verticesSize / Vertex::GetStride());

			// Render Orbit Path
			if (i == 0) { continue; }
			orbitShader.Bind();
			glBindVertexArray(quadVAO);
			mvp = glm::mat4(1.0f);
			model = glm::mat4(1.0f);
			float radius = GetOrbitOutlineModel(model, pData);
			mvp *= projection;
			mvp *= camera.GetView();
			mvp *= model;
			orbitShader.SetUniformMat4("MVP", mvp);
			orbitShader.SetUniform2f("iResolution", currentWidth, currentHeight);
			orbitShader.SetUniformFloat("iTime", time);
			orbitShader.SetUniformFloat("scale", radius);
			glDrawArrays(GL_TRIANGLES, 0, count(quadVertices, float) / 5);

			// Render Ring
			if (pData.ring == RingType::NONE) { continue; }

			quadShader.Bind();
			ringTextures.BindArray();
			mvp = glm::mat4(1.0f);
			model = glm::mat4(1.0f);
			GetRingModel(model, pData, angle);
			mvp *= projection;
			mvp *= camera.GetView();
			mvp *= model;
			quadShader.SetUniformMat4("MVP", mvp);
			quadShader.SetUniformUInt("mainTex", 1);
			quadShader.SetUniformInt("texLayer", (int)pData.ring);
			quadShader.SetUniform2f("iResolution", currentWidth, currentHeight);
			quadShader.SetUniformFloat("iTime", time);

			glDrawArrays(GL_TRIANGLES, 0, count(quadVertices, float) / 5);
		}

		//mvp *= projection;
		//mvp *= glm::mat4(glm::mat3(camera.GetView())); // Disables translation
		model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
		mvp *= model;
		spaceBox.SetTransform(mvp);

		// TODO: - FIX FRAMEBUFFER ASSIGNMENT => RESIZE
		//		 - FIX SKYBOX FAR PLANE => Actually it's alright, it's just my far plane (depth buffer) which is weird
		//		 - IMPLEMENT BLOOM


		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		spaceBox.Draw();
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

		fb.Unbind();
		fb.Draw();

		if (bloom) {
			bloomFB.Draw(fb.GetFBTexture(), 0.005f);
			finalBloomFB.Draw(tonemap, bloomFB.GetBloomMipAt(bloomMipLevel));
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		//glFlush();
		glfwPollEvents();
	}

	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();

	return 0;
}