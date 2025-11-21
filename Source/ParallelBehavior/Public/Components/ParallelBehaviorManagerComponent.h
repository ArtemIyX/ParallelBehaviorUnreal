// © Artem Podorozhko. All Rights Reserved. This project, including all associated assets, code, and content, is the property of Artem Podorozhko. Unauthorized use, distribution, or modification is strictly prohibited.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ParallelBehaviorManagerComponent.generated.h"

/**
 * @struct FParallelBehaviorSetup
 * @brief Configuration for a single parallel behavior tree instance
 */
USTRUCT(BlueprintType)
struct FParallelBehaviorSetup
{
	GENERATED_BODY()

	/** Unique identifier for this parallel tree (used for debugging and removal) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Id = NAME_None;

	/** Behavior tree asset to run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UBehaviorTree> BTAsset;
};

/**
 * @struct FParallelBehaviorRuntime
 * @brief Runtime data for a running parallel behavior tree
 */
USTRUCT()
struct PARALLELBEHAVIOR_API FParallelBehaviorRuntime
{
	GENERATED_BODY()

public:
	FParallelBehaviorRuntime() {}

	FParallelBehaviorRuntime(const FName& InId,
		UBehaviorTreeComponent* InBTComponent,
		UBlackboardComponent* InBlackboardComponent)
		: Id(InId)
		, TreeComponent(InBTComponent)
		, BlackboardComponent(InBlackboardComponent) {}

	FParallelBehaviorRuntime(const FName& InId,
		const TWeakObjectPtr<UBehaviorTreeComponent>& InBTComponent,
		const TWeakObjectPtr<UBlackboardComponent>& InBlackboardComponent)
		: Id(InId)
		, TreeComponent(InBTComponent)
		, BlackboardComponent(InBlackboardComponent) {}

public:
	UPROPERTY()
	FName Id;

	UPROPERTY()
	TWeakObjectPtr<UBehaviorTreeComponent> TreeComponent;

	UPROPERTY()
	TWeakObjectPtr<UBlackboardComponent> BlackboardComponent;
};

/**
 * @class UParallelBehaviorManagerComponent
 * @brief Manages multiple independent Behavior Trees running in parallel on the same AI pawn.
 *
 * Useful for layered AI (e.g. combat + locomotion + emotion + dialogue all running simultaneously).
 * Each tree has its own Blackboard instance to avoid key conflicts.
 */	
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARALLELBEHAVIOR_API UParallelBehaviorManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UParallelBehaviorManagerComponent();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior", DisplayName="Behaviors")
	TArray<FParallelBehaviorSetup> ParallelBehaviorDefaults;

protected:
	/** All currently active parallel trees */
	UPROPERTY()
	TArray<FParallelBehaviorRuntime> RunningTrees;

protected:
	/** Automatically start default trees */
	UFUNCTION()
	void RunDefaultTrees();


protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	/** Get currently running runtime instances (editor/debug only) */
	const TArray<FParallelBehaviorRuntime>& GetRunningTrees() const { return RunningTrees; }

	/** Add and start a new parallel behavior tree at runtime */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	bool AddTree(const FParallelBehaviorSetup& InSetup);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	UBehaviorTreeComponent* GetTree(const FName& InId) const;

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	void StopTree(const FName& InId);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	void RestartTree(const FName& InId);

	/** Remove a running tree by ID */
	UFUNCTION(BlueprintCallable, Category = "Manage")
	bool RemoveTree(FName Id);

	/** Remove all running trees */
	UFUNCTION(BlueprintCallable, Category = "Manager")
	void RemoveAllTrees();

	/**
	 * @brief Get the Pawn this manager is controlling.
	 *
	 * Override this in Blueprints or derived C++ classes if your manager is not attached
	 * to a Pawn directly (e.g. attached to an AIController that owns a Pawn).
	 *
	 * Default implementation returns OwnerController->GetPawn()
	 *
	 * @return The controlled Pawn, or nullptr if none
	 */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "Override")
	APawn* GetPawn() const;
};