# Assignment 2.1 TODO List — With Simple Mesh Library Organization

## Goal

Assignment 2.1 is about adding support for loading and rendering **static FBX models**.

Your program should still be able to render the old generated primitives, like the cube and axes, while also being able to load at least one external FBX model.

The FBX model must:

- Load successfully.
- Render correctly.
- Have several elements.

---

## 1. Add TGA FBX to the project

Add the **TGA FBX** library to your project and make sure everything compiles and links.

You need to initialize the importer when the program starts:

```cpp
TGA::FBX::Importer::InitImporter();
```

You also need to shut it down when the program closes:

```cpp
TGA::FBX::Importer::UninitImporter();
```

This can be done directly in `ModelViewer`, or inside a small helper class like `MeshLibrary`.

---

## 2. Decide where mesh loading should live

The lecture says not to build a big asset-management system yet.

Real asset management comes later in the course.  
For this assignment, keep it simple.

You have two reasonable options:

### Option A: Simple function in `ModelViewer`

This is the easiest approach.

You make a function like this directly in `ModelViewer`:

```cpp
std::shared_ptr<Mesh> LoadFBXMesh(const std::filesystem::path& aPath);
```

Then call it in `Initialize()`:

```cpp
std::shared_ptr<Mesh> mesh = LoadFBXMesh(u8"SM_Chest.fbx");
myMeshes.emplace("SM_Chest", mesh);
```

### Option B: Small `MeshLibrary` class

This is a cleaner organization option, but still simple.

The `MeshLibrary` is not a full asset manager.  
It is just a small class that stores and loads meshes in one place.

Example:

```cpp
class MeshLibrary
{
public:
    MeshLibrary();
    ~MeshLibrary();

    void Initialize();

    std::shared_ptr<Mesh> GetMesh(std::string_view aName);
    bool LoadFBXMesh(const std::filesystem::path& aPath);

private:
    void InitUnitCube();
    void InitUnitPlane();
    void InitAxes();

    std::unordered_map<std::string, std::shared_ptr<Mesh>> myMeshes;
};
```

The idea is that `MeshLibrary` handles:

- Creating default meshes, like cube, plane, and others.
- Loading FBX meshes.
- Storing meshes in a map.
- Returning already-loaded meshes by name.

Example usage:

```cpp
myMeshLibrary.Initialize();

auto chestMesh = myMeshLibrary.GetMesh("SM_Chest");
```

The map could contain something like:

```cpp
"Cube"     -> cube mesh
"Pyramid"     -> pyramid mesh
"SM_Chest" -> chest FBX mesh
```

This means you load the mesh once, then reuse it.

For Assignment 2.1, either option is fine.  
Use `MeshLibrary` if you want cleaner code, but do not spend too much time making a complex system.

---

## 3. Create FBX loading functionality

Whether you put it in `ModelViewer` or `MeshLibrary`, you need a function that loads an FBX file.

Example:

```cpp
std::shared_ptr<Mesh> LoadFBXMesh(const std::filesystem::path& aPath);
```

Inside it, use TGA FBX:

```cpp
TGA::FBX::Mesh tgaMesh;

const TGA::FBX::FbxImportStatus importStatus =
    TGA::FBX::Importer::LoadMesh(aPath, tgaMesh);

if (importStatus)
{
    // Convert tgaMesh to your own Mesh here
}
```

You should not render `TGA::FBX::Mesh` directly.  
You need to convert it into your own mesh format.

---

## 4. Convert `TGA::FBX::Mesh` to your own `Mesh`

The imported data comes as a `TGA::FBX::Mesh`.

You need to convert it into your own structures:

- Your own `Mesh`
- Your own `Vertex`
- Your own `Element`

For now, you mainly need:

- Vertex position
- Vertex color, or temporary random color
- Indices
- Elements

There is a lot of extra data in the FBX mesh that you do not need yet.

---

## 5. Handle multiple elements correctly

This is one of the most important parts of the assignment.

The assignment requires the loaded FBX model to have **several elements**.

The important difference is:

```text
TGA FBX:
- One vertex list per element
- One index list per element

Your mesh:
- One big vertex list per mesh
- One big index list per mesh
```

So when converting each FBX element into your own mesh, you must track where each element starts in your big lists.

For each element, store:

```cpp
VertexOffset
IndexOffset
NumVertices
NumIndices
```

Meaning:

- `VertexOffset` = where this element starts in your big vertex list.
- `IndexOffset` = where this element starts in your big index list.
- `NumVertices` = how many vertices this element has.
- `NumIndices` = how many indices this element has.

If these values are wrong, the model may render incorrectly, partially render, or use the wrong vertices/indices.

---

## 6. Add temporary random vertex colors

The example chest model does not have vertex colors.

Since textures have not been introduced yet, the model may render black.

To avoid that, give each vertex a random color while converting from `TGA::FBX::Mesh` to your own vertices.

Example:

```cpp
float r = Helpers::RandomFloatInRange(0.0f, 1.0f);
float g = Helpers::RandomFloatInRange(0.0f, 1.0f);
float b = Helpers::RandomFloatInRange(0.0f, 1.0f);

vertex.Color = { r, g, b, 1.0f };
```

This is only temporary until proper materials and textures are added later.

---

## 7. Load at least one FBX model on startup

When the program starts, at least one FBX model should be loaded and rendered correctly.

If you use the simple `ModelViewer` approach:

```cpp
std::shared_ptr<Mesh> mesh = LoadFBXMesh(u8"SM_Chest.fbx");
myMeshes.emplace("SM_Chest", mesh);
```

If you use `MeshLibrary`, then the loading can happen inside `MeshLibrary::Initialize()`:

```cpp
void MeshLibrary::Initialize()
{
    InitUnitCube();
    InitAxes();

    LoadFBXMesh(u8"SM_Chest.fbx");
}
```

Then `ModelViewer` can ask for the mesh later:

```cpp
auto chestMesh = myMeshLibrary.GetMesh("SM_Chest");
```

---

## 8. Make sure old primitives still work

The old generated models must still work.

For example:

- Cube
- Axes
- Plane, if you have one

You should be able to render:

- A generated primitive
- An FBX model
- Multiple copies of models

at the same time.

This is important because the assignment builds on the previous assignment.  
The new FBX-loading feature should not break the old rendering functionality.

---

## Final checklist

- [ ] TGA FBX is added to the project.
- [ ] The project compiles and links.
- [ ] `InitImporter()` is called at startup.
- [ ] `UninitImporter()` is called on shutdown.
- [ ] You decided whether to use a simple `ModelViewer` function or a `MeshLibrary`.
- [ ] A `LoadFBXMesh()` function exists.
- [ ] FBX data is converted into your own mesh format.
- [ ] Your own `Mesh`, `Vertex`, and `Element` structures are filled correctly.
- [ ] Multiple elements are handled correctly.
- [ ] `VertexOffset` is correct.
- [ ] `IndexOffset` is correct.
- [ ] `NumVertices` is correct.
- [ ] `NumIndices` is correct.
- [ ] Temporary random vertex colors are added if needed.
- [ ] At least one multi-element FBX model loads on startup.
- [ ] The FBX model renders correctly.
- [ ] Old primitives still render correctly.
- [ ] You can render both primitives and FBX models at the same time.

---

## Short version

Assignment 2.1 is mainly about:

1. Loading an FBX file using TGA FBX.
2. Converting it into your own mesh format.
3. Handling multiple elements correctly.
4. Rendering the FBX model.
5. Keeping old primitive rendering working.

The `MeshLibrary` idea is just an optional way to organize this better.  
It is a small mesh storage/loading class, not a full asset-management system.
