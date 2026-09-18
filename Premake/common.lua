----------------------------------------------------------------------------
-- the dirs table is a listing of absolute paths, since we generate projects
-- and files it makes a lot of sense to make them absolute to avoid problems
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
dirs = {}
dirs["root"] 					= os.realpath("../")
dirs["bin"]						= os.realpath(dirs.root .. "Bin/")
dirs["int"]						= os.realpath(dirs.root .. "Intermediate/")
dirs["lib"]						= os.realpath(dirs.root .. "Lib/")
dirs["source"] 					= os.realpath(dirs.root .. "Source/")
dirs["content"] 				= os.realpath(dirs.root .. "Content/")
dirs["dependencies"]			= os.realpath(dirs.root .. "Dependencies/")
dirs["application"]				= os.realpath(dirs.source .. "Application/")
dirs["engine"]				    = os.realpath(dirs.source .. "Engine/")
dirs["utilities"]				= os.realpath(dirs.source .. "Utilities/")

dirs["game"]					= os.realpath(dirs.application .. "Game")
dirs["logger"]					= os.realpath(dirs.utilities .. "Logger")
