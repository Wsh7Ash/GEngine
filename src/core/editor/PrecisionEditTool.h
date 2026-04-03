#pragma once

// ================================================================
//  PrecisionEditTool.h
//  Precision editing with snap values and keyboard modifiers.
// ================================================================

#include "TransformGizmo.h"
#include <string>
#include <functional>

namespace ge {
namespace editor {

struct SnapSettings {
    float positionSnap = 0.5f;
    float rotationSnap = 15.0f;
    float scaleSnap = 0.5f;
    
    float positionScaleFactor = 1.0f;
    float rotationScaleFactor = 1.0f;
    float scaleScaleFactor = 1.0f;
    
    bool snapEnabled = true;
    bool uniformScale = true;
    
    float GetEffectivePositionSnap() const { return positionSnap * positionScaleFactor; }
    float GetEffectiveRotationSnap() const { return rotationSnap * rotationScaleFactor; }
    float GetEffectiveScaleSnap() const { return scaleSnap * scaleScaleFactor; }
};

class PrecisionEditTool {
public:
    static PrecisionEditTool& Get();
    
    PrecisionEditTool();
    ~PrecisionEditTool();
    
    void Initialize();
    void Shutdown();
    
    void Update();
    
    void SetSnapSettings(const SnapSettings& settings);
    SnapSettings& GetSnapSettings() { return snapSettings_; }
    const SnapSettings& GetSnapSettings() const { return snapSettings_; }
    
    void SetPositionSnap(float value);
    void SetRotationSnap(float value);
    void SetScaleSnap(float value);
    
    void EnableSnap(bool enable);
    void ToggleSnap();
    bool IsSnapEnabled() const { return snapSettings_.snapEnabled; }
    
    void ApplyKeyboardModifiers();
    
    float SnapPosition(float value) const;
    float SnapRotation(float value) const;
    float SnapScale(float value) const;
    
    Math::Vec3f SnapPosition(const Math::Vec3f& value) const;
    Math::Vec3f SnapRotation(const Math::Vec3f& value) const;
    Math::Vec3f SnapScale(const Math::Vec3f& value) const;
    
    void ShowNumericInput(const std::string& label, float& value, float min = -10000.0f, float max = 10000.0f);
    void ShowNumericInput(const std::string& label, Math::Vec3f& value, float min = -10000.0f, float max = 10000.0f);
    void ShowNumericInput(const std::string& label, Math::Quatf& value);
    
    void RenderUI();
    
    bool IsShiftPressed() const { return shiftPressed_; }
    bool IsCtrlPressed() const { return ctrlPressed_; }
    bool IsAltPressed() const { return altPressed_; }
    
    float GetModifierMultiplier() const;
    
    void SetMode(GizmoMode mode);
    GizmoMode GetMode() const { return currentMode_; }
    
    void OnGizmoDragBegin();
    void OnGizmoDragEnd();
    
    std::function<void(const Math::Vec3f&)> onPositionChanged;
    std::function<void(const Math::Vec3f&)> onRotationChanged;
    std::function<void(const Math::Vec3f&)> onScaleChanged;
    
private:
    void HandleKeyboardInput();
    void UpdateModifierState();
    
    SnapSettings snapSettings_;
    GizmoMode currentMode_ = GizmoMode::Translate;
    
    bool shiftPressed_ = false;
    bool ctrlPressed_ = false;
    bool altPressed_ = false;
    
    bool isDragging_ = false;
    Math::Vec3f dragStartPosition_;
    Math::Vec3f dragStartRotation_;
    Math::Vec3f dragStartScale_;
    
    bool showNumericDialog_ = false;
    std::string numericDialogLabel_;
    float* numericDialogValue_ = nullptr;
    float numericDialogMin_ = -10000.0f;
    float numericDialogMax_ = 10000.0f;
    int numericDialogComponent_ = 0;
    enum class NumericDialogTarget { Position, Rotation, Scale } numericDialogTarget_ = NumericDialogTarget::Position;
};

class DeltaDisplay {
public:
    DeltaDisplay();
    ~DeltaDisplay();
    
    void Initialize();
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void SetPositionDelta(const Math::Vec3f& delta);
    void SetRotationDelta(const Math::Vec3f& delta);
    void SetScaleDelta(const Math::Vec3f& delta);
    
    void Render();
    
    void SetVisible(bool visible);
    bool IsVisible() const { return isVisible_; }
    
    void SetPosition(const Math::Vec2f& pos);
    Math::Vec2f GetPosition() const { return position_; }
    
private:
    Math::Vec3f positionDelta_;
    Math::Vec3f rotationDelta_;
    Math::Vec3f scaleDelta_;
    
    Math::Vec2f position_;
    bool isVisible_ = true;
    bool showValues_ = true;
};

class TransformInputPanel {
public:
    TransformInputPanel() = default;
    ~TransformInputPanel() = default;
    
    void Render(Math::Vec3f& position, Math::Vec3f& rotation, Math::Vec3f& scale);
    
    void SetCompact(bool compact) { compact_ = compact; }
    bool IsCompact() const { return compact_; }
    
    std::function<void()> onValuesChanged;
    
private:
    bool compact_ = false;
    int activeField_ = -1;
};

} // namespace editor
} // namespace ge
