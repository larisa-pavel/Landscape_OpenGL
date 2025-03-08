#pragma once

#include <string>
#include <vector>

#include "components/simple_scene.h"
#include "components/transform.h"
#include "core/gpu/frame_buffer.h"


namespace m2
{
    struct LightInfo
    {
        glm::vec3 position;
        glm::vec3 color;
        float radius;
    };

    class Lab5 : public gfxc::SimpleScene
    {
     public:
        Lab5();
        ~Lab5();

        void Init() override;

     private:
        void CreateMesh(const char* name, const std::vector<VertexFormat>& vertices, const std::vector<unsigned int>& indices);
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        void OnInputUpdate(float deltaTime, int mods) override;
        void OnKeyPress(int key, int mods) override;
        void OnKeyRelease(int key, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
        void OnWindowResize(int width, int height) override;

        void LoadShader(const std::string &fileName);
        float getHeight(float x, float z, float r_cascada, glm::vec3 center, float h_max);

        unsigned int UploadCubeMapTexture(const std::string& pos_x, const std::string& pos_y, const std::string& pos_z, const std::string& neg_x, const std::string& neg_y, const std::string& neg_z);
        void CreateFramebuffer(int width, int height);

     private:
        FrameBuffer *frameBuffer;
        FrameBuffer *lightBuffer;
        std::vector<LightInfo> lights;
        int outputType;

        float rand_info1;
        float rand_info2;
        float rand_info3;
        int sign = 1;
        int sign2 = 1;
        int location;
        int modify = 1;

        GLenum cullFace;
        GLenum polygonMode;

        //cubemap and reflexion
        int cubeMapTextureID;
        float angle;
        unsigned int framebuffer_object;
        unsigned int color_texture;
        unsigned int depth_texture;
        unsigned int type;
    };
}   // namespace m2
