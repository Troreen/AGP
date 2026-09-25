# Game settings

Edit `ApplicationSettings.json` and `InputBindings.json` here, then restart the game. Debug, Release, and Retail all read this same folder. The startup log reports its path.

The build does not copy or replace these files. `contentPath` still resolves relative to the running `Game.exe` directory.
