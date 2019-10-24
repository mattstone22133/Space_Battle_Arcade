#pragma once
#include "SAGameEntity.h"
#include <vector>
#include <map>
#include <typeindex>
#include <assert.h>

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GameComponentBase - the base class for a fast access modular component.
	//		these components reserve a spot in a GameComponentEntity's array and should
	//		be used(ie inherited from) only when extremely fast access is needed. Their
	//		access is based on generating a template method for each subclass
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class GameComponentBase : public GameEntity
	{
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SystemComponentBase - the base class for normal access modular component.
	//		These are for components that do not need rapid access. They are stored
	//		in a associate like structure. Their access is slower than dynamic casting
	//		to a type, and much slower than the template-array-based system used for
	//		game components. However, they do not have the high memory costs of gameplay
	//		components
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class SystemComponentBase : public GameEntity
	{
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System Component Entity
	//		These components have slower access, but each entity only requires memory for the components 
	//		the entity owns. Performance has show tree based map performs better than hash based map in release mode
	//		(see learningcpprepro "TestComponentSystemPerformance.cpp")
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class SystemComponentEntity : public GameEntity
	{
	public:
		template<typename ComponentType>
		void createSystemComponent()
		{
			static_assert(std::is_base_of<SystemComponentBase, ComponentType>::value);

			//have this template cache type_index since typeid may call virtual dispatch
			static std::type_index typeIndex = typeid(ComponentType);

			if (components.find(typeIndex) == components.end())
			{
				const sp<ComponentType>& component = new_sp<ComponentType>();
				components.insert({ typeIndex, component });
			}
			else
			{
				log("SystemComponentEntity", LogLevel::LOG_WARNING, "trying to create component that is already present");
			}
		}

		template<typename ComponentType>
		void deleteSystemComponent()
		{
			static_assert(std::is_base_of<SystemComponentBase, ComponentType>::value);
			static std::type_index typeIndex = typeid(ComponentType);

			auto findResult = components.find(typeIndex);
			if (findResult != components.end())
			{
				components.erase(findResult);
			}
			else
			{
				log("SystemComponentEntity", LogLevel::LOG_WARNING, "trying to delete component that doesn't exist");
			}
		}

		template<typename ComponentType>
		bool hasSystemComponent() const
		{
			static_assert(std::is_base_of<SystemComponentBase, ComponentType>::value);
			static std::type_index typeIndex = typeid(ComponentType);

			return components.find(typeIndex) != components.end();
		}

		template<typename ComponentType>
		ComponentType* getSystemComponent() //#TODO set up const access via const_cast idiom
		{
			static_assert(std::is_base_of<SystemComponentBase, ComponentType>::value);
			static std::type_index typeIndex = typeid(ComponentType);

			auto findResult = components.find(typeIndex);
			if (findResult != components.end())
			{
				//static cast is safe since we've verified the type_index
				return static_cast<ComponentType*>(findResult->second.get());
			}
			else
			{
				return nullptr;
			}
		}

	private:
		/** Profiling has show map has better performance than unordered_map in release mode with 26 components.*/
		std::map<std::type_index, sp<SystemComponentBase>> components;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ComponentEntity - Component entity is an game entity that manages modular components
	//		This primary is to have a dynamic "hasA" relationship.
	//
	//	Implementation details: subject to change 
	//		In order to make an extremely fast component retrieval system, templates are used.
	//		Each component gets its own instantiated method, for free, without any set up needed.
	//		This comes at a memory cost. each component entity holds an array of (shared) pointers
	//		the size of the total components in the game, regardless if the object has all of those components.
	//		Profiling this system (see learningcpprepro "TestComponentSystemPerformance.cpp") for performance
	//		shown this ordering for potential systems: TemplateMethod < DynamicCast < Map < HashMap. 
	//		to save memory, this should be switched to a map based system. But practically there should be a
	//		a small set of actual components in the engine, so the TemplateMethod based approach is used for now.
	//
	//  If this component isn't needed in rapid access (ie in a tick function) then it may be better to use the 
	//		system component system instead. All GameplayComponentEntities are SystemComponentEntities too.
	//		adding gameplay components increase the memory profile of all gameplay component entities by a managed pointer
	//		(it is nullptr if the entity doesn't have that component); this makes the system not scale well if a large
	//		number of components are used - hence the careful thought around creating a new gameplay component should be given.
	//		that being said, if you're doing dynamic_casts in tick methods / gamescenarios then
	//		the gameplay component is probably worth the memory overhead.
	//		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class GameplayComponentEntity : public SystemComponentEntity
	{
	public:
		template<typename ComponentType>
		void createGameComponent()
		{
			static_assert(std::is_base_of<GameComponentBase, ComponentType>::value);
			_componentOperation<ComponentType>(ComponentOp::CREATE_OP);
		}

		template<typename ComponentType>
		void deleteGameComponent()
		{
			static_assert(std::is_base_of<GameComponentBase, ComponentType>::value);
			_componentOperation<ComponentType>(ComponentOp::DELETE_OP);
		}

		template<typename ComponentType>
		bool hasGameComponent() const
		{
			static_assert(std::is_base_of<GameComponentBase, ComponentType>::value);
			return _componentOperation<ComponentType>(ComponentOp::GET_OP) != nullptr;
		}

		template<typename ComponentType>
		ComponentType* getGameComponent() //TODO if used properly set up const and non const versions of this
		{
			static_assert(std::is_base_of<GameComponentBase, ComponentType>::value);
			return _componentOperation<ComponentType>(ComponentOp::GET_OP);
		}

	private:
		/** Operations into the single method generated for each component */
		enum class ComponentOp : uint8_t { GET_OP, CREATE_OP, DELETE_OP };	//_COMP added to avoid namespace collisions with windows macros

		/** method generated for each component type in the system. */
		template<typename ComponentType>
		inline ComponentType* _componentOperation(ComponentOp operation)
		{
			//this will create many different functions that automatically know the index of the type
			//operations all live in the same function so that the this_type_index is always the same

			static_assert(std::is_base_of<GameComponentBase, ComponentType>::value);
			static size_t this_type_index = nextComponentCreationIndex++; //first time this is called, it gets a new index for fast compile time lookup
			static int calledOneTimeAtIncrement = []()->int{
				if (this_type_index == 50)
				{
					log("ComponentSystem", LogLevel::LOG_WARNING, "Warning: fast component system is optimized for small number of components; this number is now 50 which means every component entity has 50*sizeof(componentSmartPtr) array. Switching to a map based system may be advised.");
				}
			}();

			if (componentArray.size() <= this_type_index + 1)
			{
				componentArray.resize(this_type_index + 1, nullptr);
			}

			ComponentType* ret = nullptr;

			//avoid switch because I heard they suck with cache... probably should try and test that.
			if (operation == ComponentOp::GET_OP)
			{
				//static cast is safe since we're using index look up as a proxy for RTTI
				ret = static_cast<ComponentType*>(componentArray[this_type_index].get());
			}
			else if (operation == ComponentOp::CREATE_OP)
			{
				if (componentArray[this_type_index] == nullptr)
				{
					componentArray[this_type_index] = new_sp<ComponentType>();
					ret = static_cast<ComponentType*>(componentArray[this_type_index].get());
				}
				else
				{
					log("component system", LogLevel::LOG_WARNING, "creating component that already exists");
					assert(false);
				}
			}
			else if (operation == ComponentOp::DELETE_OP)
			{
				if (sp<GameComponentBase>& comp = componentArray[this_type_index])
				{
					componentArray[this_type_index] = nullptr;
				}
				else
				{
					log("component system", LogLevel::LOG_WARNING, "deleting component that already exists");
					assert(false);
				}
			}
			return ret;
		}
	private:

		static size_t nextComponentCreationIndex;
		std::vector<sp<GameComponentBase>> componentArray;
	};
	


}