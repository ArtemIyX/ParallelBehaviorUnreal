# Parallel Behavior Plugin - Unreal Engine 5.6+
## Description
ParallelBehavioris a lightweight Unreal Engine plugin that allows a single AI-controlled Pawn (or AIController) to run multiple independent Behavior Trees simultaneously, each with its own isolated Blackboard.

This solves a common limitation of the default ``UBehaviorTreeComponent``: only one tree can be active at a time on an AIController.
With this component you can easily implement layered AI architecture such as:

- Combat logic (main BT)
- Locomotion / movement overrides
- Emotional state / mood system
- Dialogue / interaction layer
- Environmental awareness (e.g. weather reactions)

Each layer runs completely in parallel without key conflicts because every tree has its own ``UBlackboardComponent``.

## Features

- Run unlimited parallel Behavior Trees on the same Pawn/AIController
- Each tree has a fully independent Blackboard instance
- Blueprint-exposed API (Authority-only where required)
- Automatic startup of default trees defined in component defaults
- Unique ID system for runtime management (add/stop/restart/remove by name)
- Safe cleanup on EndPlay
- Works in multiplayer (authority checks included)

## Usage
1. Add the Component
   - Create  ParallelBehaviorManagerComponent to your AIController or directly to your Pawn (recommended on AIController).
3. Define Default Trees (optional)
   - In the component's details panel you will see an array ``Behaviors`` (default trees).
   - Add entries and assign Behavior Tree assets. These will automatically start on ``BeginPlay``.
   - Each entry can have a custom Id (FName).

## Runtime Control (Blueprints or C++)
| Function | Description |
|----------|--------------|
| Add Tree | Dynamically spawn and start a new parallel BT from a setup struct|
| Get Tree | Retrieve the ``UBehaviorTreeComponent`` for a specific ID|
| Stop Tree | DAbort execution of a specific tree (keeps component alive)|
| Restart Tree | Stop + start again|
| Remove Tree | Stop + destroy component for a specific ID|
| Remove all Trees | Full cleanup of all parallel trees (called on ``EndPlay``)|

## API Quick Reference
```cpp
// Add a new tree at runtime
FParallelBehaviorSetup Setup;
Setup.Id = FName("TemporaryAlert");
Setup.BTAsset = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/AI/BT_Alert.BT_Alert"));
ManagerComponent->AddTree(Setup);

// Get component
UBehaviorTreeComponent* AlertComp = ManagerComponent->GetTree("TemporaryAlert");

// Stop / Restart / Remove
ManagerComponent->StopTree("TemporaryAlert");
ManagerComponent->RestartTree("EmotionLayer");
ManagerComponent->RemoveTree("DialogueLayer");
```
## Overriding GetPawn()
The default implementation already returns ``GetOwner<AController>()->GetPawn()``

You normally don't need to override it. The function is marked as BlueprintNativeEvent if you ever need custom logic.

## Known Limitations
- Parallel trees do not have built-in priority system (you must implement arbitration in your trees or via events)
- Very large numbers of parallel trees (>20) may impact performance – use reasonably

## License
This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.
