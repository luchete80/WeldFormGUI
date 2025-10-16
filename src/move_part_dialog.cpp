#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "move_part_dialog.h"

#include <iostream>


using namespace std;

//// To know 
MoveCommand MovePartDialog::Draw(double &step, double* pos){

    MoveCommand cmd = {-1, 0.0, false};
    const char* labels[3] = {"X", "Y", "Z"};

    ImGui::Begin("Transform Controls");
    ImGui::Text("Step size:");
    ImGui::SameLine();
    ImGui::InputDouble("##delta", &step, 0.01, 1.0, "%.3f");

    for (int i = 0; i < 3; ++i)
    {
        ImGui::PushID(i);
        ImGui::Text("%s:", labels[i]);
        ImGui::SameLine(40);

        if (ImGui::Button("-")) {
            cmd.axis = i;
            cmd.delta = -step;
            cmd.active = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("+")) {
            cmd.axis = i;
            cmd.delta = step;
            cmd.active = true;
        }

        ImGui::SameLine(100);
        if (ImGui::Button("⬆")) { /* acción extra */ }
        ImGui::SameLine();
        ImGui::Text("Up to other piece");

        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::Text("Current position: X=%.3f, Y=%.3f, Z=%.3f", pos[0], pos[1], pos[2]);
    
    ImGui::End();
    return cmd;
}

