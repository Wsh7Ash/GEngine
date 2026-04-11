#include "PrecisionEditTool.h"

#include "../math/MathUtils.h"
#include <imgui.h>

#include <algorithm>

namespace ge {
namespace editor {

PrecisionEditTool& PrecisionEditTool::Get() {
    static PrecisionEditTool instance;
    return instance;
}

PrecisionEditTool::PrecisionEditTool() {
    Initialize();
}

PrecisionEditTool::~PrecisionEditTool() {
}

void PrecisionEditTool::Initialize() {
    snapSettings_ = SnapSettings{};
    currentMode_ = GizmoMode::Translate;
}

void PrecisionEditTool::Shutdown() {
    isDragging_ = false;
}

void PrecisionEditTool::Update() {
    UpdateModifierState();
    ApplyKeyboardModifiers();
}

void PrecisionEditTool::SetSnapSettings(const SnapSettings& settings) {
    snapSettings_ = settings;
}

void PrecisionEditTool::SetPositionSnap(float value) {
    snapSettings_.positionSnap = std::max(0.01f, value);
}

void PrecisionEditTool::SetRotationSnap(float value) {
    snapSettings_.rotationSnap = std::max(0.1f, value);
}

void PrecisionEditTool::SetScaleSnap(float value) {
    snapSettings_.scaleSnap = std::max(0.01f, value);
}

void PrecisionEditTool::EnableSnap(bool enable) {
    snapSettings_.snapEnabled = enable;
}

void PrecisionEditTool::ToggleSnap() {
    snapSettings_.snapEnabled = !snapSettings_.snapEnabled;
}

void PrecisionEditTool::ApplyKeyboardModifiers() {
    snapSettings_.positionScaleFactor = 1.0f;
    snapSettings_.rotationScaleFactor = 1.0f;
    snapSettings_.scaleScaleFactor = 1.0f;

    if (shiftPressed_) {
        snapSettings_.positionScaleFactor = 0.5f;
        snapSettings_.rotationScaleFactor = 0.5f;
        snapSettings_.scaleScaleFactor = 0.5f;
    }

    if (ctrlPressed_) {
        snapSettings_.positionScaleFactor *= 2.0f;
        snapSettings_.rotationScaleFactor *= 2.0f;
        snapSettings_.scaleScaleFactor *= 2.0f;
    }
}

float PrecisionEditTool::SnapPosition(float value) const {
    if (!snapSettings_.snapEnabled) {
        return value;
    }

    const float snap = std::max(0.01f, snapSettings_.GetEffectivePositionSnap());
    return std::round(value / snap) * snap;
}

float PrecisionEditTool::SnapRotation(float value) const {
    if (!snapSettings_.snapEnabled) {
        return value;
    }

    const float snap = std::max(0.1f, snapSettings_.GetEffectiveRotationSnap());
    return std::round(value / snap) * snap;
}

float PrecisionEditTool::SnapScale(float value) const {
    if (!snapSettings_.snapEnabled) {
        return value;
    }

    const float snap = std::max(0.01f, snapSettings_.GetEffectiveScaleSnap());
    return std::round(value / snap) * snap;
}

Math::Vec3f PrecisionEditTool::SnapPosition(const Math::Vec3f& value) const {
    return {SnapPosition(value.x), SnapPosition(value.y), SnapPosition(value.z)};
}

Math::Vec3f PrecisionEditTool::SnapRotation(const Math::Vec3f& value) const {
    return {SnapRotation(value.x), SnapRotation(value.y), SnapRotation(value.z)};
}

Math::Vec3f PrecisionEditTool::SnapScale(const Math::Vec3f& value) const {
    Math::Vec3f snapped = {SnapScale(value.x), SnapScale(value.y), SnapScale(value.z)};
    if (snapSettings_.uniformScale) {
        const float uniform = snapped.x;
        snapped.y = uniform;
    }
    return snapped;
}

void PrecisionEditTool::ShowNumericInput(const std::string& label, float& value, float min, float max) {
    if (ImGui::DragFloat(label.c_str(), &value, 0.1f, min, max, "%.2f")) {
        switch (currentMode_) {
            case GizmoMode::Translate:
                value = SnapPosition(value);
                break;
            case GizmoMode::Rotate:
                value = SnapRotation(value);
                break;
            case GizmoMode::Scale:
                value = SnapScale(value);
                break;
        }
    }
}

void PrecisionEditTool::ShowNumericInput(const std::string& label, Math::Vec3f& value, float min, float max) {
    if (ImGui::DragFloat3(label.c_str(), &value.x, 0.1f, min, max, "%.2f")) {
        switch (currentMode_) {
            case GizmoMode::Translate:
                value = SnapPosition(value);
                if (onPositionChanged) {
                    onPositionChanged(value);
                }
                break;
            case GizmoMode::Rotate:
                value = SnapRotation(value);
                if (onRotationChanged) {
                    onRotationChanged(value);
                }
                break;
            case GizmoMode::Scale:
                value = SnapScale(value);
                if (onScaleChanged) {
                    onScaleChanged(value);
                }
                break;
        }
    }
}

void PrecisionEditTool::ShowNumericInput(const std::string& label, Math::Quatf& value) {
    Math::Vec3f euler = value.ToEuler();
    Math::Vec3f degrees = {
        Math::RadiansToDegrees(euler.x),
        Math::RadiansToDegrees(euler.y),
        Math::RadiansToDegrees(euler.z)
    };

    SetMode(GizmoMode::Rotate);
    ShowNumericInput(label, degrees, -360.0f, 360.0f);
    value = Math::Quatf::FromEuler(Math::DegreesToRadians(degrees.x),
                                   Math::DegreesToRadians(degrees.y),
                                   Math::DegreesToRadians(degrees.z));
}

void PrecisionEditTool::RenderUI() {
    if (ImGui::Checkbox("Snap", &snapSettings_.snapEnabled)) {
        ApplyKeyboardModifiers();
    }

    if (!snapSettings_.snapEnabled) {
        return;
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    switch (currentMode_) {
        case GizmoMode::Translate:
            ImGui::DragFloat("##PositionSnap", &snapSettings_.positionSnap, 0.05f, 0.05f, 10.0f, "%.2f");
            break;
        case GizmoMode::Rotate:
            ImGui::DragFloat("##RotationSnap", &snapSettings_.rotationSnap, 1.0f, 1.0f, 180.0f, "%.0f");
            break;
        case GizmoMode::Scale:
            ImGui::DragFloat("##ScaleSnap", &snapSettings_.scaleSnap, 0.05f, 0.05f, 10.0f, "%.2f");
            break;
    }
}

float PrecisionEditTool::GetModifierMultiplier() const {
    if (ctrlPressed_) {
        return 2.0f;
    }
    if (shiftPressed_) {
        return 0.5f;
    }
    return 1.0f;
}

void PrecisionEditTool::SetMode(GizmoMode mode) {
    currentMode_ = mode;
}

void PrecisionEditTool::OnGizmoDragBegin() {
    isDragging_ = true;
}

void PrecisionEditTool::OnGizmoDragEnd() {
    isDragging_ = false;
}

void PrecisionEditTool::HandleKeyboardInput() {
}

void PrecisionEditTool::UpdateModifierState() {
    if (!ImGui::GetCurrentContext()) {
        shiftPressed_ = false;
        ctrlPressed_ = false;
        altPressed_ = false;
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    shiftPressed_ = io.KeyShift;
    ctrlPressed_ = io.KeyCtrl;
    altPressed_ = io.KeyAlt;
    HandleKeyboardInput();
}

DeltaDisplay::DeltaDisplay() = default;

DeltaDisplay::~DeltaDisplay() = default;

void DeltaDisplay::Initialize() {
    positionDelta_ = Math::Vec3f::Zero();
    rotationDelta_ = Math::Vec3f::Zero();
    scaleDelta_ = Math::Vec3f::Zero();
    position_ = {24.0f, 64.0f};
}

void DeltaDisplay::Shutdown() {
}

void DeltaDisplay::BeginFrame() {
}

void DeltaDisplay::EndFrame() {
}

void DeltaDisplay::SetPositionDelta(const Math::Vec3f& delta) {
    positionDelta_ = delta;
}

void DeltaDisplay::SetRotationDelta(const Math::Vec3f& delta) {
    rotationDelta_ = delta;
}

void DeltaDisplay::SetScaleDelta(const Math::Vec3f& delta) {
    scaleDelta_ = delta;
}

void DeltaDisplay::Render() {
    if (!isVisible_) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(position_.x, position_.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.65f);
    if (ImGui::Begin("Transform Delta", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("Move  %.2f %.2f %.2f", positionDelta_.x, positionDelta_.y, positionDelta_.z);
        ImGui::Text("Rotate %.2f %.2f %.2f", rotationDelta_.x, rotationDelta_.y, rotationDelta_.z);
        ImGui::Text("Scale %.2f %.2f %.2f", scaleDelta_.x, scaleDelta_.y, scaleDelta_.z);
    }
    ImGui::End();
}

void DeltaDisplay::SetVisible(bool visible) {
    isVisible_ = visible;
}

void DeltaDisplay::SetPosition(const Math::Vec2f& pos) {
    position_ = pos;
}

void TransformInputPanel::Render(Math::Vec3f& position, Math::Vec3f& rotation, Math::Vec3f& scale) {
    bool changed = false;
    changed |= ImGui::DragFloat3("Position", &position.x, compact_ ? 0.05f : 0.1f, -10000.0f, 10000.0f, "%.2f");
    changed |= ImGui::DragFloat3("Rotation", &rotation.x, compact_ ? 0.5f : 1.0f, -360.0f, 360.0f, "%.2f");
    changed |= ImGui::DragFloat3("Scale", &scale.x, compact_ ? 0.05f : 0.1f, -1000.0f, 1000.0f, "%.2f");

    if (changed && onValuesChanged) {
        onValuesChanged();
    }
}

} // namespace editor
} // namespace ge
