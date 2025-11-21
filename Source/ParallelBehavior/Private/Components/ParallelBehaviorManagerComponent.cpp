// © Artem Podorozhko. All Rights Reserved. This project, including all associated assets, code, and content, is the property of Artem Podorozhko. Unauthorized use, distribution, or modification is strictly prohibited.
#include "Components/ParallelBehaviorManagerComponent.h"
#include "ParallelBehavior.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"


UParallelBehaviorManagerComponent::UParallelBehaviorManagerComponent()
{
	// We want to tick if any tree needs it, but usually BehaviorTreeComponent handles its own ticking
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}


void UParallelBehaviorManagerComponent::RunDefaultTrees()
{
	int32 n = ParallelBehaviorDefaults.Num();
	for (int32 i = 0; i < n; ++i)
	{
		AddTree(ParallelBehaviorDefaults[i]);
	}
}


// Called when the game starts
void UParallelBehaviorManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		RunDefaultTrees();
	}
}

void UParallelBehaviorManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAllTrees(); // Ensures proper cleanup
	Super::EndPlay(EndPlayReason);
}

bool UParallelBehaviorManagerComponent::AddTree(const FParallelBehaviorSetup& InSetup)
{
	if (InSetup.BTAsset == nullptr)
	{
		UE_LOG(LogParallelBehavior, Warning, TEXT("AddTree: Unable to run NULL behavior tree"));
		return false;
	}

	UBlackboardComponent* blackboardComp = nullptr;
	if (InSetup.BTAsset->BlackboardAsset == nullptr)
	{
		UE_LOG(LogParallelBehavior, Warning, TEXT("AddTree: trying to use NULL Blackboard asset. Ignoring"));
	}
	else
	{
		const FString bbName = FString::Printf(TEXT("%s_BlackboardComponent"), *InSetup.Id.ToString());

		blackboardComp = NewObject<UBlackboardComponent>(this, *bbName);
		if (blackboardComp != nullptr)
		{
			if (blackboardComp->InitializeBlackboard(*InSetup.BTAsset->BlackboardAsset))
			{
				// find the "self" key and set it to our pawn
				const FBlackboard::FKey selfKey = InSetup.BTAsset->BlackboardAsset->GetKeyID(FBlackboard::KeySelf);
				if (selfKey != FBlackboard::InvalidKey)
				{
					blackboardComp->SetValue<UBlackboardKeyType_Object>(selfKey, GetPawn());
				}
			}
			blackboardComp->RegisterComponent();

		}
	}
	const FString btName = FString::Printf(TEXT("%s_BehaviorTreeComponent"), *InSetup.Id.ToString());
	UBehaviorTreeComponent* btComp = NewObject<UBehaviorTreeComponent>(this, *btName);
	btComp->RegisterComponent();

	check(btComp != nullptr);
	btComp->StartTree(*InSetup.BTAsset, EBTExecutionMode::Looped);

	RunningTrees.Add(
		FParallelBehaviorRuntime(InSetup.Id, btComp, blackboardComp));

	UE_LOG(LogParallelBehavior, Log, TEXT("AddTree: Started tree '%s' with blackboard '%s'"), *GetNameSafe(InSetup.BTAsset.Get()),
		*GetNameSafe(InSetup.BTAsset->BlackboardAsset));
	return true;
}

void UParallelBehaviorManagerComponent::StopTree(const FName& InId)
{
	if (UBehaviorTreeComponent* tree = GetTree(InId))
	{
		tree->StopTree();
	}
}

void UParallelBehaviorManagerComponent::RestartTree(const FName& InId)
{
	if (UBehaviorTreeComponent* tree = GetTree(InId))
	{
		tree->RestartTree();
	}
}

bool UParallelBehaviorManagerComponent::RemoveTree(FName Id)
{
	int32 foundIndex = INDEX_NONE;
	for (int32 i = RunningTrees.Num() - 1; i >= 0; --i)
	{
		const FParallelBehaviorRuntime& rt = RunningTrees[i];
		if (rt.Id == Id)
		{
			if (rt.TreeComponent.IsValid())
			{
				rt.TreeComponent->StopTree(EBTStopMode::Safe); // or Force if you prefer
				rt.TreeComponent->DestroyComponent();
			}
			if (rt.BlackboardComponent.IsValid())
			{
				rt.BlackboardComponent->DestroyComponent();
			}
			foundIndex = i;

		}
	}
	if (foundIndex != INDEX_NONE)
	{
		RunningTrees.RemoveAt(foundIndex);
		UE_LOG(LogParallelBehavior, Log, TEXT("ParallelBehavior: Removed tree ID '%s'"), *Id.ToString());
	}

	return foundIndex != INDEX_NONE;
}

void UParallelBehaviorManagerComponent::RemoveAllTrees()
{
	for (int32 i = RunningTrees.Num() - 1; i >= 0; --i)
	{
		const FParallelBehaviorRuntime& rt = RunningTrees[i];

		if (rt.TreeComponent.IsValid())
		{
			rt.TreeComponent->StopTree(EBTStopMode::Safe); // or Force if you prefer
			rt.TreeComponent->DestroyComponent();
		}
		if (rt.BlackboardComponent.IsValid())
		{
			rt.BlackboardComponent->DestroyComponent();
		}

	}
	RunningTrees.Empty();
}

APawn* UParallelBehaviorManagerComponent::GetPawn_Implementation() const
{
	if (AController* ownerController = GetOwner<AController>())
	{
		return ownerController->GetPawn();
	}
	return nullptr;
}

UBehaviorTreeComponent* UParallelBehaviorManagerComponent::GetTree(const FName& InId) const
{
	const int32 n = ParallelBehaviorDefaults.Num();
	for (int32 i = 0; i < n; ++i)
	{
		if (RunningTrees[i].Id == InId)
		{
			return RunningTrees[i].TreeComponent.Get();
		}
	}
	UE_LOG(LogParallelBehavior, Warning, TEXT("[UParallelBehaviorManagerComponent] Failed to find behavior tree with id '%s'"), *InId.ToString());
	return nullptr;
}