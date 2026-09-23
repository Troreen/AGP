`SceneData` currently lives in `GameFramework` which feels backwards considering that is game specific data. It should be moved to the `Game` module. 

`GameScene` should be renamed to something like `SceneLoader` since it isnt a scene by itself it is a helper that loads scenes.

Rename `IGame` into soemthing more explanatory like `GameInterface`