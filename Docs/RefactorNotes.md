`SceneData` currently lives in `GameFramework` which feels backwards considering that is game specific data. It should be moved to the `Game` module. 

`GameScene` should be renamed to something like `SceneLoader` since it isnt a scene by itself it is a helper that loads scenes.

Rename `IGame` into soemthing more explanatory like `GameInterface`


Overall architecture needs to be rethought. There is too much abstraction and things made for "reusablity" this just creates a lot of complexity and makes it hard to understand what is going on. The engine will not need these kinds of features given our 3 game scope. Things that need to be changed from game to game can be done so manually. 

instead of exposing the internal layers of scene loading we should hide it behind a simple SceneLoader

Component regisrty is redundant and can be put inside SceneLoader