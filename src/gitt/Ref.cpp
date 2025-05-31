#include "Ref.h"

namespace clone
{
	std::string HeadRef::sha() const
	{
		return this->ref.object_id; 
	}
}