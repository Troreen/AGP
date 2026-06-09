# Assignment 2.2 — Skeletal Animations TODO

Source material:

- **F05 — Animationer** lecture slides
- **Omniway Assignment 2.2 — Skelettanimationer** assignment text provided by the user

---

## Scope Rule

Implement only what is required by the official assignment text and what is needed from the course material to satisfy those requirements.

Do **not** add advanced animation systems just because they sound useful. If something is not in the assignment text and is not part of the course material, it should not be implemented for this assignment.

Exception: the **VG requirement** explicitly asks for **animation layers**. That should be implemented for VG because Omniway explicitly requires it, even though it is not fully explained in the F05 lecture slides. Keep the VG implementation focused on the requested behavior only.

Do **not** add unrelated systems such as:

- Inverse kinematics.
- Root motion.
- Animation retargeting.
- Morph targets / blend shapes.
- Facial animation systems.
- Complex animation state machines.
- Physics-driven animation.
- More than 128 bones.
- More than 4 bone influences per vertex.

---

## 1. Assignment Goal

Assignment 2.2 adds support for:

- Loading skeletons from FBX files.
- Loading skeletal animations from FBX files.
- Playing animations on skeletal meshes.
- Switching between animations.
- Keeping all previous Assignment 2.1 requirements working.

The program must still satisfy all previous requirements. Passing an earlier assignment does **not** mean those features can be broken now.

---

## Implementation Reference

This section maps the assignment requirements to the current code implementation.

### Asset Layout

The provided FBX assets were moved into a cleaner asset tree:

- Character mesh: `Assets/Meshes/Characters/TGA_Bro/SK_C_TGA_Bro.fbx`
- Static prop mesh: `Assets/Meshes/Props/SM_Chest.fbx`
- Locomotion animations: `Assets/Animations/Characters/TGA_Bro/Locomotion/`
- Idle animations: `Assets/Animations/Characters/TGA_Bro/Idle/`

The startup loading paths are registered in `Source/Application/ModelViewer/MeshLibrary.cpp` inside `MeshLibrary::Initialize()`.

### Camera And Input Controls

WASD, Space/Ctrl vertical movement, and right-mouse camera look are implemented in:

- `Source/Utilities/FreeFlyCameraController.cpp`
- `Source/Utilities/FreeFlyCameraController.h`

The controller stores yaw and pitch separately, clamps pitch, and writes a yaw/pitch quaternion without roll. That is what prevents camera tumble and roll.

Animation switching is implemented in:

- `Source/Application/ModelViewer/ModelViewer.cpp`

`ModelViewer::HandleAnimationInput()` uses `InputHandler::IsKeyPressed(...)`, not held-state input, so Numpad 1/2/3 changes only once per key press.

Current bindings:

- Numpad 1: `Walk`
- Numpad 2: `Run`
- Numpad 3: one-shot `Wave` partial layer

### Startup Scene

The startup scene is built in:

- `Source/Application/ModelViewer/ModelViewer.cpp`

`ModelViewer::LoadScene()` creates:

- the camera actor,
- world axes,
- the static chest FBX,
- the skeletal TGA Bro FBX,
- at least two primitive meshes from the primitive list.

Primitive mesh creation is implemented in:

- `Source/Application/ModelViewer/PrimitiveMeshBuilder.cpp`
- `Source/Application/ModelViewer/PrimitiveMeshBuilder.h`

The skeletal character starts playing `Walk` immediately through `MeshComponent::PlayAnimation("Walk", true)`.

### Static And Skeletal FBX Loading

FBX mesh loading is implemented in:

- `Source/Application/ModelViewer/MeshLibrary.cpp`
- `Source/Application/ModelViewer/MeshLibrary.h`

Important functions:

- `MeshLibrary::LoadFBXMesh(...)`
- `ConvertVertex(...)`
- `ConvertSkeleton(...)`
- `AppendElement(...)`

Static and skeletal meshes both use `TGA::FBX::Importer::LoadMeshW(...)`.

The importer already converts the FBX scene to the FBX SDK DirectX axis system. The project should not apply a fake vertical mirror in `ConvertVertex(...)`; imported positions are copied as provided by the importer.

### Vertex Bone Data

CPU vertex skinning data is stored in:

- `Source/Graphics/GraphicsEngine/Objects/Vertex.h`
- `Source/Graphics/GraphicsEngine/Objects/Vertex.cpp`

`Vertex` now contains:

- `BoneIDs`
- `SkinWeights`

The DirectX input layout uses:

- `BONEIDS` with `DXGI_FORMAT_R32G32B32A32_UINT`
- `SKINWEIGHTS` with `DXGI_FORMAT_R32G32B32A32_FLOAT`

Bone IDs and weights are copied from `TGA::FBX::Vertex` in `ConvertVertex(...)` in `MeshLibrary.cpp`. Weights are normalized when the imported total is greater than zero.

### Skeleton Data

The engine skeleton structure is implemented in:

- `Source/Graphics/GraphicsEngine/Objects/Mesh.h`
- `Source/Graphics/GraphicsEngine/Objects/Mesh.cpp`

`Skeleton` stores:

- a vector of joints,
- parent index,
- child indices,
- joint name,
- inverse bind pose,
- name-to-index lookup map.

FBX skeleton conversion is done in `ConvertSkeleton(...)` in:

- `Source/Application/ModelViewer/MeshLibrary.cpp`

The bind pose inverse matrix is transposed during conversion, as required by the assignment notes:

```cpp
joint.BindPoseInverse = ConvertMatrix(sourceBone.BindPoseInverse).GetTranspose();
```

### Animation Data

The engine animation structure is implemented in:

- `Source/Graphics/GraphicsEngine/Objects/Mesh.h`
- `Source/Graphics/GraphicsEngine/Objects/Mesh.cpp`

`Animation` stores:

- frames,
- duration,
- frames per second,
- local joint transforms by joint name.

FBX animation loading is implemented in:

- `Source/Application/ModelViewer/MeshLibrary.cpp`

Important functions:

- `MeshLibrary::LoadFBXAnimation(...)`
- `ConvertAnimation(...)`

Animations are loaded with `TGA::FBX::Importer::LoadAnimationW(...)`.

`ConvertAnimation(...)` uses `sourceFrame.LocalTransforms`, not `GlobalTransforms`.

Loaded animation names:

- `Walk`
- `Run`
- `Wave`
- `Breathing`

### Per-Actor Animation Playback

Animation playback state is implemented per mesh component in:

- `Source/GameFramework/MeshComponent.h`
- `Source/GameFramework/MeshComponent.cpp`

`MeshComponent` stores:

- base animation state,
- partial animation state,
- current frame,
- timer,
- loop flag,
- active flag,
- per-component joint matrix cache.

This keeps animation state per actor/component rather than global.

Frame timing is advanced in:

- `MeshComponent::AdvancePlayback(...)`

It uses `1.0f / FramesPerSecond`, accumulates delta time, and subtracts frame time instead of resetting the timer to zero.

### Hierarchical Pose Update

Hierarchical skeleton update is implemented in:

- `Source/GameFramework/MeshComponent.cpp`

Important functions:

- `MeshComponent::RebuildJointTransforms()`
- `MeshComponent::UpdateJointPose(...)`
- `MeshComponent::GetLocalTransformForJoint(...)`

The hierarchy starts at joint index `0`.

The code uses the engine's row-vector convention:

```cpp
const CU::Matrix4f jointTransform = GetLocalTransformForJoint(aJointIndex) * aParentJointTransform;
myJointTransforms[aJointIndex] = joint.BindPoseInverse * jointTransform;
```

The engine is intended to be row-vector style. CPU transform construction follows this convention in:

- `CommonUtilities/include/Transform.hpp`

The HLSL side should match this convention by using `row_major` matrices and vector-first `mul(...)` calls in:

- `Source/Graphics/GraphicsEngine/Shaders/Common.hlsli`
- `Source/Graphics/GraphicsEngine/Shaders/VertexShader.hlsl`

### Animation Constant Buffer

The animation constant buffer is implemented in:

- `Source/Graphics/GraphicsEngine/ConstantBuffers/AnimationBuffer.h`
- `Source/Graphics/GraphicsEngine/GraphicsEngine.h`
- `Source/Graphics/GraphicsEngine/GraphicsEngine.cpp`
- `Source/Graphics/GraphicsEngine/Shaders/Common.hlsli`

C++ side:

```cpp
struct AnimationBuffer
{
    std::array<CU::Matrix4f, 128> JointTransforms;
};
```

HLSL side:

```hlsl
cbuffer AnimationBuffer : register(b2)
{
    row_major float4x4 AB_JointTransforms[128];
}
```

The buffer is uploaded in `GraphicsEngine::RenderMesh(...)` only when the `MeshComponent` reports skinning is active.

### Static Mesh Compatibility

Static mesh compatibility is handled with a `HasSkinning` flag in:

- `Source/Graphics/GraphicsEngine/ConstantBuffers/ObjectBuffer.h`
- `Source/Graphics/GraphicsEngine/Shaders/Common.hlsli`
- `Source/Graphics/GraphicsEngine/Shaders/VertexShader.hlsl`
- `Source/Graphics/GraphicsEngine/GraphicsEngine.cpp`

C++ uses an unsigned integer for constant-buffer safety:

```cpp
unsigned HasSkinning = 0;
CU::Vector3f __padding = CU::Vector3f::Zero;
```

The shader only applies skinning when `OB_HasSkinning` is true.

### GPU Skinning

GPU skinning is implemented in:

- `Source/Graphics/GraphicsEngine/Shaders/VertexShader.hlsl`

The shader builds a weighted skin matrix from up to four joint influences:

```hlsl
float4x4 skinMatrix = 0;
skinMatrix += AB_JointTransforms[aVertex.BoneIDs.x] * aVertex.SkinWeights.x;
skinMatrix += AB_JointTransforms[aVertex.BoneIDs.y] * aVertex.SkinWeights.y;
skinMatrix += AB_JointTransforms[aVertex.BoneIDs.z] * aVertex.SkinWeights.z;
skinMatrix += AB_JointTransforms[aVertex.BoneIDs.w] * aVertex.SkinWeights.w;
```

Because the engine is row-vector style, the skinned position should be calculated as:

```hlsl
localPosition = mul(aVertex.Position, skinMatrix);
```

World, view, and projection should also use vector-first multiplication.

### Animation Switching And Looping

Animation switching is controlled in:

- `Source/Application/ModelViewer/ModelViewer.cpp`

Playback behavior is implemented in:

- `Source/GameFramework/MeshComponent.cpp`

`Walk` and `Run` are started with looping enabled.

`Wave` is started as a non-looping partial animation layer. When the wave reaches its final frame, `AdvancePlayback(...)` deactivates the partial layer, so the base animation continues.

### VG Animation Layers

Minimal animation layers are implemented in:

- `Source/GameFramework/MeshComponent.h`
- `Source/GameFramework/MeshComponent.cpp`
- `Source/Application/ModelViewer/ModelViewer.cpp`

The system has:

- one full-body base layer,
- one partial override layer,
- a boolean joint mask,
- separate playback state for base and partial layers.

Important functions:

- `MeshComponent::PlayAnimation(...)`
- `MeshComponent::PlayPartialAnimation(...)`
- `MeshComponent::ConfigurePartialLayerFromJointNames(...)`
- `MeshComponent::MarkJointAndChildren(...)`
- `MeshComponent::GetLocalTransformForJoint(...)`

The partial layer mask is configured in `ModelViewer::LoadScene()` with likely right-arm or upper-body joint-name candidates such as:

- `clavicle_r`
- `upperarm_r`
- `lowerarm_r`
- `hand_r`
- `RightShoulder`
- `RightArm`
- `RightForeArm`
- `RightHand`
- `spine_03`

When the partial layer is active, masked joints sample from `Wave`; all other joints continue sampling the base animation.

### Build And Environment Note

The Codex shell currently exposes both `PATH` and `Path`, which can make MSBuild fail before compilation. The build workaround used during implementation is:

```bat
cmd /S /C "set PATH=& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" AGP.sln /p:Configuration=Debug /p:Platform=x64 /m"
```

This removes only the duplicate all-caps `PATH` variable for that command process while preserving the canonical `Path`.

---

## 2. Official Pass Requirements

### Controls

The program must support all of these controls:

- [ ] **WASD** moves the camera forward, left, backward, and right, based on the camera rotation.
- [ ] **Space / Ctrl** moves the camera up and down based on world directions.
- [ ] **Mouse movement** rotates the camera around the world when the **right mouse button** is held down.
- [ ] The camera must **not tumble**.
- [ ] The camera must never have roll rotation.
- [ ] **Numpad 1, 2, and 3** switch the animation for the example model.
- [ ] Only one animation change may happen per key press.
- [ ] Holding down an animation key must **not** restart the animation every frame.

Implementation hint for animation switching:

```cpp
if (input.WasPressedThisFrame(Key::Numpad1))
{
    PlayAnimation("Walk");
}
```

Avoid this if it triggers every frame while the key is held:

```cpp
if (input.IsHeld(Key::Numpad1))
{
    PlayAnimation("Walk"); // Bad if this restarts every frame.
}
```

### Features

The program must support:

- [ ] Loading static models from FBX files.
- [ ] Loading skeletal models from FBX files.
- [ ] Loading animations from FBX files.
- [ ] Playing animations.

### Startup State

When the program starts:

- [ ] At least **two 3D primitive shapes** must be visible.
  - Examples: cube, sphere, pyramid, torus, etc.
- [ ] The camera must be controllable according to the control requirements above.
- [ ] One skeletal mesh must be loaded.
- [ ] The skeletal mesh must already be playing an animation.
- [ ] The skeletal mesh must be rendered correctly.
- [ ] Static meshes/primitives must still render correctly after animation support is added.

---

## 3. Official VG Requirement

The VG requirement applies **only to Assignment 2.2** and will not be built out further in later course moments.

To achieve **Väl Godkänt (VG)**, you must also implement:

- [ ] **Animation layers.**

This means:

- [ ] A model can play more than one animation at the same time.
- [ ] Different animations can affect different bones.
- [ ] Example target behavior: the character can **walk and wave at the same time**.
- [ ] In that example, one arm plays the wave animation while the rest of the body plays the walking animation.

### VG Scope Clarification

For VG, implement the smallest system that proves animation layers work.

A reasonable VG implementation is:

- [ ] One base/full-body animation layer, such as Walk or Run.
- [ ] One partial-body override layer, such as Wave.
- [ ] A way to choose which bones are affected by the partial layer.
- [ ] Final joint matrices are generated from the mixed/layered pose.
- [ ] The example model can visibly walk while waving.

Do **not** turn this into a large animation graph system.

---

## 4. Course Theory Checklist

### Skeleton

- [ ] A mesh that should be animated needs a skeleton.
- [ ] A skeleton is hierarchical.
- [ ] Bones are connected through joints.
- [ ] Each joint has its own local transform space.
- [ ] The skeleton has a bind pose, usually an A-pose or T-pose.
- [ ] The bind pose describes the original/resting transform of every joint.
- [ ] The course limit is **128 bones or fewer**.

### Skinning

- [ ] Each vertex can be affected by up to **4 bones**.
- [ ] Each vertex stores bone IDs.
- [ ] Each vertex stores skin weights.
- [ ] Skin weights describe how much each bone affects the vertex.
- [ ] The total skin weight for a vertex should add up to **1.0**.

### Animation

- [ ] An animation is a list of frames/poses.
- [ ] Each frame contains local transforms for joints.
- [ ] Animations have a duration.
- [ ] Animations have a frames-per-second value.
- [ ] Frame time is calculated from animation FPS.

```cpp
frameTime = 1.0f / animation.FramesPerSecond;
```

- [ ] Use a timer to advance frames.
- [ ] Prefer subtracting frame time instead of resetting the timer to zero.

```cpp
animTimer -= frameTime;
```

Instead of:

```cpp
animTimer = 0.0f;
```

This preserves leftover time and reduces timing drift.

---

## 5. Data Structures TODO

### Skeleton Structure

Create a skeleton structure that stores joints, not just bones.

```cpp
struct Skeleton
{
    struct Joint
    {
        Matrix BindPoseInverse;
        int Parent;
        std::vector<int> Children;
        std::string Name;
    };

    std::vector<Joint> Joints;
    std::unordered_map<std::string, size_t> JointNameToIndex;
};
```

TODO:

- [ ] Store all joints in a vector.
- [ ] Store parent index for each joint.
- [ ] Store child indices for each joint.
- [ ] Store the joint name.
- [ ] Store the inverse bind pose matrix.
- [ ] Store a name-to-index map for fast lookup.
- [ ] Keep the skeleton inside the mesh or somewhere equally suitable.

### Animation Structure

Create an animation structure that stores frames and local transforms.

```cpp
struct Animation
{
    struct Frame
    {
        std::unordered_map<std::string, Matrix> Transforms;
    };

    std::vector<Frame> Frames;
    float Duration;
    float FramesPerSecond;
};
```

TODO:

- [ ] Store all animation frames.
- [ ] Store animation duration.
- [ ] Store animation FPS.
- [ ] For each frame, store local joint transforms.
- [ ] Use joint names as keys in the frame transform map.

### Animation State Per Actor / Component

Each actor that plays an animation needs its own playback state.

TODO:

- [ ] Store the current animation.
- [ ] Store the current frame index.
- [ ] Store an animation timer.
- [ ] Store whether the current animation loops.
- [ ] Store a matrix cache for joint transforms.

Example:

```cpp
Matrix myJointTransforms[128];
```

This must be per object/actor/component, not global, because different actors may play the same animation at different times.

---

## 6. FBX Import TODO

### Skeletal Mesh Import

Skeletons are imported automatically when calling `LoadMesh`, if the FBX file contains a skeleton.

TODO:

- [ ] Use `LoadMesh` for the skeletal mesh FBX file.
- [ ] Read the `TGA::FBX::Skeleton` from the imported mesh.
- [ ] Convert the FBX skeleton into your own `Skeleton` structure.
- [ ] Copy over parent index.
- [ ] Copy over children indices.
- [ ] Copy over joint/bone name.
- [ ] Copy over bind pose inverse.
- [ ] Copy over or rebuild the name-to-index map.
- [ ] Remember that FBX calls them `Bones`, but your own structure can call them `Joints`.

### Critical Matrix Note

The lecture says the bind pose inverse from `TGA::FBX::Skeleton` is transposed.

TODO:

- [ ] Transpose `BindPoseInverse` when reading it in.

Example:

```cpp
meshJoint.BindPoseInverse = meshJoint.BindPoseInverse.GetTransposed();
```

If this is missed, animation matrices will likely be incorrect.

### Animation Import

Animations are loaded with `LoadAnimation`.

TODO:

- [ ] Load animation FBX files using `LoadAnimation`.
- [ ] Convert `TGA::FBX::Animation` into your own `Animation` structure.
- [ ] Store `Length` / `Duration`.
- [ ] Store `FramesPerSecond`.
- [ ] Store `Frames`.
- [ ] In each frame, use `LocalTransforms`.
- [ ] Do **not** use `GlobalTransforms`; the lecture says they exist for older-code compatibility.

---

## 7. Vertex Data TODO

Update the CPU-side vertex structure.

```cpp
struct Vertex
{
    Vector4f Position;
    Vector4f Color;
    Vector4u BoneIDs;
    Vector4f SkinWeights;
};
```

TODO:

- [ ] Add `BoneIDs` to the CPU vertex.
- [ ] Add `SkinWeights` to the CPU vertex.
- [ ] Read bone IDs from the FBX vertex data.
- [ ] Read skin weights from the FBX vertex data.
- [ ] Make sure `SkinWeights[i]` corresponds to `BoneIDs[i]`.

### Input Layout

TODO:

- [ ] Update the DirectX input layout.
- [ ] Use the correct format for bone IDs.

```cpp
DXGI_FORMAT_R32G32B32A32_UINT
```

- [ ] Use matching semantics between C++ and HLSL.
- [ ] Example semantics: `BONEIDS` and `SKINWEIGHTS`.

---

## 8. HLSL Vertex Input TODO

Update the GPU-side vertex structure.

TODO:

- [ ] Add `uint4 BoneIDs`.
- [ ] Add `float4 SkinWeights`.
- [ ] Use the same semantics as the C++ input layout.

Example:

```hlsl
struct Vertex
{
    float4 Position : POSITION;
    float4 Color : COLOR;
    uint4 BoneIDs : BONEIDS;
    float4 SkinWeights : SKINWEIGHTS;
};
```

---

## 9. Animation Playback TODO

### Timer Update

TODO:

- [ ] Calculate frame time from animation FPS.
- [ ] Increase the timer using delta time.
- [ ] When the timer reaches frame time:
  - [ ] Advance to the next frame.
  - [ ] Subtract `frameTime` from the timer.
  - [ ] Recalculate the animation matrix cache.

Example:

```cpp
animTimer += aDeltaTime;

if (animTimer >= frameTime)
{
    currentFrame++;
    animTimer -= frameTime;
    UpdateAnimation(...);
}
```

### Looping Rules

The lecture notes state:

- [ ] Walk should loop.
- [ ] Run should loop.
- [ ] Wave should **not** loop.
- [ ] Non-looping animations should stop or return to the previous animation when finished.

A simple animation stack can be used if helpful:

```cpp
std::stack<std::string, std::vector<std::string>> myAnimStack;
std::unordered_map<std::string, std::shared_ptr<Animation>> myAnimations;
```

Example behavior:

- Walk is playing.
- Wave is triggered.
- Wave plays once.
- When Wave finishes, return to Walk.

---

## 10. Hierarchical Animation Update TODO

The skeleton must be updated hierarchically from the root joint.

TODO:

- [ ] Start at root joint index `0`.
- [ ] For the current joint:
  - [ ] Get the joint name.
  - [ ] Get the current animation frame.
  - [ ] Get the local transform for this joint from the frame.
  - [ ] Apply the joint transform to the parent transform.
  - [ ] Apply the inverse bind pose.
  - [ ] Store the final matrix in the output transform cache.
  - [ ] Recursively update each child joint.

Suggested function shape:

```cpp
void UpdateAnimation(
    size_t aCurrentFrame,
    unsigned aJointIdx,
    const Matrix& aParentJointTransform,
    Matrix* outTransforms
)
{
    // 1. Get name of current joint.
    // 2. Get current frame from animation.
    // 3. Get joint transform of current joint from current frame.
    // 4. Apply joint transform to parent transform.
    // 5. Apply BindPoseInverse to the result.
    // 6. Store result in outTransforms.
    // 7. Recursively update children.
}
```

Important:

```cpp
A * B != B * A
```

Matrix multiplication order matters. If the model deforms incorrectly, check matrix order and transposes first.

---

## 11. Animation Buffer TODO

Create a constant buffer for animation matrices.

### C++ Side

```cpp
struct AnimationBuffer
{
    Matrix JointTransforms[128];
};
```

### HLSL Side

```hlsl
cbuffer AnimationBuffer : register(b2)
{
    float4x4 AB_JointTransforms[128];
};
```

TODO:

- [ ] Create the animation constant buffer.
- [ ] Upload the actor/component joint matrix cache to the buffer.
- [ ] Bind the buffer before rendering animated meshes.
- [ ] Only set/use this buffer for animated models.

---

## 12. Vertex Shader Skinning TODO

In the vertex shader, create a skin matrix from the four bone influences.

TODO:

- [ ] Initialize `skinMatrix` to zero.
- [ ] Add each joint transform multiplied by its matching skin weight.
- [ ] Use `*` for scalar/component-wise weight multiplication.
- [ ] Use `mul` for matrix/vector multiplication.
- [ ] Apply skinning before world/view/projection.

Example logic:

```hlsl
float4x4 skinMatrix = 0;

skinMatrix += AB_JointTransforms[vertex.BoneIDs.x] * vertex.SkinWeights.x;
skinMatrix += AB_JointTransforms[vertex.BoneIDs.y] * vertex.SkinWeights.y;
skinMatrix += AB_JointTransforms[vertex.BoneIDs.z] * vertex.SkinWeights.z;
skinMatrix += AB_JointTransforms[vertex.BoneIDs.w] * vertex.SkinWeights.w;

float4 skinnedPosition = mul(vertex.Position, skinMatrix);
```

Then continue with world, view, and projection as usual.

---

## 13. Static Mesh Compatibility TODO

After adding animation code, ordinary non-animated meshes can break if the vertex shader always applies skinning.

TODO:

- [ ] Add a `HasSkinning` flag to the object buffer.
- [ ] Only apply skinning if the object has a skeleton/skinning data.
- [ ] Render static meshes normally.

Example HLSL idea:

```hlsl
if (OB_HasSkinning)
{
    // Apply skinning.
}

// Then apply world, view, and projection.
```

---

## 14. Constant Buffer Packing TODO

The GPU has different packing rules than the CPU.

### Bool Issue

Do not use a C++ `bool` directly in a constant buffer if HLSL expects a bool.

CPU `bool` is usually 1 byte, while GPU bool is 4 bytes.

Use this on the C++ side instead:

```cpp
unsigned HasSkinning;
```

The HLSL side can still use:

```hlsl
bool OB_HasSkinning;
```

### Padding Issue

Constant buffers need 16-byte alignment.

TODO:

- [ ] Add padding after `HasSkinning`.
- [ ] Make sure the C++ and HLSL buffer sizes match.

C++:

```cpp
struct ObjectBuffer
{
    Matrix World;
    unsigned HasSkinning;
    FVector3 __padding;
};
```

HLSL:

```hlsl
cbuffer ObjectBuffer : register(b1)
{
    float4x4 OB_World;
    bool OB_HasSkinning;
    float3 __ob_Padding;
};
```

Expected size:

```text
sizeof(ObjectBuffer) = 80
```

---

## 15. VG Animation Layers TODO

Only do this section after the pass requirements work.

The official VG requirement is animation layers: one model should be able to play more than one animation at the same time on different bones.

### Minimal Target

- [ ] Base layer: full-body animation, for example Walk.
- [ ] Upper-body or arm layer: partial animation, for example Wave.
- [ ] The character can walk while waving.
- [ ] The wave affects only the selected arm/upper-body bones.
- [ ] The rest of the skeleton continues using the base animation.

### Bone Mask / Layer Mask

You need a way to decide which joints belong to the partial layer.

TODO:

- [ ] Choose the root joint for the waving arm or upper body.
- [ ] Mark that joint and its children as affected by the Wave layer.
- [ ] Store this as a simple mask, for example:

```cpp
std::vector<bool> WaveLayerMask;
```

or:

```cpp
std::array<bool, 128> WaveLayerMask;
```

- [ ] The mask should answer: “Should this joint use the Wave animation instead of the base animation?”

### Layered Pose Calculation

A simple VG approach:

- [ ] Sample the base animation frame.
- [ ] Sample the partial animation frame.
- [ ] For each joint:
  - [ ] If the joint is in the partial layer mask, use the partial animation transform.
  - [ ] Otherwise, use the base animation transform.
- [ ] Run the hierarchical update using the chosen local transform for each joint.
- [ ] Upload the final matrices to the animation buffer as usual.

Pseudo-code idea:

```cpp
Matrix GetLayeredLocalTransform(
    const Skeleton& skeleton,
    const Animation& baseAnim,
    size_t baseFrame,
    const Animation& upperAnim,
    size_t upperFrame,
    size_t jointIndex,
    const std::array<bool, 128>& upperBodyMask)
{
    const std::string& jointName = skeleton.Joints[jointIndex].Name;

    if (upperBodyMask[jointIndex])
    {
        return upperAnim.Frames[upperFrame].Transforms.at(jointName);
    }

    return baseAnim.Frames[baseFrame].Transforms.at(jointName);
}
```

### VG Timing

The base and partial animation can have separate playback states.

TODO:

- [ ] Store a timer and frame index for the base animation.
- [ ] Store a timer and frame index for the partial animation.
- [ ] Let Walk/Run loop.
- [ ] Let Wave finish normally if it is non-looping.
- [ ] When Wave is not active, use only the base animation.

### VG Boundaries

For VG, you do **not** need to implement:

- [ ] Smooth cross-fading between layers, unless the teacher explicitly asks for it.
- [ ] Weight blending between layers, unless the teacher explicitly asks for it.
- [ ] A full animation graph editor.
- [ ] IK.
- [ ] Root motion.
- [ ] Retargeting.

The official example is enough: walk + wave, with wave affecting only part of the skeleton.

---

## 16. Suggested Implementation Order

### Phase 1 — Keep Previous Requirements Working

- [ ] Confirm static FBX model loading still works.
- [ ] Confirm at least two 3D primitives are visible.
- [ ] Confirm camera movement works with WASD.
- [ ] Confirm Space/Ctrl vertical movement works.
- [ ] Confirm mouse look with right mouse button works.
- [ ] Confirm camera has no roll/tumble.

### Phase 2 — Data and Import

- [ ] Update vertex structure with `BoneIDs` and `SkinWeights`.
- [ ] Update input layout.
- [ ] Create `Skeleton` structure.
- [ ] Import skeleton from skeletal mesh FBX.
- [ ] Transpose `BindPoseInverse` during import.
- [ ] Create `Animation` structure.
- [ ] Import animations from FBX using local transforms.

### Phase 3 — CPU Animation Playback

- [ ] Add animation state to actor/component.
- [ ] Add per-actor joint transform cache.
- [ ] Implement frame timer.
- [ ] Implement loop/non-loop handling.
- [ ] Implement hierarchical `UpdateAnimation`.
- [ ] Verify matrix cache updates correctly.

### Phase 4 — GPU Skinning

- [ ] Create `AnimationBuffer`.
- [ ] Upload joint matrices to GPU.
- [ ] Update HLSL vertex input.
- [ ] Implement skin matrix in vertex shader.
- [ ] Add `HasSkinning` to object buffer.
- [ ] Fix constant buffer padding/alignment.
- [ ] Make static meshes render normally.

### Phase 5 — Assignment Behavior

- [ ] Load at least one animated skeletal mesh on startup.
- [ ] Start playing an animation automatically.
- [ ] Add Numpad 1/2/3 animation switching.
- [ ] Make key presses edge-triggered, not held-triggered.
- [ ] Set Walk and Run to loop.
- [ ] Set Wave to not loop.
- [ ] Make Wave return to a previous/default animation when finished, if using an animation stack.

### Phase 6 — VG Only After Pass Works

- [ ] Implement animation layers.
- [ ] Add a bone mask for the partial animation layer.
- [ ] Make Wave affect only the selected arm/upper-body bones.
- [ ] Keep Walk/Run affecting the rest of the body.
- [ ] Show the model walking and waving at the same time.
- [ ] Re-test all pass requirements after VG changes.

---

## 17. Final Testing Checklist

### Required for Pass

- [ ] Program starts without crashing.
- [ ] At least two 3D primitive shapes are visible.
- [ ] Static FBX models can be loaded.
- [ ] WASD moves the camera based on camera rotation.
- [ ] Space/Ctrl moves the camera up/down in world directions.
- [ ] Mouse rotates the camera while right mouse button is held.
- [ ] Camera has no roll/tumble.
- [ ] A skeletal mesh is visible.
- [ ] The skeletal mesh plays an animation immediately.
- [ ] Numpad 1 changes animation once per press.
- [ ] Numpad 2 changes animation once per press.
- [ ] Numpad 3 changes animation once per press.
- [ ] Holding a Numpad key does not restart the animation every frame.
- [ ] Walk loops.
- [ ] Run loops.
- [ ] Wave does not loop.
- [ ] Static meshes still render correctly.
- [ ] No DirectX constant-buffer size/alignment errors.
- [ ] No obvious mesh exploding/stretching from wrong matrix order.
- [ ] Bind pose inverse was transposed during import.
- [ ] `LocalTransforms` are used for animation frames.
- [ ] `GlobalTransforms` are not used.
- [ ] Bone IDs use `R32G32B32A32_UINT`.
- [ ] CPU and GPU semantics match.
- [ ] Animation buffer is only required for animated models.

### Required for VG

- [ ] A model can play more than one animation at the same time.
- [ ] Different animations can affect different bones.
- [ ] Walk + Wave can run at the same time.
- [ ] Wave affects only the selected arm/upper-body bones.
- [ ] The rest of the body continues walking.
- [ ] The implementation does not include unrelated systems that were not requested.

---

## 18. Common Mistakes to Avoid

- Forgetting to preserve Assignment 2.1 requirements.
- Forgetting to show at least two primitive shapes at startup.
- Forgetting to keep static FBX loading working.
- Forgetting to transpose `BindPoseInverse`.
- Using `GlobalTransforms` instead of `LocalTransforms`.
- Applying matrix multiplication in the wrong order.
- Forgetting that `A * B != B * A`.
- Restarting animation every frame while a key is held.
- Applying skinning to static meshes.
- Using a C++ `bool` directly in a constant buffer without handling size differences.
- Forgetting 16-byte constant buffer alignment.
- Forgetting to update both CPU and GPU vertex definitions.
- Using the wrong input layout format for bone IDs.
- Trying to implement advanced features that are not required.
- For VG, accidentally applying Wave to the whole body instead of only the masked bones.
