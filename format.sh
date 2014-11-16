#!/bin/sh

astyle --indent=spaces=2 --style=stroustrup \
	--add-brackets \
	--max-code-length=100 \
	--break-blocks \
	--convert-tabs \
	--recursive "*.cpp" "*.h" "*.hpp"
