/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define CRT_SECURE_NO_WARNINGS 1

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"
#include "OBJ_Loader.h"
#include <filesystem>
#include <windows.h>

objl::Loader loader;
//GLuint cube_texture;
std::vector<GLuint> GlobTextures;
std::vector<std::string> UsedTexes;

bool GAMEOVER = false;


class LoadedMesh {
public:
	std::vector<float> Verticies;
	std::vector<unsigned int> Indices;
	std::vector<float> TexCoords;
	std::vector<float> Normals;
	glm::vec4 Kd;
	std::string Material;
	GLuint Texture;

	LoadedMesh(std::vector<float> v, std::vector<float> t, std::vector<unsigned int> i, std::vector<float> n, std::string m) {
		Verticies = v;
		TexCoords = t;
		Indices = i;
		Material = m;
		Normals = n;

		auto it = std::find(UsedTexes.begin(), UsedTexes.end(), m);
		if (it != UsedTexes.end()) {
			size_t tex_id = it - UsedTexes.begin();
			Texture = GlobTextures[tex_id];
		} else {
			auto tex = this->readTexture(m.data());
			GlobTextures.emplace_back(tex);
			UsedTexes.emplace_back(m.data());
			Texture = tex;
		}
	}

	GLuint readTexture(const char* filename) {
		GLuint tex;
		glActiveTexture(GL_TEXTURE0);

		std::vector<unsigned char> image;  
		unsigned width, height;   
		
		unsigned error = lodepng::decode(image, width, height, filename);
		if (error)
			std::cerr << "ERROR WHILE LOADING TEXTURE. CODE: " << error << "; Image: " << filename << std::endl;
		else
			std::cout << "GOOD TEXTURE => HERE: " << filename << std::endl;
		
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex); 

		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
					 GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return tex;
	}
};

class LoadedModel {
public:
	std::vector<LoadedMesh> Meshes;

	LoadedModel() = default;

	bool loadModel(std::string path, bool inv = true) {
		bool good = loader.LoadFile(path);
		if (!good)
			return false;

		for (auto& mesh : loader.LoadedMeshes) {
			std::vector<float> vertices;
			std::vector<float> texCoords;
			std::vector <float> normals;
			for (auto& vert : mesh.Vertices) {
				vertices.emplace_back(vert.Position.X);
				vertices.emplace_back(vert.Position.Y);
				vertices.emplace_back(vert.Position.Z);

				normals.emplace_back(vert.Normal.X);
				normals.emplace_back(vert.Normal.Y);
				normals.emplace_back(vert.Normal.Z);

				texCoords.emplace_back(vert.TextureCoordinate.X);
				texCoords.emplace_back(inv ? 1.0f - vert.TextureCoordinate.Y : vert.TextureCoordinate.Y);
			}
			std::filesystem::path model_dir = std::filesystem::path(path).parent_path();
			std::string texName = (model_dir / mesh.MeshMaterial.map_Kd).u8string();
			auto *n_mesh = new LoadedMesh(vertices, texCoords, mesh.Indices, normals, texName);
			n_mesh->Kd = glm::vec4(mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z, 1);
			Meshes.emplace_back(*n_mesh);
		}

		return true;
	}

	std::string extractDir(std::string path) {
		std::string directory;
		const size_t last_slash_idx = path.rfind('\\');
		if (std::string::npos != last_slash_idx)
			directory = path.substr(0, last_slash_idx);

		return directory;
	}

	void drawModel(ShaderProgram* sp) {
		for (auto& mesh : Meshes) {
			glEnableVertexAttribArray(sp->a("vertex"));
			glVertexAttribPointer(sp->a("vertex"), 3, GL_FLOAT, false, 0, mesh.Verticies.data());
			glEnableVertexAttribArray(sp->a("texCoord"));
			glVertexAttribPointer(sp->a("texCoord"), 2, GL_FLOAT, false, 0, mesh.TexCoords.data());
			glEnableVertexAttribArray(sp->a("normal"));
			glVertexAttribPointer(sp->a("normal"), 3, GL_FLOAT, false, 0, mesh.Normals.data());

			glUniform1i(sp->u("tex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mesh.Texture);

			glDrawElements(GL_TRIANGLES, mesh.Indices.size(), GL_UNSIGNED_INT, mesh.Indices.data());

			glDisableVertexAttribArray(sp->a("vertex"));
			glDisableVertexAttribArray(sp->a("normal"));
			glDisableVertexAttribArray(sp->a("texCoord"));
		}
	}
};


glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 7.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 15.0f, 0.0f);

//float lastX = 1280 / 2, lastY = 720 / 2;
//bool firstMouse = true;
//float yaw = -90.0f;
//float pitch = 0.0f;

float deltaTime = 0.0f;    
float lastFrame = 0.0f;

float fov = 60.0f;

float speed_x = 0;
float speed_z = 0;


LoadedModel* bolid = new LoadedModel();
LoadedModel* bridge = new LoadedModel();

// KLAWISZE OBSLUGA
//void movementProcess(GLFWwindow* window) {
//	cameraSpeed = 2.0f * deltaTime;
//	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
//		if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
//			cameraPos += cameraSpeed * cameraFront * cameraSpeedMultiply;
//		else
//			cameraPos += cameraSpeed * cameraFront;
//	}
//	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
//		if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
//			cameraPos -= cameraSpeed * cameraFront * cameraSpeedMultiply;
//		else
//			cameraPos -= cameraSpeed * cameraFront;
//	}
//	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
//		if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
//			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * cameraSpeedMultiply;
//		else
//			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
//	}
//	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
//		if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
//			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * cameraSpeedMultiply;
//		else
//			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
//	}
//}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 90.0f)
		fov = 90.0f;
}

//void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
//	if (firstMouse) {
//		lastX = xpos;
//		lastY = ypos;
//		firstMouse = false;
//	}
//
//	float xoffset = xpos - lastX;
//	float yoffset = lastY - ypos;
//	lastX = xpos;
//	lastY = ypos;
//
//	float sensitivity = 0.1f;
//	xoffset *= sensitivity;
//	yoffset *= sensitivity;
//
//	yaw += xoffset;
//	pitch += yoffset;
//
//	if (pitch > 89.0f)
//		pitch = 89.0f;
//	if (pitch < -89.0f)
//		pitch = -89.0f;
//
//	glm::vec3 direction;
//	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
//	direction.y = sin(glm::radians(pitch));
//	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
//	cameraFront = glm::normalize(direction);
//}

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void key_callback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mod
) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT) {
			speed_z = -PI;
		}
		if (key == GLFW_KEY_RIGHT) {
			speed_z = PI;
		}
		if (key == GLFW_KEY_UP) {
			speed_x = -PI;
		}
		if (key == GLFW_KEY_DOWN) {
			speed_x = PI;
		}
	}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
			speed_z = 0;
		}
		if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
			speed_x = 0;
		}
	}
}

GLuint readTexture(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);

	std::vector<unsigned char> image; 
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex); 
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}


//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSetCursorPosCallback(window, mouse_callback); // MOUSE CALLBACK
	bool good  = bolid->loadModel("models/bolid/untitled.obj", true);
	bool good2 = bridge->loadModel("models/bridge/untitled.obj", true);

	if (!(good && good2))
		throw std::exception("Couldn't load model");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
	//************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************
	for (auto &tex : GlobTextures)
		glDeleteTextures(1, &tex);
}

//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window, float angle_x, float angle_z) {
	//************Tutaj umieszczaj kod rysujący obraz******************l
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::vec3 front = glm::vec3(0.2f, 1.0f, 0.1f);
	glm::mat4 P = glm::perspective(glm::radians(fov), 16.0f / 9.0f, 0.03f, 5000.0f);
	glm::mat4 V = glm::lookAt(glm::vec3(-6.0f - angle_x, 4.2f, 1.0f), glm::vec3(-angle_x, 0.0f, 0.0f) + front, glm::vec3(0.0f, 15.0f, 0.0f));
	spShifty->use();

	// BOLID
	glm::mat4 M = glm::mat4(1.0f);;
	M = glm::translate(M, glm::vec3(-angle_x, 0.0f, angle_z/2));

	glm::vec4 &carpos = M[3];
	glm::vec4 lightPos = glm::vec4(carpos.x, carpos.y + 50.0f, 0.0f, 1);
	glUniform4fv(spShifty->u("lightPos"), 1, glm::value_ptr(lightPos));

	auto distToSide = glm::length(carpos - glm::vec4(carpos.x, 0, 0, 1));
	auto distToEnd = glm::length(carpos - glm::vec4(.0f, .0f, .0f, 1.0f));

	if (distToSide > 2.7 || distToEnd > 226.0) {
		GAMEOVER = true;
		std::cout << "GAME OVER!" << std::endl;
		std::cout << " Dist to side: " << distToSide << ". Dist to end: " << distToEnd << std::endl;
		return;
	}
	std::cout << "Dist: " << " " << distToSide << " " << distToEnd << std::endl;

	glUniformMatrix4fv(spShifty->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spShifty->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spShifty->u("M"), 1, false, glm::value_ptr(M));
	
	bolid->drawModel(spShifty);
	
	// BRIDGE
	glm::mat4 M1 = glm::mat4(1.0f);
	glUniformMatrix4fv(spShifty->u("P"), 1, false, glm::value_ptr(P)); 
	glUniformMatrix4fv(spShifty->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spShifty->u("M"), 1, false, glm::value_ptr(M1));
	bridge->drawModel(spShifty);

	glfwSwapBuffers(window);
}

#pragma comment(lib, "Winmm.lib")
int main(void) {
	std::ios_base::sync_with_stdio(false);

	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1280, 720, "OpenGL", NULL, NULL);

	if (!window) {
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	//Główna pętla
	float angle_x = 0;
	float angle_z = 0;
	float pos = 0;

	glfwSetTime(0);
	sndPlaySound(L"models/acc.wav", SND_ASYNC | SND_LOOP);
	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		angle_x += (speed_x + pos)  * deltaTime;
		angle_z += speed_z * deltaTime;
		pos -= 0.001;

		//movementProcess(window);
		if (!GAMEOVER) {
			drawScene(window, angle_x, angle_z);
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			sndPlaySound(nullptr, SND_ASYNC | SND_LOOP);
			int val = MessageBox(nullptr, L"GAME OVER!", L"GAME OVER!", MB_OK);
			if (val == IDOK)
				break;
		}
		glfwPollEvents();
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
