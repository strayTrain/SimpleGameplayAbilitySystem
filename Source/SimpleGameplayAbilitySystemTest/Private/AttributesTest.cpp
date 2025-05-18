// Copyright Rancorous Games, 2024

#include "AttributesTest.h"

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "NativeGameplayTags.h"
#include "Framework/DebugTestResult.h"

#include "SGASCommonTestSetup.cpp"
#include "MockClasses/AttributeEventReceiver.h"

#define TestNamePrefix "GameTests.SGAS.Attributes"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttributesTest_BasicManipulation, TestNamePrefix ".BasicManipulation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

class FAttributesTestContext
{
public:
	FAttributesTestContext(FName TestNameSuffix)
		: TestFixture(FName(*(FString(TestNamePrefix) + TestNameSuffix.ToString()))),
		  Character(nullptr),
		  SGASComponent(nullptr)
	{
		World = TestFixture.GetWorld();
		if (World)
		{
			Character = World->SpawnActor<ACharacter>();
			if (Character)
			{
				SGASComponent = NewObject<USimpleGameplayAbilityComponent>(Character, TEXT("TestSGASComponent"));
				if (SGASComponent)
				{
					SGASComponent->RegisterComponent();
				}
			}
		}
	}

	~FAttributesTestContext()
	{
		if (Character)
		{
			Character->Destroy();
			Character = nullptr;
		}
	}

	FTestFixture TestFixture;
	UWorld* World;
	ACharacter* Character;
	USimpleGameplayAbilityComponent* SGASComponent;
};


class FAttributesTestScenarios
{
public:
	FAutomationTestBase* Test;

	FAttributesTestScenarios(FAutomationTestBase* InTest)
		: Test(InTest)
	{
	}

	bool TestBasicAttributeManipulation() const
	{
		const FName TestContextName = TEXT(".BasicManipulationScenario");
		FAttributesTestContext Context(TestContextName);
		FDebugTestResult Res;
		const float Tolerance = 0.001f;

		// --- Initial Setup Checks ---
		Res &= Test->TestNotNull(TEXT("BasicManipulation: World should be created"), Context.World);
		if (!Context.World) return Res;

		Res &= Test->TestNotNull(TEXT("BasicManipulation: Character should be spawned"), Context.Character);
		if (!Context.Character) return Res;

		Res &= Test->TestNotNull(TEXT("BasicManipulation: SGASComponent should be created"), Context.SGASComponent);
		if (!Context.SGASComponent) return Res;

		// For basic tests, HasAuthority should ideally be true.
		Res &= Test->TestTrue(
			TEXT("BasicManipulation: Component should have authority for this test"),
			Context.SGASComponent->HasAuthority());
		if (!Context.SGASComponent->HasAuthority())
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("BasicManipulation: Authority check failed. Test assumes server-side operations."));
		}

		// --- Attribute Definition (No Regen parameters relevant here) ---
		FFloatAttribute TestAttr;
		TestAttr.AttributeName = TEXT("TestHealth");
		TestAttr.AttributeTag = TestAttributeTag;
		TestAttr.BaseValue = 100.0f;
		TestAttr.CurrentValue = 80.0f;

		// --- Test AddFloatAttribute ---
		FGameplayTag DomainTag = TestAttributeTag; // Here we use the attribute id for the domain tag but below we use Authority
		UAttributeEventReceiver* AddEventReceiver = NewObject<UAttributeEventReceiver>();
		AddEventReceiver->ExpectedEventTag = FDefaultTags::FloatAttributeAdded();
		AddEventReceiver->ExpectedDomainTag = DomainTag;
		AddEventReceiver->ExpectedSenderActor = Context.SGASComponent ? Context.SGASComponent->GetOwner() : nullptr;

		USimpleEventSubsystem* EventSubsystem = Context.TestFixture.GetSubsystem();
		FGuid AddEventSubID;

		
		if (EventSubsystem && AddEventReceiver->ExpectedSenderActor)
		{
			FSimpleEventDelegate Delegate;
			Delegate.BindDynamic(AddEventReceiver, &UAttributeEventReceiver::HandleEvent);
			TArray<UObject*> SenderFilter = {AddEventReceiver->ExpectedSenderActor};
			AddEventSubID = EventSubsystem->ListenForEvent(AddEventReceiver, true, // Listen once
			                                               FGameplayTagContainer(FDefaultTags::FloatAttributeAdded()),
			                                               FGameplayTagContainer(DomainTag),
			                                               Delegate, {}, SenderFilter);
		}

		Context.SGASComponent->AddFloatAttribute(TestAttr);

		if (EventSubsystem)
		{
			Res &= Test->TestTrue(
				TEXT("BasicManipulation: AttributeAddedEvent should have fired"), AddEventReceiver->bEventFired);
		}
		if (AddEventSubID.IsValid() && EventSubsystem) EventSubsystem->StopListeningForEventSubscriptionByID(
			AddEventSubID);


		// --- Test GetFloatAttributeValue (CurrentValue and BaseValue) ---
		bool bWasFound = false;
		float Value = Context.SGASComponent->GetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag,
		                                                            bWasFound);
		Res &= Test->TestTrue(TEXT("BasicManipulation: Attribute should be found after add (CurrentValue)"), bWasFound);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: CurrentValue after add should be 80.0f"), Value, 80.0f, Tolerance);

		Value = Context.SGASComponent->GetFloatAttributeValue(EAttributeValueType::BaseValue, TestAttributeTag,
		                                                      bWasFound);
		Res &= Test->TestTrue(TEXT("BasicManipulation: Attribute should be found after add (BaseValue)"), bWasFound);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: BaseValue after add should be 100.0f"), Value, 100.0f, Tolerance);

		// --- Test HasFloatAttribute ---
		Res &= Test->TestTrue(
			TEXT("BasicManipulation: HasFloatAttribute should be true after add"),
			Context.SGASComponent->HasFloatAttribute(TestAttributeTag));

		// --- Test SetFloatAttributeValue (CurrentValue) ---
		DomainTag = FDefaultTags::AuthorityAttributeDomain(); // Why are we using AuthorityAttributeDomain here but the attributeid elsewhere
		UAttributeEventReceiver* SetEventReceiver = NewObject<UAttributeEventReceiver>();
		SetEventReceiver->ExpectedEventTag = FDefaultTags::FloatAttributeCurrentValueChanged();
		SetEventReceiver->ExpectedDomainTag = DomainTag;
		SetEventReceiver->ExpectedSenderActor = Context.SGASComponent ? Context.SGASComponent->GetOwner() : nullptr;
		FGuid SetEventSubID;

		if (EventSubsystem && SetEventReceiver->ExpectedSenderActor)
		{
			FSimpleEventDelegate Delegate;
			Delegate.BindDynamic(SetEventReceiver, &UAttributeEventReceiver::HandleEvent);
			TArray<UObject*> SenderFilter = {SetEventReceiver->ExpectedSenderActor};
			SetEventSubID = EventSubsystem->ListenForEvent(SetEventReceiver, true,
			                                               FGameplayTagContainer(
				                                               FDefaultTags::FloatAttributeCurrentValueChanged()),
			                                               //FGameplayTagContainer(TestAttr.AttributeTag),
			                                               FGameplayTagContainer(
				                                               DomainTag),
			                                               Delegate, {}, SenderFilter);
		}

		float Overflow = 0.f;
		Context.SGASComponent->SetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag, 90.0f,
		                                              Overflow);

		if (EventSubsystem)
		{
			Res &= Test->TestTrue(
				TEXT("BasicManipulation: AttributeCurrentValueChangedEvent for Set should have fired"),
				SetEventReceiver->bEventFired);
		}
		if (SetEventSubID.IsValid() && EventSubsystem) EventSubsystem->StopListeningForEventSubscriptionByID(
			SetEventSubID);


		Value = Context.SGASComponent->GetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag,
		                                                      bWasFound);
		Res &= Test->TestTrue(TEXT("BasicManipulation: Attribute should be found after set"), bWasFound);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: CurrentValue after set should be 90.0f"), Value, 90.0f, Tolerance);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: Overflow after non-overflowing set should be 0.0f"), Overflow, 0.0f, Tolerance);


		// --- Test IncrementFloatAttributeValue (CurrentValue) ---
		// Event receiver setup for CurrentValue Change (from increment)
		UAttributeEventReceiver* IncEventReceiver = NewObject<UAttributeEventReceiver>();
		IncEventReceiver->ExpectedEventTag = FDefaultTags::FloatAttributeCurrentValueChanged();
		IncEventReceiver->ExpectedDomainTag = DomainTag;
		IncEventReceiver->ExpectedSenderActor = Context.SGASComponent ? Context.SGASComponent->GetOwner() : nullptr;
		FGuid IncEventSubID;

		if (EventSubsystem && IncEventReceiver->ExpectedSenderActor)
		{
			FSimpleEventDelegate Delegate;
			Delegate.BindDynamic(IncEventReceiver, &UAttributeEventReceiver::HandleEvent);
			TArray<UObject*> SenderFilter = {IncEventReceiver->ExpectedSenderActor};
			IncEventSubID = EventSubsystem->ListenForEvent(IncEventReceiver, true,
			                                               FGameplayTagContainer(
				                                               FDefaultTags::FloatAttributeCurrentValueChanged()),
			                                               FGameplayTagContainer(DomainTag),
			                                               Delegate, {}, SenderFilter);
		}

		Context.SGASComponent->IncrementFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag, 5.0f,
		                                                    Overflow); // 90 + 5 = 95

		if (EventSubsystem)
		{
			Res &= Test->TestTrue(
				TEXT("BasicManipulation: AttributeCurrentValueChangedEvent for Increment should have fired"),
				IncEventReceiver->bEventFired);
		}
		if (IncEventSubID.IsValid() && EventSubsystem) EventSubsystem->StopListeningForEventSubscriptionByID(
			IncEventSubID);


		Value = Context.SGASComponent->GetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag,
		                                                      bWasFound);
		Res &= Test->TestTrue(TEXT("BasicManipulation: Attribute should be found after increment"), bWasFound);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: CurrentValue after increment should be 95.0f"), Value, 95.0f, Tolerance);

		// --- Test SetFloatAttributeValue with Clamping (MaxValue) ---
		TestAttr.ValueLimits.UseMaxCurrentValue = true;
		TestAttr.ValueLimits.MaxCurrentValue = 100.0f;
		Context.SGASComponent->OverrideFloatAttribute(TestAttributeTag, TestAttr); // Re-add with MaxValue limit

		Context.SGASComponent->SetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag, 120.0f,
		                                              Overflow); // Try to set above max
		Value = Context.SGASComponent->GetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag,
		                                                      bWasFound);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: CurrentValue after set above max should be clamped to 100.0f"), Value, 100.0f,
			Tolerance);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: Overflow after set above max should be 20.0f"), Overflow, 20.0f, Tolerance);

		// --- Test SetFloatAttributeValue with Clamping (MinValue) ---
		TestAttr.ValueLimits.UseMinCurrentValue = true;
		TestAttr.ValueLimits.MinCurrentValue = 10.0f;
		Context.SGASComponent->OverrideFloatAttribute(TestAttributeTag, TestAttr); // Re-add with MinValue limit

		Context.SGASComponent->SetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag, 5.0f,
		                                              Overflow); // Try to set below min
		Value = Context.SGASComponent->GetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag,
		                                                      bWasFound);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: CurrentValue after set below min should be clamped to 10.0f"), Value, 10.0f,
			Tolerance);
		Res &= Test->TestNearlyEqual(
			TEXT("BasicManipulation: Overflow after set below min should be -5.0f (Value - Min)"), Overflow, -5.0f,
			Tolerance); // Overflow can be negative

		
		// --- Test RemoveFloatAttribute ---
		DomainTag = TestAttributeTag; // Again here the domain is the attribute id
		UAttributeEventReceiver* RemoveEventReceiver = NewObject<UAttributeEventReceiver>();
		RemoveEventReceiver->ExpectedEventTag = FDefaultTags::FloatAttributeRemoved();
		RemoveEventReceiver->ExpectedDomainTag = DomainTag;
		RemoveEventReceiver->ExpectedSenderActor = Context.SGASComponent ? Context.SGASComponent->GetOwner() : nullptr;
		FGuid RemoveEventSubID;

		if (EventSubsystem && RemoveEventReceiver->ExpectedSenderActor)
		{
			FSimpleEventDelegate Delegate;
			Delegate.BindDynamic(RemoveEventReceiver, &UAttributeEventReceiver::HandleEvent);
			TArray<UObject*> SenderFilter = {RemoveEventReceiver->ExpectedSenderActor};
			RemoveEventSubID = EventSubsystem->ListenForEvent(RemoveEventReceiver, true,
			                                                  FGameplayTagContainer(
				                                                  FDefaultTags::FloatAttributeRemoved()),
			                                                  FGameplayTagContainer(DomainTag),
			                                                  Delegate, {}, SenderFilter);
		}

		Context.SGASComponent->RemoveFloatAttribute(TestAttributeTag);

		if (EventSubsystem)
		{
			Res &= Test->TestTrue(
				TEXT("BasicManipulation: AttributeRemovedEvent should have fired"), RemoveEventReceiver->bEventFired);
		}
		if (RemoveEventSubID.IsValid() && EventSubsystem) EventSubsystem->StopListeningForEventSubscriptionByID(
			RemoveEventSubID);

		Res &= Test->TestFalse(
			TEXT("BasicManipulation: HasFloatAttribute should be false after remove"),
			Context.SGASComponent->HasFloatAttribute(TestAttributeTag));
		Value = Context.SGASComponent->GetFloatAttributeValue(EAttributeValueType::CurrentValue, TestAttributeTag,
		                                                      bWasFound);
		Res &= Test->TestFalse(TEXT("BasicManipulation: Attribute should NOT be found after remove"), bWasFound);

		return Res;
	}
};

bool FAttributesTest_BasicManipulation::RunTest(const FString& Parameters)
{
	FAttributesTestScenarios TestScenarios(this);
	return TestScenarios.TestBasicAttributeManipulation();
}
