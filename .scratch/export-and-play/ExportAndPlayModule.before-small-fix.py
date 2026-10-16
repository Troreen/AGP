print("- Load ExportAndPlay.py")

import os
from pathlib import Path
import shutil
#import subprocess

import tkinter
from tkinter import filedialog

#region Helper Functions



def AddBackSlashAtStart(aStringValue):
    returnVal = f"\{aStringValue}"
    return returnVal



''' FindDirectories
- Locates the source files for FromPath, ToPath and GamePath
'''
def FindDirectories():
    # Find path where 'this' script exists
    dirPath = Path(os.path.dirname(os.path.realpath(__file__)))
    print(f"- Python Directory: {dirPath}")
    
    # Save paths to information files
    fromSourcePath = Path(os.path.dirname(os.path.realpath(__file__)) + '/UE_PythonPaths/taloc_FromPath.txt')
    toSourcePath = Path(os.path.dirname(os.path.realpath(__file__)) + '/UE_PythonPaths/taloc_ToPath.txt')
    gameSourcePath = Path(os.path.dirname(os.path.realpath(__file__)) + '/UE_PythonPaths/taloc_GamePath.txt')
    print("<< Path source locations >>")
    print(f"- 'from' source location: {fromSourcePath}")
    print(f"- 'to' source location: {toSourcePath}")
    print(f"- 'game' source location: {gameSourcePath}")

    # Return source paths as tuple
    return (fromSourcePath, toSourcePath, gameSourcePath)



''' QueryPaths
- Queries source files to identify FromPath and ToPath
'''
def QueryPaths(aPaths):
    # Assign queries
    fromQuery = Path(aPaths[0])
    toQuery = Path(aPaths[1])
    gameQuery = Path(aPaths[2])

    # Check if queries are invalid - if true: return
    if not fromQuery.exists():
        print(f"- FromPath source '{aPaths[0]}' is invalid")
        return False
    elif not toQuery.exists():
        print(f"- ToPath source '{aPaths[1]}' is invalid")
        return False
    elif not gameQuery.exists():
        print(f"- GamePath source '{aPaths[2]}' is invalid")
        return False

    # Assign fromPath and toPath
    fromPath = ""
    toPath = ""
    gamePath = ""
    print("<< Paths >>")
    with fromQuery.open() as file:
        fromPath = file.readline()
        print("- From path: " + fromPath)

    with toQuery.open() as file:
        toPath = file.readline()
        print("- To path: " + toPath)
    
    with gameQuery.open() as file:
        gamePath = file.readline()
        print("- Game path: " + gamePath)

    # Return paths as tuple
    return (fromPath, toPath, gamePath)



''' MoveFile
- Moves file from folder[0] to folder[1]
'''
def MoveFile(aFileToMove, aPaths):
    print("<< Move Level File >>")
    file = AddBackSlashAtStart(aFileToMove)
    fileQuery = Path(aPaths[0] + file)
    print(fileQuery)
    if not fileQuery.exists():
        print(f"- Couldn't find file to move at location: '{aPaths[0]}'")
        return False

    print(f"- Moving file:'{aFileToMove}' from '{aPaths[0]}' to {aPaths[1]}")
    
    # Copy and add prefix
    sourceFilePath = os.path.join(aPaths[0], aFileToMove)
    destinationPath = aPaths[1]
    
    fileNameWithPrefix = "EAP.json"
    destination = destinationPath + AddBackSlashAtStart(fileNameWithPrefix)

    shutil.copy(sourceFilePath, destination)

    print("Copied: ", aFileToMove, " as: ", fileNameWithPrefix)
    return True



''' BootGame
- Run .exe at location
'''
def BootGame(aGamePath):
    print("<< Start Game >>")
    gameQuery = Path(aGamePath)
    if not gameQuery.exists():
        print(f"- Game.exe not found at directory: '{aGamePath}'")
        return False

    #exe = gameQuery.resolve()
    #subprocess.Popen([str(exe)])

    print(f"- Start game at: {aGamePath}")
    os.startfile(aGamePath)
    return True

#endregion

#region public API

''' MoveLevelFile
- Copy Level to Game Engine
'''
def MoveLevelFile():
    sourceDirectories = FindDirectories()
    localPaths = QueryPaths(sourceDirectories)
    if not localPaths:
        print("- localPaths not valid!")
        return False

    #Open Browser to select file to move
    filetypes = ( ('json files', '*.json') )
    tkinter.Tk().withdraw() # prevents an empty tkinter window from appearing
    folder_path = filedialog.askopenfilename( initialdir=Path(f"{localPaths[0]}") )
    
    # Save file name
    LevelFileName = os.path.basename(folder_path)

    return MoveFile(LevelFileName, localPaths)



''' PlayInGameEngine
- Boot Game
'''
def PlayInGameEngine():
    sourceDirectories = FindDirectories()
    localPaths = QueryPaths(sourceDirectories)
    if not localPaths:
        print("- localPaths not valid!")
        return False

    return BootGame(localPaths[2] + "\Game.exe")



''' ExportAndPlay
- Export Level
- Copy Level to Game Engine
- Boot Game
'''
def ExportAndPlay():
    MoveLevelFile()
    PlayInGameEngine()

#endregion