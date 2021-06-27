#version 330


uniform sampler2D tex;
//uniform vec3 lightPos;
//uniform vec3 lightColor;

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela

//Zmienne interpolowane
//in vec3 Normal;
//in vec3 FragPos;
in vec2 i_texCord;
in vec4 n;
in vec4 l;
in vec4 v;

float toonify(float a, float s){
	return round(s * a) / s;
}

void main(void) {
    // OLD
    /*float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec4 colorObj = texture(tex, i_texCord);

    vec3 result = (ambient + diffuse) * colorObj.rgb;
    pixelColor = vec4(result, 1.0);*/

    // NEW
    vec4 norm_l = normalize(l);
    vec4 norm_n = normalize(n);
    vec4 norm_v = normalize(v);
    vec4 r = reflect(-norm_l, norm_n);

    vec4 kd = texture(tex, i_texCord);
	vec4 ks = vec4(1, 1, 1, 1);

    float nl = clamp(dot(norm_n, norm_l), 0, 1);
	float rv = pow(clamp(dot(r, norm_v), 0, 1), 25);

    //vec4 colorObj = texture(tex, i_texCord);

    pixelColor = vec4(kd.rgb * nl, kd.a) + vec4(ks.rgb * rv, 1);

}