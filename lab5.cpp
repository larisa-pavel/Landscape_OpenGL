#include "lab_m2/lab5/lab5.h"
#include "lab_m2/lab4/lab4.h"

#include <glm/gtc/noise.hpp> // For Perlin noise


#include <vector>
#include <iostream>
#include "core/engine.h"
#include "utils/gl_utils.h"
#include "stb/stb_image.h"
#include <limits>

using namespace std;
using namespace m2;


//Generates a random value between 0 and 1.
inline float Rand01()
{
    return rand() / static_cast<float>(RAND_MAX);
}

/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Lab5::Lab5()
{
    framebuffer_object = 0;
    color_texture = 0;
    depth_texture = 0;

    angle = 0;

    type = 0;
}


Lab5::~Lab5()
{
}


void Lab5::Init()
{
    outputType = 0;

    auto camera = GetSceneCamera();
    camera->SetPositionAndRotation(glm::vec3(0, 8, 25.5), glm::quat(glm::vec3(-20 * TO_RADIANS, 0, 0)));
    camera->Update();

    //The start of lab 6


    std::string texturePath = PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "cubemap_night");
    std::string shaderPath = PATH_JOIN(window->props.selfDir, SOURCE_PATH::M2, "lab5", "shaders");

    {
        Mesh* mesh = new Mesh("bunny");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "plane50.obj");
        mesh->UseMaterials(false);
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("cubemap_night");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
        mesh->UseMaterials(false);
        meshes[mesh->GetMeshID()] = mesh;
    }

    // Create a shader program for rendering cubemap texture
    {
        Shader* shader = new Shader("CubeMap");
        shader->AddShader(PATH_JOIN(shaderPath, "CubeMap.VS.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, "CubeMap.FS.glsl"), GL_FRAGMENT_SHADER);
        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }

    // Create a shader program for standard rendering
    {
        Shader* shader = new Shader("ShaderNormal");
        shader->AddShader(PATH_JOIN(shaderPath, "Normal.VS.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, "Normal.FS.glsl"), GL_FRAGMENT_SHADER);
        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }

    // Create a shader program for creating a CUBEMAP
    {
        Shader* shader = new Shader("Framebuffer");
        shader->AddShader(PATH_JOIN(shaderPath, "Framebuffer.VS.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, "Framebuffer.FS.glsl"), GL_FRAGMENT_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, "Framebuffer.GS.glsl"), GL_GEOMETRY_SHADER);
        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }

    cubeMapTextureID = UploadCubeMapTexture(
        PATH_JOIN(texturePath, "pos_x.png"),
        PATH_JOIN(texturePath, "pos_y.png"),
        PATH_JOIN(texturePath, "pos_z.png"),
        PATH_JOIN(texturePath, "neg_x.png"),
        PATH_JOIN(texturePath, "neg_y.png"),
        PATH_JOIN(texturePath, "neg_z.png"));


    // Create the framebuffer on which the scene is rendered from the perspective of the mesh
    // Texture size must be cubic
    CreateFramebuffer(1024, 1024);

    //The end of lab 6

    TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES), "ground.jpg");

    // Load a mesh from file into GPU memory
    

    {
        Mesh* mesh = new Mesh("plane");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "plane50.obj");
        mesh->UseMaterials(false);
        meshes[mesh->GetMeshID()] = mesh;
    }

    // Load a mesh from file into GPU memory
    {
        Mesh* mesh = new Mesh("sphere");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
        mesh->UseMaterials(false);
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("quad");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "quad.obj");
        mesh->UseMaterials(false);
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES), "rain.png");
    }

    LoadShader("Render2Texture");
    LoadShader("Composition");
    LoadShader("LightPass");
    auto resolution = window->GetResolution();

    frameBuffer = new FrameBuffer();
    frameBuffer->Generate(resolution.x, resolution.y, 3);
    //frameBuffer contains 3 textures (position, normal and color)

    lightBuffer = new FrameBuffer();
    lightBuffer->Generate(resolution.x, resolution.y, 1, false);
    //lightBuffer contains 1 texture (light accumulation)

    for (int i = 0; i < 10; ++i)
    {
        LightInfo lightInfo;

        // TODO(student): Set lightInfo with random position, random color
        // and a random radius for each light source.
        // You can use the Rand01 function defined above.
        // The chosen position is between (-10, 0, -10) and (10, 3, 10)
        // The chosen color is between (0, 0, 0) and (1, 1, 1).
        // The chosen radius is between 3 and 4.
        rand_info1 = Rand01();
        rand_info2 = Rand01();
        rand_info3 = Rand01();

        if (rand_info1 < Rand01()) {
            sign = 1;
        }
        else {
            sign = -1;
        }

        lightInfo.position = glm::vec3(rand_info1 * 12 * (sign) + (sign)*3,5 + rand_info2 * 3, rand_info3 * 12 * sign + (sign) * 3);
        lightInfo.color = glm::vec3(rand_info1, rand_info2, rand_info3);
        lightInfo.radius = 3 + Rand01();
        lights.push_back(lightInfo);
    }

    // Generate a grid of vertices for terrain
    std::vector<VertexFormat> terrainVertices;
    std::vector<unsigned int> terrainIndices;

    int m = 40, n = 40; // Grid resolution
    float h_max = 8.0f;   // Maximum height
    float r = 8.0f;      // Radius of influence

    glm::vec3 center(0.0f, 0.0f, 0.0f);

    const glm::vec3 bezier_p0 = glm::vec3(0.0f, 0.2f, 0.0f);
    const glm::vec3 bezier_p1 = glm::vec3(8.0f, 0.7f, 0.0f);
    const glm::vec3 bezier_p2 = glm::vec3(13.0f, 3.2f, 0.0f);
    const glm::vec3 bezier_p3 = glm::vec3(15.0f, 6.0f, 0.0f);


    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            float x = (float)i - m / 2.0f;
            float z = (float)j - n / 2.0f;

            glm::vec3 position(x, 0.0f, z);
            float d = glm::distance(glm::vec2(position.x, position.z), glm::vec2(center.x, center.z)) / r;

            float h = 0.0f;
            if (d < 1.0f) {
                h = (d * d / 2.0f) * h_max;
            }
            else {
                h = (1.0f - ((2.0f - d) * (2.0f - d)) / 2.0f) * h_max;
            }
            float noiseValue = glm::perlin(glm::vec2(x, z) * 0.2f)*3.0f; // Scale noise frequency

            // Blend the smooth noise into the terrain
            float distance = sqrt(x * x + z * z);
            float blend = glm::clamp((distance) / 5.0f, 0.0f, 1.0f);
            h += noiseValue * blend;
            float d_bezier = 0.0f;
            float min_distance = 1e10;
            float closest_t = 0.0f;
            for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
                glm::vec3 bezier_point = glm::vec3(bezier_p0) * glm::vec3(pow((1 - t), 3)) +
                    glm::vec3(bezier_p1) * glm::vec3(3 * t * pow((1 - t), 2)) +
                    glm::vec3(bezier_p2) * glm::vec3(3 * pow(t, 2) * (1 - t)) +
                    glm::vec3(bezier_p3) * glm::vec3(pow(t, 3));
                float dis = glm::sqrt((position.x - bezier_point.x) * (position.x - bezier_point.x) + (position.z - bezier_point.z) * (position.z - bezier_point.z));
                if (dis < min_distance) {
                    min_distance = dis;
                    closest_t = t;
                }
            }
            d_bezier = min_distance;
            if (d_bezier >= 3.0f) {
                position.y = h;
            }
            else {
                glm::vec3 b_closest = glm::vec3(bezier_p0) * glm::vec3(pow((1 - closest_t), 3)) +
                    glm::vec3(bezier_p1) * glm::vec3(3 * closest_t * pow((1 - closest_t), 2)) +
                    glm::vec3(bezier_p2) * glm::vec3(3 * pow(closest_t, 2) * (1 - closest_t)) +
                    glm::vec3(bezier_p3) * glm::vec3(pow(closest_t, 3));
                float norm = glm::clamp(d_bezier / r, 0.0f, 1.0f);
                position.y = glm::mix(b_closest.y, h, 1 - std::sin(M_PI * 0.5 - norm * M_PI * 0.5));
            }
            
            float offset = 0.05f;

            // Compute the heights at the surrounding positions
            float heightRight = getHeight(position.x + offset, position.z, r, center, h); // Sample terrain height at (x + offset, z)
            float heightLeft = getHeight(position.x - offset, position.z, r, center, h);  // Sample terrain height at (x - offset, z)
            float heightFront = getHeight(position.x, position.z + offset, r, center, h); // Sample terrain height at (x, z + offset)
            float heightBack = getHeight(position.x, position.z - offset, r, center, h);  // Sample terrain height at (x, z - offset)

            // Define points with correct heights
            glm::vec3 right = glm::vec3(position.x + offset, heightRight, position.z);
            glm::vec3 left = glm::vec3(position.x - offset, heightLeft, position.z);
            glm::vec3 front = glm::vec3(position.x, heightFront, position.z + offset);
            glm::vec3 back = glm::vec3(position.x, heightBack, position.z - offset);

            // Compute the normal using the cross product
            glm::vec3 normal = glm::normalize(glm::cross(front - back, right - left));
            float u = (float)j / ((float)n);
            float v = (float)i / ((float)m);
            glm::vec2 uv(u, v);
            
            terrainVertices.emplace_back(position, glm::vec3(0.0f,0.1f,0.6f), normal, uv); // Add normal data
        }
    }

    // Generate indices for the grid
    for (int i = 0; i < m - 1; ++i) {
        for (int j = 0; j < n - 1; ++j) {
            int idx = i * n + j;
            terrainIndices.push_back(idx);
            terrainIndices.push_back(idx + 1);
            terrainIndices.push_back(idx + n);

            terrainIndices.push_back(idx + 1);
            terrainIndices.push_back(idx + n + 1);
            terrainIndices.push_back(idx + n);
        }
    }

    CreateMesh("terrain", terrainVertices, terrainIndices);
}

float Lab5::getHeight(float x, float z, float r_cascada, glm::vec3 center, float h_max)
{
    float d = glm::distance(glm::vec2(x, z), glm::vec2(center.x, center.z)) / r_cascada;

    float h = 0.0f;
    if (d < 1.0f) {
        h = (d * d / 2.0f) * h_max ;
    }
    else {
        h = (1.0f - ((2.0f - d) * (2.0f - d)) / 2.0f) * h_max ;
    }

    float noiseValue = glm::perlin(glm::vec2(x, z) * 0.2f) * 3.0f; // Scale noise frequency

    // Blend the smooth noise into the terrain
    float distance = sqrt(x * x + z * z);
    float blend = glm::clamp((distance) / 5.0f, 0.0f, 1.0f);

    h += noiseValue * blend;
    return h;
}


void Lab5::CreateMesh(const char* name, const std::vector<VertexFormat>& vertices, const std::vector<unsigned int>& indices)
{
    unsigned int VAO = 0;
    // TODO(student): Create the VAO and bind it
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO = 0;
    // TODO(student): Create the VBO and bind it
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // TODO(student): Send vertices data into the VBO buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    unsigned int IBO = 0;
    // TODO(student): Create the IBO and bind it
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    // TODO(student): Send indices data into the IBO buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);
    // ========================================================================
    // This section demonstrates how the GPU vertex shader program
    // receives data. It will be learned later, when GLSL shaders will be
    // introduced. For the moment, just think that each property value from
    // our vertex format needs to be sent to a certain channel, in order to
    // know how to receive it in the GLSL vertex shader.

    // Set vertex position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), 0);

    // Set vertex normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(sizeof(glm::vec3)));

    // Set texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(2 * sizeof(glm::vec3)));

    // Set vertex color attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));
    // ========================================================================

    // TODO(student): Unbind the VAO
    glBindVertexArray(0);
    // Check for OpenGL errors
    if (GetOpenGLError() == GL_INVALID_OPERATION)
    {
        cout << "\t[NOTE] : For students : DON'T PANIC! This error should go away when completing the tasks." << std::endl;
        cout << "\t[NOTE] : For developers : This happens because OpenGL core spec >=3.1 forbids null VAOs." << std::endl;
    }

    // Mesh information is saved into a Mesh object
    meshes[name] = new Mesh(name);
    meshes[name]->InitFromBuffer(VAO, static_cast<unsigned int>(indices.size()));
}


void Lab5::FrameStart()
{
}


void Lab5::Update(float deltaTimeSeconds)
{
    //The start of lab 6

    angle += 0.5f * deltaTimeSeconds;

    auto camera = GetSceneCamera();

    // Draw the scene in Framebuffer
    if (framebuffer_object)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);
        // Set the clear color for the color buffer
        glClearColor(0, 0, 0, 1);
        // Clears the color buffer (using the previously set color) and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, 1024, 1024);

        Shader* shader = shaders["Framebuffer"];
        shader->Use();

        glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

        {
            glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(30));

            glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
            glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projection));

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextureID);
            glUniform1i(glGetUniformLocation(shader->program, "texture_cubemap"), 1);

            glUniform1i(glGetUniformLocation(shader->program, "cube_draw"), 1);

            meshes["cubemap_night"]->Render();
        }

        {
            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix *= glm::translate(glm::mat4(1), glm::vec3(0, -0.5, 0));

            modelMatrix *= glm::scale(glm::mat4(1), glm::vec3(0.1f));

            glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

            glm::mat4 cubeView[6] =
            {
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // +X
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // -X
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)), // +Y
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)), // -Y
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // +Z
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // -Z
            };

            glUniformMatrix4fv(glGetUniformLocation(shader->GetProgramID(), "viewMatrices"), 6, GL_FALSE, glm::value_ptr(cubeView[0]));
            glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projection));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, TextureManager::GetTexture("ground.jpg")->GetTextureID());
            glUniform1i(glGetUniformLocation(shader->program, "texture_1"), 0);

            glUniform1i(glGetUniformLocation(shader->program, "cube_draw"), 0);
            meshes["terrain"]->Render();
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, color_texture);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        //reset drawing to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, window->GetResolution().x, window->GetResolution().y);

    // Draw the cubemap
    {
        frameBuffer->Bind();
        Shader* shader = shaders["ShaderNormal"];
        shader->Use();

        glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(100));

        glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
        glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextureID);
        int loc_texture = shader->GetUniformLocation("texture_cubemap");
        glUniform1i(loc_texture, 0);

        meshes["cubemap_night"]->Render();
    }


    // Draw the reflection on the mesh
    {
        Shader* shader = shaders["CubeMap"];
        shader->Use();

        glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(0.4f));

        glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
        glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));

        auto cameraPosition = camera->m_transform->GetWorldPosition();

        if (!color_texture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextureID);
            int loc_texture = shader->GetUniformLocation("texture_cubemap");
            glUniform1i(loc_texture, 0);
        }

        if (color_texture) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, color_texture);
            int loc_texture2 = shader->GetUniformLocation("texture_cubemap");
            glUniform1i(loc_texture2, 1);
        }


        int loc_camera = shader->GetUniformLocation("camera_position");
        glUniform3f(loc_camera, cameraPosition.x, cameraPosition.y, cameraPosition.z);

        glUniform1i(shader->GetUniformLocation("type"), type);

        meshes["bunny"]->Render();
    }

    //The end of lab 6

    for (auto& l : lights)
    {
        // TODO(student): Move the light sources in an orbit around the center of the scene.
        // The orbit is in the xoz plane. Compute rotationRadians for the current frame 
        // such that the light sources rotate 6 degrees/second. Use deltaTimeSeconds.
        float rotationRadians = RADIANS(6) * deltaTimeSeconds;

        glm::mat4 rotateMatrix = glm::rotate(glm::mat4(1.0f), rotationRadians, glm::vec3(0, 1, 0));
        l.position = glm::vec3(rotateMatrix * glm::vec4(l.position, 1.0f));

        //prima modificare
    }


    // ------------------------------------------------------------------------
    // Deferred rendering pass
    {
        //frameBuffer->Bind();

        auto shader = shaders["Render2Texture"];

        TextureManager::GetTexture("default.png")->BindToTextureUnit(GL_TEXTURE0);

        // Render scene objects
        RenderMesh(meshes["sphere"], shader, glm::vec3(-4, 1, 1));
        
        RenderMesh(meshes["terrain"], shader, glm::vec3(0, -2.5, 0));
        // Render a simple point light bulb for each light (for debugging purposes)
        TextureManager::GetTexture("default.png")->BindToTextureUnit(GL_TEXTURE0);
        for (auto &l : lights)
        {
            auto model = glm::translate(glm::mat4(1), l.position);
            model = glm::scale(model, glm::vec3(0.2f));
            RenderMesh(meshes["sphere"], shader, model);
        }

        TextureManager::GetTexture("ground.jpg")->BindToTextureUnit(GL_TEXTURE0);
    }

    // ------------------------------------------------------------------------
    // Lighting pass
    {
        glm::vec3 ambientLight(0.2f);
        //Set the initial light accumulation in each pixel to be equal to the ambient light.
        lightBuffer->SetClearColor(glm::vec4(ambientLight.x, ambientLight.y, ambientLight.z, 1.0f));
        lightBuffer->Bind();
        glClearColor(0, 0, 0, 1);

        // Enable buffer color accumulation
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ONE);

        auto shader = shaders["LightPass"];
        shader->Use();

        {
            int texturePositionsLoc = shader->GetUniformLocation("texture_position");
            glUniform1i(texturePositionsLoc, 0);
            frameBuffer->BindTexture(0, GL_TEXTURE0);
        }

        {
            int textureNormalsLoc = shader->GetUniformLocation("texture_normal");
            glUniform1i(textureNormalsLoc, 1);
            frameBuffer->BindTexture(1, GL_TEXTURE1);
        }

        auto camera = GetSceneCamera();
        glm::vec3 cameraPos = camera->m_transform->GetWorldPosition();
        int loc_eyePosition = shader->GetUniformLocation("eye_position");
        glUniform3fv(loc_eyePosition, 1, glm::value_ptr(cameraPos));

        auto resolution = window->GetResolution();
        int loc_resolution = shader->GetUniformLocation("resolution");
        glUniform2i(loc_resolution, resolution.x, resolution.y);

        //Front face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        for (auto& lightInfo : lights)
        {
            // TODO(student): Set the shader uniforms 'light_position', 'light_color' and 'light_radius'
            // with the values from the light source. Use shader 'shader'.
            location = glGetUniformLocation(shader->program, "light_position");
            glUniform3fv(location, 1, glm::value_ptr(lightInfo.position));

            location = glGetUniformLocation(shader->program, "light_color");
            glUniform3fv(location, 1, glm::value_ptr(lightInfo.color));

            location = glGetUniformLocation(shader->program, "light_radius");
            glUniform1f(location, lightInfo.radius);
            // TODO(student): Draw the mesh "sphere" at the position of the light source
            // and scaled 2 times the light source radius.
            // Use RenderMesh(mesh, shader, position, scale). Use shader 'shader'.
            RenderMesh(meshes["sphere"], shader, lightInfo.position, glm::vec3(2 * lightInfo.radius));
        }

        glDisable(GL_CULL_FACE);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // ------------------------------------------------------------------------
    // Composition pass
    {
        FrameBuffer::BindDefault();

        auto shader = shaders["Composition"];
        shader->Use();

        int outputTypeLoc = shader->GetUniformLocation("output_type");
        glUniform1i(outputTypeLoc, outputType);

        {
            int texturePositionsLoc = shader->GetUniformLocation("texture_position");
            glUniform1i(texturePositionsLoc, 1);
            frameBuffer->BindTexture(0, GL_TEXTURE0 + 1);
        }

        {
            int textureNormalsLoc = shader->GetUniformLocation("texture_normal");
            glUniform1i(textureNormalsLoc, 2);
            frameBuffer->BindTexture(1, GL_TEXTURE0 + 2);
        }

        {
            int textureColorLoc = shader->GetUniformLocation("texture_color");
            glUniform1i(textureColorLoc, 3);
            frameBuffer->BindTexture(2, GL_TEXTURE0 + 3);
        }

        {
            int textureDepthLoc = shader->GetUniformLocation("texture_depth");
            glUniform1i(textureDepthLoc, 4);
            frameBuffer->BindDepthTexture(GL_TEXTURE0 + 4);
        }

        {
            int textureLightLoc = shader->GetUniformLocation("texture_light");
            glUniform1i(textureLightLoc, 5);
            lightBuffer->BindTexture(0, GL_TEXTURE0 + 5);
        }

        // Render the object again but with different properties
        RenderMesh(meshes["quad"], shader, glm::vec3(0, 0, 0));
    }


}


void Lab5::FrameEnd()
{
    DrawCoordinateSystem();
}


void Lab5::LoadShader(const std::string &name)
{
    std::string shaderPath = PATH_JOIN(window->props.selfDir, SOURCE_PATH::M2, "lab5", "shaders");

    // Create a shader program for particle system
    {
        Shader *shader = new Shader(name);
        shader->AddShader(PATH_JOIN(shaderPath, name + ".VS.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, name + ".FS.glsl"), GL_FRAGMENT_SHADER);

        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }
}


unsigned int Lab5::UploadCubeMapTexture(const std::string& pos_x, const std::string& pos_y, const std::string& pos_z, const std::string& neg_x, const std::string& neg_y, const std::string& neg_z)
{
    int width, height, chn;

    unsigned char* data_pos_x = stbi_load(pos_x.c_str(), &width, &height, &chn, 0);
    unsigned char* data_pos_y = stbi_load(pos_y.c_str(), &width, &height, &chn, 0);
    unsigned char* data_pos_z = stbi_load(pos_z.c_str(), &width, &height, &chn, 0);
    unsigned char* data_neg_x = stbi_load(neg_x.c_str(), &width, &height, &chn, 0);
    unsigned char* data_neg_y = stbi_load(neg_y.c_str(), &width, &height, &chn, 0);
    unsigned char* data_neg_z = stbi_load(neg_z.c_str(), &width, &height, &chn, 0);

    unsigned int textureID = 0;
    // TODO(student): Create the texture
    glGenTextures(1, &textureID);
    // TODO(student): Bind the texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (GLEW_EXT_texture_filter_anisotropic) {
        float maxAnisotropy;

        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // TODO(student): Load texture information for each face
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pos_x);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pos_y);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pos_z);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_neg_x);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_neg_y);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_neg_z);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    if (GetOpenGLError() == GL_INVALID_OPERATION)
    {
        cout << "\t[NOTE] : For students : DON'T PANIC! This error should go away when completing the tasks." << std::endl;
    }

    // Free memory
    SAFE_FREE(data_pos_x);
    SAFE_FREE(data_pos_y);
    SAFE_FREE(data_pos_z);
    SAFE_FREE(data_neg_x);
    SAFE_FREE(data_neg_y);
    SAFE_FREE(data_neg_z);

    return textureID;
}

void Lab5::CreateFramebuffer(int width, int height)
{
    // TODO(student): In this method, use the attributes
    // 'framebuffer_object', 'color_texture'
    // declared in lab6.h

    // TODO(student): Generate and bind the framebuffer
    glGenFramebuffers(1, &framebuffer_object);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

    // TODO(student): Generate and bind the color texture
    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, color_texture);

    // TODO(student): Initialize the color textures
    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }


    if (color_texture) {
        //cubemap params
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (GLEW_EXT_texture_filter_anisotropic) {
            float maxAnisotropy;

            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Bind the color textures to the framebuffer as a color attachments
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_texture, 0);

        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        std::vector<GLenum> draw_textures;
        draw_textures.push_back(GL_COLOR_ATTACHMENT0);
        glDrawBuffers(draw_textures.size(), &draw_textures[0]);

    }

    // TODO(student): Generate and bind the depth texture
    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depth_texture);

    // TODO(student): Initialize the depth textures
    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }

    if (depth_texture) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture, 0);
    }

    glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Lab5::OnInputUpdate(float deltaTime, int mods)
{
    // Treat continuous update based on input
}


void Lab5::OnKeyPress(int key, int mods)
{
    // Add key press event

    // These are the key mappings for compositing different passes.
    // What does each key seem to activate? Where can you find the
    // answer? Examine the source code to find out!
    int index = key - GLFW_KEY_0;
    if (index >= 0 && index <= 9)
    {
        outputType = index;
    }

    if (key == GLFW_KEY_R) {
        for (auto& lightInfo : lights) {
            rand_info1 = Rand01();
            rand_info2 = Rand01();
            rand_info3 = Rand01();


            if (rand_info1 < Rand01()) {
                sign = 1;
            }
            else {
                sign = -1;
            }

            if (rand_info3 < Rand01()) {
                sign2 = 1;
            }
            else {
                sign2 = -1;
            }
            lightInfo.position = glm::vec3(rand_info1 * 10 * sign, rand_info2 * 3, rand_info3 * 10 * sign);
            lightInfo.color = glm::vec3(rand_info1, rand_info2, rand_info3);
            lightInfo.radius = 3 + Rand01();
        }
    }


}


void Lab5::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Lab5::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Lab5::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Lab5::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Lab5::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Treat mouse scroll event
}


void Lab5::OnWindowResize(int width, int height)
{
    // Treat window resize event
    frameBuffer->Resize(width, height, 32);
    lightBuffer->Resize(width, height, 32);
}
