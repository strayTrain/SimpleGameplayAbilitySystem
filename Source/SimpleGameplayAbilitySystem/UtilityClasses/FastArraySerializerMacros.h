#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

/**
 * @brief Declares the standard Add, Change, Remove delegates for a Fast Array Serializer item type.
 * @param ItemType The struct type deriving from FFastArraySerializerItem.
 * @param DelegatePrefix A unique prefix for the delegate type names (e.g., GameplayTagCounter).
 */
#define DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(ItemType, DelegatePrefix) \
    DECLARE_DELEGATE_OneParam(FOn##DelegatePrefix##Added, const ItemType&); \
    DECLARE_DELEGATE_OneParam(FOn##DelegatePrefix##Changed, const ItemType&); \
    DECLARE_DELEGATE_OneParam(FOn##DelegatePrefix##Removed, const ItemType&);

#define DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(ContainerType) \
    template<> struct TStructOpsTypeTraits<ContainerType> \
    : public TStructOpsTypeTraitsBase2<ContainerType> \
    { enum { WithNetDeltaSerializer = true }; };