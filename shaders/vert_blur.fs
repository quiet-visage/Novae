
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

// NOTE: Render size values must be passed from code
const float renderHeight = 100;

float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    vec4 premultiplied= vec4(texColor.rgb*texColor.a, a);

    vec4 texelColor = premultiplied*weight[0];

    for (int i = 1; i < 3; i++)
    {
        texelColor += texture(texture0, fragTexCoord + vec2(0.0, offset[i])/renderHeight)*weight[i];
        texelColor += texture(texture0, fragTexCoord - vec2(0.0, offset[i])/renderHeight)*weight[i];
    }

    vec3 finalrgb = orig.rgb + texelColor.rgb*(1-texColor.a);
    float alpha = texColor.a+texelColor.a*(1-texColor.a);

    finalColor = vec4(finalrgb, alpha);
}
