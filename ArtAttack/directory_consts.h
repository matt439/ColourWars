#ifndef DIRECTORY_CONSTS_H
#define DIRECTORY_CONSTS_H

#include <string>

namespace directory_consts
{
	const static std::string BASE_DIRECTORY = "C:/VSProjects/ChromaClash/ChromaClash/";
	const static std::string TEXTURE_DIRECTORY = "./textures/";
	//const std::string SHADER_DIRECTORY = directory_consts::BASE_DIRECTORY +
	//	"shaders/";
	const static std::string LEVEL_DIRECTORY = directory_consts::BASE_DIRECTORY +
		"levels/";
	//const std::string FONT_DIRECTORY = directory_consts::BASE_DIRECTORY +
	//	"fonts/";
	const static std::string FONT_DIRECTORY = "./fonts/";
	const static std::string SAVE_FILE_PATH = directory_consts::BASE_DIRECTORY +
		"save/save_data.txt";
	const static std::string SOUNDS_DIRECTORY = "./sounds/";
}

#endif // !DIRECTORY_CONSTS_H