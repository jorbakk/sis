do shell script "/usr/local/bin/sisui&> /dev/null &"

do shell script "sleep .5"


tell application "System Events"

	tell application process "sisui"

		set frontmost to true

		set currentWindow to window 1

		set the position of currentWindow to {90, 65}

		do shell script "sleep .3"

		set the size of currentWindow to {1280, 960}		

	end tell

end tell