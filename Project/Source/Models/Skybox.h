#ifndef SKYBOX_H
#define SKYBOX_H

#include "Shader.h"
#include "Texture.h"

class Skybox {

	SkyboxTexture cubeTexture;
	Shader skyboxShader;

	glm::mat4 transform = glm::mat4(1.0f);

	unsigned int VAO;
public:
	Skybox(const std::string* skyboxTextures);
	~Skybox();

	void SetTransform(const glm::mat4& mvp);
	void SetShader(const Shader& shader);

	void Draw();
};


#endif // !SKYBOX_H
