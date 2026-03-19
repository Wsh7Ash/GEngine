#pragma once

#include "Command.h"
#include "../ecs/Entity.h"
#include "../ecs/World.h"
#include "../ecs/components/TransformComponent.h"
#include "../math/VecTypes.h"
#include "../math/quaternion.h"

namespace ge {
namespace cmd {

    class CommandChangeTransform : public ICommand {
    public:
        CommandChangeTransform(ecs::World& world, ecs::Entity entity, 
                             const Math::Vec3f& oldPos, const Math::Vec3f& newPos,
                             const Math::Quatf& oldRot, const Math::Quatf& newRot,
                             const Math::Vec3f& oldScale, const Math::Vec3f& newScale)
            : world_(world), entity_(entity), 
              oldPos_(oldPos), newPos_(newPos),
              oldRot_(oldRot), newRot_(newRot),
              oldScale_(oldScale), newScale_(newScale) {}

        virtual void Execute() override {
            if (!world_.IsAlive(entity_)) return;
            auto& tc = world_.GetComponent<ecs::TransformComponent>(entity_);
            tc.position = newPos_;
            tc.rotation = newRot_;
            tc.scale = newScale_;
        }

        virtual void Undo() override {
            if (!world_.IsAlive(entity_)) return;
            auto& tc = world_.GetComponent<ecs::TransformComponent>(entity_);
            tc.position = oldPos_;
            tc.rotation = oldRot_;
            tc.scale = oldScale_;
        }

        virtual bool MergeWith(ICommand* other) override {
            auto* command = dynamic_cast<CommandChangeTransform*>(other);
            if (command && command->entity_ == entity_) {
                // If the entities match, we can just update the "new" state to the latest one
                // This allows continuous dragging to be recorded as one command
                newPos_ = command->newPos_;
                newRot_ = command->newRot_;
                newScale_ = command->newScale_;
                return true;
            }
            return false;
        }

    private:
        ecs::World& world_;
        ecs::Entity entity_;
        Math::Vec3f oldPos_, newPos_;
        Math::Quatf oldRot_, newRot_;
        Math::Vec3f oldScale_, newScale_;
    };

} // namespace cmd
} // namespace ge
