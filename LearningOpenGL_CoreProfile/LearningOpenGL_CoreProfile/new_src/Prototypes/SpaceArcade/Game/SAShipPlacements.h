#pragma once
#include "AssetConfigs/SAConfigBase.h"
#include "../Tools/DataStructures/SATransform.h"
#include "../GameFramework/SAWorldEntity.h"
#include "../GameFramework/RenderModelEntity.h"
#include "SAProjectileSystem.h"
#include <optional>

namespace SA
{
	enum class PlacementType
	{
		COMMUNICATIONS, DEFENSE, TURRET
	};
	std::string lexToString(PlacementType enumValue);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents the data side of the placeable object
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class PlacementSubConfig
	{
		friend class ModelConfigurerEditor_Level;
		friend class ShipPlacementEntity;
	private:
		glm::vec3 position{ 0.f };
		glm::vec3 rotation_deg{ 0.f }; //yaw, pitch, roll
		glm::vec3 scale{ 1.f };
		std::string filePath;
		PlacementType placeementType;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents the in world object that is associated with a ship. eg cannon, satellite, etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ShipPlacementEntity : public RenderModelEntity, public IProjectileHitNotifiable
	{
	public:
		ShipPlacementEntity() : RenderModelEntity(nullptr, Transform{})
		{}
	public:
		virtual void draw(Shader& shader) override;
		void setParentXform(glm::mat4 parentXform); //#TODO #scenenodes
		const glm::mat4& getParentXform() const { return parentXform; }
		virtual void setTransform(const Transform& inTransform) override;
		/** returns the model matrix considering the parent's transform*/
		const glm::mat4& getParentXLocalModelMatrix(){ return cachedModelMat_PxL; }
	protected:
		virtual void postConstruct() override;
	private:
		virtual void notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc) override;
		void updateModelMatrixCache();
	private: //model editor special access
		friend class ModelConfigurerEditor_Level;
		void replacePlacementConfig(const PlacementSubConfig& newConfig);
	private:
		sp<CollisionData> collision;
		PlacementSubConfig config;
		glm::mat4 cachedModelMat_PxL{ 1.f };
		glm::mat4 parentXform{ 1.f };
	};
}