#version 330

//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform vec4 lightPos;

//Atrybuty
layout(location = 0) in vec3 vertex; //wspolrzedne wierzcholka w przestrzeni modelu
layout(location = 1) in vec3 normal; //wektor normalny w wierzcholku
layout(location = 2) in vec2 texCoord; //współrzędne teksturowania

//Zmienne interpolowane
out vec2 i_texCord;
//out vec3 FragPos;
//out vec3 Normal;
out vec4 n;
out vec4 l;
out vec4 v;

void main(void) {
    //OLD

    /*FragPos = vec3(M * vec4(vertex, 1.0));
    Normal = mat3(transpose(inverse(M))) * normal;
    i_texCord = texCoord;
    gl_Position = P * V * vec4(FragPos, 1.0);*/

    //NEW

    l = normalize(V * lightPos - V * M * vec4(vertex, 1.0f));
    n = normalize(V * M * vec4(normal, 0.0f));

    v = normalize(vec4(0, 0, 0, 1) - V * M * vec4(vertex, 1.0f));

    i_texCord = texCoord;

    gl_Position = P *V* M * vec4(vertex, 1.0);
}