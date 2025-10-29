Option 1, show 1 frame at a time

int currentFrame = 0;

if (!results.frames.empty()) {
    viewer->addActor(results.frames[currentFrame]->actor);
}


ImGui::SliderInt("Frame", &currentFrame, 0, results.frames.size() - 1);
if (ImGui::IsItemEdited()) {
    viewer->clearActors();
    viewer->addActor(results.frames[currentFrame]->actor);
}


Option 2. Show all with different opacity

for (size_t i = 0; i < results.frames.size(); ++i) {
    auto& frame = results.frames[i];
    frame->actor->GetProperty()->SetOpacity(i == currentFrame ? 1.0 : 0.0);
}



Automatic Animation
static int currentFrame = 0;
static double lastTime = ImGui::GetTime();

if (ImGui::GetTime() - lastTime > 0.1) { // 10 fps
    currentFrame = (currentFrame + 1) % results.frames.size();
    viewer->clearActors();
    viewer->addActor(results.frames[currentFrame]->actor);
    lastTime = ImGui::GetTime();
}
