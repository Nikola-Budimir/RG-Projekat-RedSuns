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

uniform PointLight pointLight[2];
uniform DirectionLight directionLight;
uniform SpotLight spotLight;
uniform Material material;

uniform vec3 viewPosition;
// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
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

    vec3 ambientLight = light.ambient * attenuation;
    vec3 diffuseLight = light.diffuse * diff * attenuation;
    vec3 specularLight = light.specular * spec * attenuation;

    vec3 ambient = ambientLight * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = diffuseLight* vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = specularLight* vec3(texture(material.texture_specular1, TexCoords));

    return (ambient + diffuse + specular);
}
vec3 CalcDirLight(DirectionLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfwayDir, normal), 0.0), material.shininess);
    //result
    vec3 ambientLight = light.ambient;
    vec3 diffuseLight = light.diffuse * diff;
    vec3 specularLight = light.specular * spec;

    vec3 ambient = ambientLight * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = diffuseLight* vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = specularLight* vec3(texture(material.texture_specular1, TexCoords));
    return (ambient + diffuse + specular);
}
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
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

    vec3 ambientLight = light.ambient * attenuation * intensity;
        vec3 diffuseLight = light.diffuse * diff * attenuation * intensity;
        vec3 specularLight = light.specular * spec * attenuation * intensity;

        vec3 ambient = ambientLight * vec3(texture(material.texture_diffuse1, TexCoords));
        vec3 diffuse = diffuseLight* vec3(texture(material.texture_diffuse1, TexCoords));
        vec3 specular = specularLight* vec3(texture(material.texture_specular1, TexCoords));
    return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);

    vec3 result = vec3(0.0f);

    result += CalcPointLight(pointLight[0], normal, FragPos, viewDir);
    result += CalcPointLight(pointLight[1], normal, FragPos, viewDir);
    result += CalcDirLight(directionLight, normal, viewDir);

     if(spotLight.spotSwitch)
        result += CalcSpotLight(spotLight, normal, FragPos, viewDir);


    FragColor = vec4(result, texture(material.texture_specular1, TexCoords).a);
}