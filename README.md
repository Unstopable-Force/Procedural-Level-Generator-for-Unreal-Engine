# Procedural Level Generator for Unreal Engine

A robust, seed-based procedural level and dungeon generator written in C++ for Unreal Engine. It structures environments by interconnecting modular room actors using socket-based alignment, featuring intelligent collision validation with a deferred backtracking loop, and dead-end optimization via Instanced Static Meshes (ISM).

This system is completely data-driven and configurable via the Unreal Editor Details Panel, making it perfect for roguelikes, dungeon crawlers, or stylized modular projects.

================================================================================
DETAILED GENERATION WORKFLOW
================================================================================

1. Initialization: On BeginPlay, if bGenerate is true, the system initializes an
   FRandomStream using the provided GenerationSeed (or generates a random one 
   if 0). It then spawns the designated StartRoom at the generator's transform.

2. Exit Pool Management: The generator recursively gathers all child scene 
   components from a component named "Exits" inside the newly spawned room 
   and appends them to a runtime master list (ExitsList).

3. Room Spawning: The system selects a random exit point from the pool, picks 
   a random room class from the currently active sublist, and spawns it aligned 
   perfectly with the exit's location and rotation.

4. Deferred Overlap Validation: To prevent physics race conditions, the 
   generator waits for 0.2 seconds using a non-blocking FTimerHandle before 
   executing CheckOverlappings().

5. Backtracking & Retries:
   - If an overlap is detected: The newly spawned room is instantly destroyed. 
     The exit point remains in the pool, and the system immediately tries 
     again, potentially selecting a different room class or a different exit.
   - If valid: The room is accepted. RoomAmount is decremented, the used exit 
     is moved to DoorList, and the new room's exits are added to the pool.

6. Dynamic Pacing: As RoomAmount counts down, the active room pool changes 
   dynamically:
   - Normal state: BaseRoomList is used.
   - Interval state: If RoomAmount is a multiple of 10 (RoomAmount % 10 == 0), 
     it switches to SpecialRoomList.
   - Final state: When RoomAmount == 1, it locks into EndRoomList to cap off 
     the level layout.

7. Finalization: Once RoomAmount <= 0, it triggers CloseExits() to fill all 
   remaining open sockets with a single UInstancedStaticMeshComponent (Hedges) 
   and spawns interactive GateClass actors at all successful connection points.

================================================================================
CRITICAL BLUEPRINT REQUIREMENTS
================================================================================

For your Room Blueprints to work seamlessly with this C++ architecture, they 
must strictly adhere to the following component naming and structural 
hierarchy rules:

1. The "Exits" Component
   - Type: USceneComponent (or any derived class).
   - Name: Must be named exactly "Exits".
   - Structure: Can contain any number of child scene components representing 
     socket attachment points. Reflection is recursive, meaning your exit points 
     can be safely nested inside child hierarchies or child actors within the room.

2. The "GeneratorCollision" Component
   - Type: USceneComponent (or any derived class).
   - Name: Must be named exactly "GeneratorCollision".
   - Structure: Must contain one or multiple UBoxComponent shapes outlining the 
     room's physical footprint.
   - [CRITICAL RULE - Direct Attachment]: The UBoxComponent elements must be 
     immediate direct children of the GeneratorCollision component. The C++ 
     code queries children non-recursively (bIncludeAllDescendants = false).
   - [CRITICAL RULE - Attachment Naming]: The overlap filtering algorithm 
     explicitly verifies that an external colliding component's AttachParent 
     is named exactly "GeneratorCollision". If your box components are attached 
     to a root component or any intermediate component with a different name, 
     the collision system will fail to register the overlap, causing rooms 
     to clip into each other.

================================================================================
CONFIGURATION PROPERTIES
================================================================================

Exposed parameters in the Unreal Details Panel:

* bGenerate (bool) [Category: Generation]
  Toggles whether the procedural execution starts on BeginPlay.

* RoomAmount (int32) [Category: Generation]
  The total size of the layout (decrements dynamically during generation).

* GenerationSeed (int32) [Category: Generation]
  Integer seed for deterministic layouts. Set to 0 for a completely random run.

* bSpawnGates (bool) [Category: Generation]
  Toggles whether gate actors are placed between connected rooms.

* GateClass (TSubclassOf<AActor>) [Category: Generation]
  The blueprint class spawned at connected exit transforms (e.g., doors, arches).

* Hedges (UInstancedStaticMeshComponent) [Category: Generation]
  Component handling the efficient, single-draw-call rendering of blocked 
  dead ends.

* StartRoom (TSubclassOf<AActor>) [Category: Rooms]
  The entry point room blueprint class.

* BaseRoomList (TArray<TSubclassOf<AActor>>) [Category: Rooms]
  List of standard modular rooms.

* SpecialRoomList (TArray<TSubclassOf<AActor>>) [Category: Rooms]
  Reward/Challenge rooms spawned at intervals (multiples of 10).

* EndRoomList (TArray<TSubclassOf<AActor>>) [Category: Rooms]
  Exit/Boss room blueprints selected for the final layout slot.

================================================================================
TECH STACK
================================================================================
* Engine Compatibility: Unreal Engine 5.x (Utilizes modernized TObjectPtr engine pointers).
* Language: C++ (Standard Unreal Engine API).

================================================================================
LICENSE
================================================================================
Copyright (c) 2026 Nikita Yesman. All rights reserved.
