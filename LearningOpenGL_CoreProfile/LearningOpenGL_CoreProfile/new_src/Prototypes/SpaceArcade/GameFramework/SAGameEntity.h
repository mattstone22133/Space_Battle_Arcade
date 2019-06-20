#pragma once
#include <memory>
#include "../Game/OptionalCompilationMacros.h"

namespace SA
{
///////////////////////////////////////////////////////////////////////////////////////////////////
//These forward declarations are carefully placed here to avoid circle includes.
//new_sp is desired to be responsible for calling post construct, and therefore 
//needs to know of the type "GameEntity". So that file will include "GameEntity"
//However, game entity needs to know the smart pointer aliases to friend them, 
//hence the forward declarations
//
//Note, Some of below are the real type aliases (eg see sp)

	//shared pointer
	template<typename T>
	using sp = std::shared_ptr<T>;

	template<typename T, typename... Args>
	sp<T> new_sp(Args&&... args);

	//weak pointer
	template<typename T>
	using wp = std::weak_ptr<T>;

	//unique pointer
	template<typename T>
	using up = std::unique_ptr<T>;

	template<typename T, typename... Args>
	up<T> new_up(Args&&... args);
///////////////////////////////////////////////////////////////////////////////////////////////////
}



/** Automatically provide template type for convenience */
#define sp_this() sp_this_impl<std::remove_reference<decltype(*this)>::type>()

/** Represents a top level object*/
namespace SA
{
	class GameEntity : public std::enable_shared_from_this<GameEntity>
 	{
	public:
		/** Game entities will all have virtual destructors to avoid easy-to-miss mistakes*/
		virtual ~GameEntity(){}

	protected:

		/** Not intended to be called directl; please use macro "sp_this" to avoid specifying template types*/
		template<typename T>
		sp<T> sp_this_impl()
		{
			//static cast safe because this must be called from derived classes 
			//static cast for speed; does not inccur RTTI overhead of dynamic cast
			return std::static_pointer_cast<T>(shared_from_this());
		}
	private:
		template<typename T, typename... Args>
		friend sp<T> new_sp(Args&&... args);
		 
		//new_sp will call this function after the object has been created, allowing GameEntities to subscribe to delegates immediately after construction
		virtual void postConstruct() {};
	};
}

/* Smart Pointer Alias
	Bodies of smart pointers alias functions
	Must come after definition of game entity
*/
namespace SA
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Usage example
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	int main(){
		using namespace SA;

		//shared pointer
		sp<EG_Class> ptr2 = new_sp<EG_Class>();
		ptr2->Foo();

		//weak pointer
		wp<EG_Class> wp1 = ptr2;
		if (sp<EG_Class> aquired = wp1.lock())
		{
			aquired->Foo();
		}

		//unique pointer
		up<EG_Class> up1 = new_up<EG_Class>();
		up1->Foo();
	}
	*/

	template<typename T, typename... Args>
	sp<T> new_sp(Args&&... args)
	{
		if constexpr (std::is_base_of<SA::GameEntity, T>::value)
		{
			sp<T> newObj = std::make_shared<T>(std::forward<Args>(args)...);
			//safe cast because of type-trait
			GameEntity* newGameEntity = reinterpret_cast<GameEntity*>(newObj.get());
			newGameEntity->postConstruct();
			return newObj;
		}
		else
		{
			return std::make_shared<T>(std::forward<Args>(args)...);
		}
	}

	template<typename T, typename... Args>
	up<T> new_up(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

}