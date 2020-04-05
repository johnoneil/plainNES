#pragma once
#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
	// program ID
	unsigned int ID;

	// Constructor reads and builds the shader
	Shader();
	void init();

	// Use/activate the shader
	void use();

	//Set uniform functions
	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;
	void setMat4(const std::string &name, glm::mat4 value) const;

	float getFloat(const std::string &name) const;
};

