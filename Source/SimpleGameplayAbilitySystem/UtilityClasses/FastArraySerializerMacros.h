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


template<typename TItem>
class IFastArrayItemHost
{
public:
    virtual ~IFastArrayItemHost() = default;

    virtual void NotifyItemAdded(const TItem& Item) = 0;
    virtual void NotifyItemRemoved(const TItem& Item) = 0;

    // Diffs TItem's snapshot against its current replicated values,
    // fires per-field delegates, and fires misprediction if bWasPredicted
    // and any field changed. Implemented per-component since it broadcasts
    // component-owned delegates.
    virtual void DiffAndBroadcast(TItem& Item, bool bWasPredicted) = 0;
};


#define FAST_ARRAY_ITEM_BODY(ArrayType)                                         \
public:                                                                         \
    bool bHasPendingPrediction = false;                                         \
    void SyncSnapshot();                                                        \
    void PreReplicatedRemove(const ArrayType& InArraySerializer);               \
    void PostReplicatedAdd(const ArrayType& InArraySerializer);                 \
    void PostReplicatedChange(const ArrayType& InArraySerializer);


#define FAST_ARRAY_BODY(ItemType, ArrayType)                                    \
    IFastArrayItemHost<ItemType>* OwningComponent = nullptr;                    \
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)                  \
    {                                                                           \
        return FFastArraySerializer::FastArrayDeltaSerialize<ItemType,          \
            ArrayType>(Items, DeltaParms, *this);                               \
    }


#define FAST_ARRAY_TYPE_TRAITS(ArrayType)                                       \
    template<>                                                                  \
    struct TStructOpsTypeTraits<ArrayType>                                      \
        : public TStructOpsTypeTraitsBase2<ArrayType>                           \
    { enum { WithNetDeltaSerializer = true }; };


#define IMPLEMENT_FAST_ARRAY_CALLBACKS(ItemType, ArrayType)                     \
    void ItemType::PostReplicatedAdd(const ArrayType& S)                        \
    {                                                                           \
        SyncSnapshot();                                                         \
        if (S.OwningComponent)                                                  \
            S.OwningComponent->NotifyItemAdded(*this);                          \
    }                                                                           \
    void ItemType::PreReplicatedRemove(const ArrayType& S)                      \
    {                                                                           \
        if (S.OwningComponent)                                                  \
            S.OwningComponent->NotifyItemRemoved(*this);                        \
    }                                                                           \
    void ItemType::PostReplicatedChange(const ArrayType& S)                     \
    {                                                                           \
        if (S.OwningComponent)                                                  \
        {                                                                       \
            const bool bWas = bHasPendingPrediction;                           \
            bHasPendingPrediction = false;                                      \
            S.OwningComponent->DiffAndBroadcast(*this, bWas);                  \
        }                                                                       \
    }


template<typename TItem, typename TArrayType>
struct TFastArrayOps
{
    void Init(TArrayType& InArray, IFastArrayItemHost<TItem>* Owner)
    {
        Array = &InArray;
        InArray.OwningComponent = Owner;
    }

    void Add(TItem NewItem)
    {
        check(Array);
        NewItem.SyncSnapshot();
        Array->Items.Add(MoveTemp(NewItem));
        Array->MarkArrayDirty();
    }
    
    void Add(TArray<TItem>& NewItems)
    {
        check(Array);
        for (TItem& Item : NewItems)
        {
            Item.SyncSnapshot();
            Array->Items.Add(MoveTemp(Item));
        }
        Array->MarkArrayDirty();
    }
    
    void Clear()
    {
        check(Array);
        Array->Items.Empty();
        Array->MarkArrayDirty();
    }

    bool Remove(TFunctionRef<bool(const TItem&)> Predicate)
    {
        check(Array);
        const int32 Idx = Array->Items.IndexOfByPredicate(Predicate);
        if (Idx == INDEX_NONE) return false;
        Array->Items.RemoveAt(Idx);
        Array->MarkArrayDirty();
        return true;
    }

    TItem* Find(TFunctionRef<bool(const TItem&)> Predicate)
    {
        check(Array);
        return Array->Items.FindByPredicate(Predicate);
    }

    void MarkItemDirty(TItem& Item)
    {
        check(Array);
        Array->MarkItemDirty(Item);
    }

    TArray<TItem>& GetItems()
    {
        check(Array);
        return Array->Items;
    }

private:
    TArrayType* Array = nullptr;
};
