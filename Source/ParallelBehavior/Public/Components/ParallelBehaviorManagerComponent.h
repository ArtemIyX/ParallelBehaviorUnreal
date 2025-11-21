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
	/** Default behaviors */
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
	/**
	 * Returns a const reference to the array of all currently running parallel behavior trees.
	 * 
	 * This is a lightweight accessor meant for debugging, inspection, or iteration over active trees.
	 * The returned array contains FParallelBehaviorRuntime structs with runtime data (ID, component, setup, etc.).
	 * 
	 * @return Const reference to the internal array of running behavior trees.
	 */
	const TArray<FParallelBehaviorRuntime>& GetRunningTrees() const { return RunningTrees; }

	/**
	 * Adds and starts a new parallel behavior tree instance based on the provided setup.
	 * 
	 * Only allowed on authority (server). The tree will be instantiated, initialized with the given
	 * setup (BehaviorTree asset, Blackboard overrides, etc.), and immediately started.
	 * A unique FName ID is automatically generated unless one is specified in the setup.
	 * 
	 * @param InSetup Configuration data defining the behavior tree asset, optional custom ID, 
	 *                blackboard initialization values, and other runtime parameters.
	 * @return true if the tree was successfully created and started, false otherwise 
	 *         (e.g. invalid asset, duplicate ID, or failure to spawn component).
	 */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	bool AddTree(const FParallelBehaviorSetup& InSetup);

	/**
	 * Retrieves the Behavior Tree component associated with the specified identifier.
	 * 
	 * Only allowed on authority. Searches through all managed parallel behavior tree instances.
	 * 
	 * @param InId The unique identifier of the behavior tree instance to retrieve.
	 * @return Pointer to the UBehaviorTreeComponent if found, nullptr if no tree with that ID exists.
	 */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	UBehaviorTreeComponent* GetTree(const FName& InId) const;

	/**
	 * Stops execution of the behavior tree instance with the given ID.
	 * 
	 * Immediately aborts the running tree. All active tasks are canceled, but the component and
	 * blackboard data are preserved until explicitly removed.
	 * Safe to call on non-existent IDs (no-op).
	 * 
	 * @param InId The unique identifier of the behavior tree instance to stop.
	 */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	void StopTree(const FName& InId);

	/**
	 * Restarts the behavior tree instance with the given ID.
	 * 
	 * Equivalent to calling StopTree() followed by starting the tree again from its root node.
	 * The blackboard is typically reset (depending on setup flags), and all tasks begin fresh.
	 * Useful for resetting AI state without removing/re-adding the tree entirely.
	 * 
	 * @param InId The unique identifier of the behavior tree instance to restart.
	 */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Manage")
	void RestartTree(const FName& InId);

	/**
	 * Removes and destroys the behavior tree instance with the specified ID.
	 * 
	 * Stops the tree (if running), destroys its UBehaviorTreeComponent, and removes it from the
	 * internal tracking array. Can be called from anywhere (client/server), but typically used
	 * after authority has stopped/restarted as needed.
	 * 
	 * @param Id The unique identifier of the behavior tree instance to remove.
	 * @return true if a tree with the given ID was found and removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Manage")
	bool RemoveTree(FName Id);

	/**
	 * Removes and destroys all currently managed parallel behavior tree instances.
	 * 
	 * Stops every running tree, destroys their components, and clears the internal array.
	 * Useful for cleanup on death, level transition, or when fully resetting AI logic.
	 */
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