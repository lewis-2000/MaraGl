#include "ScenePanel.h"
#include "HierarchyPanel.h"
#include "Renderer.h"
#include "Scene.h"
#include "TransformComponent.h"

#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

namespace MaraGl
{
    void ScenePanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        ImVec2 size = ImGui::GetContentRegionAvail();
        const ImVec2 fbScale = ImGui::GetIO().DisplayFramebufferScale;
        m_IsFocused = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        if (m_Framebuffer && size.x > 0.0f && size.y > 0.0f)
        {
            const float scaleX = fbScale.x > 0.0f ? fbScale.x : 1.0f;
            const float scaleY = fbScale.y > 0.0f ? fbScale.y : 1.0f;

            const uint32_t targetWidth = static_cast<uint32_t>(size.x * scaleX + 0.5f);
            const uint32_t targetHeight = static_cast<uint32_t>(size.y * scaleY + 0.5f);

            if (targetWidth != m_Framebuffer->getWidth() ||
                targetHeight != m_Framebuffer->getHeight())
            {
                m_Framebuffer->resize(targetWidth, targetHeight);
            }

            ImGui::Image(
                (ImTextureID)(uintptr_t)m_Framebuffer->getColorAttachment(),
                size,
                {0, 1},
                {1, 0});

            const ImVec2 imageMin = ImGui::GetItemRectMin();
            const ImVec2 imageMax = ImGui::GetItemRectMax();

            ImGui::SetCursorScreenPos(ImVec2(imageMin.x + 10.0f, imageMin.y + 10.0f));
            ImGui::BeginGroup();
            if (ImGui::SmallButton("Move"))
                m_Operation = GizmoOperation::Translate;
            ImGui::SameLine();
            if (ImGui::SmallButton("Scale"))
                m_Operation = GizmoOperation::Scale;
            ImGui::SameLine();
            ImGui::TextUnformatted(m_Operation == GizmoOperation::Translate ? "Move" : "Scale");
            ImGui::EndGroup();

            uint32_t selectedID = 0;
            TransformComponent *transform = nullptr;
            if (GetSelectedTransform(selectedID, transform) && transform && m_Renderer)
            {
                ImGuizmo::BeginFrame();
                ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
                ImGuizmo::SetRect(imageMin.x, imageMin.y, imageMax.x - imageMin.x, imageMax.y - imageMin.y);

                glm::mat4 modelMatrix = transform->GetMatrix();
                const glm::mat4 view = m_Renderer->GetCamera().GetView();
                const glm::mat4 projection = m_Renderer->GetCamera().GetProjection();

                ImGuizmo::OPERATION operation = (m_Operation == GizmoOperation::Translate)
                                                    ? ImGuizmo::TRANSLATE
                                                    : ImGuizmo::SCALE;
                ImGuizmo::MODE space = (m_Space == 0) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

                if (ImGuizmo::Manipulate(glm::value_ptr(view),
                                         glm::value_ptr(projection),
                                         operation,
                                         space,
                                         glm::value_ptr(modelMatrix)))
                {
                    float translation[3];
                    float rotation[3];
                    float scale[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix), translation, rotation, scale);
                    transform->Position = glm::vec3(translation[0], translation[1], translation[2]);
                    transform->Rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
                    transform->Scale = glm::vec3(scale[0], scale[1], scale[2]);
                }

                ImGui::SetCursorScreenPos(ImVec2(imageMin.x + 10.0f, imageMin.y + 38.0f));
                if (m_Operation != GizmoOperation::Scale)
                {
                    ImGui::RadioButton("Local", &m_Space, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("World", &m_Space, 1);
                }
            }
        }
        else
        {
            ImGui::TextUnformatted("Viewport unavailable");
        }

        ImGui::End();
    }

    bool ScenePanel::GetSelectedTransform(uint32_t &entityIDOut, TransformComponent *&transformOut) const
    {
        entityIDOut = 0;
        transformOut = nullptr;

        if (!m_Scene || !m_HierarchyPanel)
            return false;

        entityIDOut = m_HierarchyPanel->GetSelectedEntityID();
        if (entityIDOut == 0)
            return false;

        Entity *entity = m_Scene->FindEntityByID(entityIDOut);
        if (!entity)
            return false;

        transformOut = entity->GetComponent<TransformComponent>();
        return transformOut != nullptr;
    }
}
