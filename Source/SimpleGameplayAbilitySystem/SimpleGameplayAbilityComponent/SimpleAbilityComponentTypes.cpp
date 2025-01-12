#include "SimpleAbilityComponentTypes.h"

void FFloatAttribute::PreReplicatedRemove(const struct FFloatAttributeContainer& InArraySerializer)
{
	InArraySerializer.OnFloatAttributeRemoved.ExecuteIfBound(*this);
}

void FFloatAttribute::PostReplicatedAdd(const struct FFloatAttributeContainer& InArraySerializer)
{
	InArraySerializer.OnFloatAttributeAdded.ExecuteIfBound(*this);
}

void FFloatAttribute::PostReplicatedChange(const struct FFloatAttributeContainer& InArraySerializer)
{
	InArraySerializer.OnFloatAttributeChanged.ExecuteIfBound(*this);
}
