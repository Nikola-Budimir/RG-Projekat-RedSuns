#version 330 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct DirectionLight{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight{
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;

    float cutOff;
    float outerCutOff;
    bool spotSwitch;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform PointLight pointLight;
uniform DirectionLight directionLight;
uniform SpotLight spotLight;
uniform Material material;

uniform vec3 viewPosition;
// calculates the color when using a point light.
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfwayDir, normal), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec4 ambient = vec4(light.ambient, 1.0f) * texture(material.texture_diffuse1, TexCoords);
    vec4 diffuse = vec4(light.diffuse, 1.0f) * diff * texture(material.texture_diffuse1, TexCoords);
    vec4 specular = vec4(light.specular, 1.0f) * spec * texture(material.texture_specular1, TexCoords);


    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}
vec4 CalcDirLight(DirectionLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfwayDir, normal), 0.0), material.shininess);
    //result
    vec4 ambient = vec4(light.ambient , 1.0f) * texture(material.texture_diffuse1, TexCoords);
    vec4 diffuse = vec4(light.diffuse, 1.0f) * diff * texture(material.texture_diffuse1, TexCoords);
    vec4 specular = vec4(light.specular, 1.0f) * spec * texture(material.texture_specular1, TexCoords);
    return (ambient + diffuse + specular);
}
vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    //diff shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfwayDir, normal), 0.0), material.shininess);
    //attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    //cutoff calc
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    //result

    vec4 ambient = vec4(light.ambient, 1.0) * texture(material.texture_diffuse1, TexCoords);
    vec4 diffuse = vec4(light.diffuse, 1.0) * diff * texture(material.texture_diffuse1, TexCoords);
    vec4 specular = vec4(light.specular, 1.0) * spec * texture(material.texture_specular1, TexCoords);
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);

    vec4 result = CalcPointLight(pointLight, normal, FragPos, viewDir);
    result += CalcDirLight(directionLight, normal, viewDir);

     if(spotLight.spotSwitch)
        result += CalcSpotLight(spotLight, normal, FragPos, viewDir);


    FragColor = result;
}