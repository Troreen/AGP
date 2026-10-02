

|Fil|Beskrivning|\*Äger %Ärver av|Övrigt|
|-|-|-|-|

|Game::Main.cpp|- Skapar och sätter filesystem paths.<br />- Påbörjar Logging<br />- Startar services<br />- Skapar Game variable<br />- Skapar GameApplication variable<br />- Säger åt spelet att starta (GameApplication.Run(game))|||
|-|-|-|-|
|Game::Game.h|- Håller i hur detta spelet ser ut och fungerar "rådata"|\* MeshLibrary||
|Game::GameApplication.h|- Håller i Timern<br />- Init windows<br />- Init Services<br />- Init Input<br />- Init Sound Settings<br />- Runs Main Game Loop once a frame<br />- Säger åt render att ske varje frame<br />+ Laddar in en ny scen från jsonfil och skapar en ny World|\* ApplicationSettings (struct)<br />\* World<br />\* Renderstuff<br />\* Textstuff<br />\* DebugCamera|!! Hårdkodar in en del namn och filepaths till scener|
|Game::MeshLibrary.h|- Laddar in alla meshes från FBX<br />- Laddar Animationer från FBX<br />- Skapar primitiva meshes (kuber etc)<br />- Håller endast i meshar, actors håller i sina transforms, animations etc.|\* ContentRoot filepath<br />\* Lista av alla meshes i spelet||
|||||

|World.h|- Nuvarande, aktiva scenen<br />- Kan hitta en actor<br />- Kan spawna actors i pågående scen|\* Camera ptr<br />\* Ptr till alla Actors||
|-|-|-|-|
|Actor.h|- Varje grej i världen|\* World ptr, vilken värld actorn tillhör<br />\* Tranform<br />\* Componenter|!! Hårdkodade actors skapas i Game::ConfigureWorld men spawnas med World.SpawnActor()<br />!! Scenskapade actors skapas i WorldFromSceneConverter::BuildWorldFromSceneData() i samband med att World skapas<br />!! DebugCamera spawnar sin egen actor|
|Component.h||\* Actor ptr, ägare<br />\* Name<br />\* Tags||
|SceneComponent.h||% Component<br />\* Transform, en local offset av sin actor||
|MeshComponentBase.h||% SceneComponent<br />\* Mesh ptr, något från meshLibrary<br />\* Lista av materials (MaterialAsset)||

|SkeletalMeshComponent.h||% MeshComponentBase<br />\* Lista av alla JointTransforms||
|-|-|-|-|
|AnimatorComponent.h||% Component<br />\* SkeletalMeshComponent ptr<br />\* AnimationTree ptr<br />\* Animations (AnimationAsset ptr)||
|CameraComponent.h||% SceneComponent<br />\* CommonUtilities::Camera3D||
|||||
|||||



