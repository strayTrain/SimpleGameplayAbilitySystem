#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "SimpleEventReplicator.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USimpleEventReplicator : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for components that need to replicate SimpleEvent subsystem events across the network.
 * Since SimpleEventSubsystem is a GameInstance subsystem (local only), components on replicated actors
 * implement this interface to handle event replication via RPCs.
 */
class SIMPLEGAMEPLAYABILITYSYSTEM_API ISimpleEventReplicator
{
	GENERATED_BODY()

public:
	/**
	 * Sends an event locally without any replication.
	 *
	 * @param EventTag The gameplay tag identifying the event
	 * @param DomainTag The domain tag categorizing the event
	 * @param Payload The payload of the event as an instanced struct
	 * @param Sender The object that sent the event
	 * @param ListenerFilter Only send the event to listeners in this list. If empty, send to all listeners.
	 */
	virtual void SendEvent(
		FGameplayTag EventTag,
		FGameplayTag DomainTag,
		FInstancedStruct Payload,
		UObject* Sender,
		const TArray<UObject*>& ListenerFilter) = 0;

	/**
	 * Sends an event to the server. If called on server, sends locally.
	 *
	 * @param EventTag The gameplay tag identifying the event
	 * @param DomainTag The domain tag categorizing the event
	 * @param Payload The payload of the event as an instanced struct
	 * @param Sender The object that sent the event
	 * @param ListenerFilter Only send the event to listeners in this list
	 */
	virtual void SendEventToServer(
		FGameplayTag EventTag,
		FGameplayTag DomainTag,
		FInstancedStruct Payload,
		UObject* Sender,
		const TArray<UObject*>& ListenerFilter) = 0;

	/**
	 * Sends an event to the owning client. Must be called on server.
	 *
	 * @param EventTag The gameplay tag identifying the event
	 * @param DomainTag The domain tag categorizing the event
	 * @param Payload The payload of the event as an instanced struct
	 * @param Sender The object that sent the event
	 * @param ListenerFilter Only send the event to listeners in this list
	 */
	virtual void SendEventToClient(
		FGameplayTag EventTag,
		FGameplayTag DomainTag,
		FInstancedStruct Payload,
		UObject* Sender,
		const TArray<UObject*>& ListenerFilter) = 0;

	/**
	 * Sends an event to all clients. Can be called from server or client.
	 * If called from client, sends locally first, then requests server to multicast.
	 * Internally generates a unique EventID to prevent the originating client from processing the event twice.
	 *
	 * @param EventTag The gameplay tag identifying the event
	 * @param DomainTag The domain tag categorizing the event
	 * @param Payload The payload of the event as an instanced struct
	 * @param Sender The object that sent the event
	 * @param ListenerFilter Only send the event to listeners in this list
	 */
	virtual void SendEventToAllClients(
		FGameplayTag EventTag,
		FGameplayTag DomainTag,
		FInstancedStruct Payload,
		UObject* Sender,
		const TArray<UObject*>& ListenerFilter) = 0;
};
