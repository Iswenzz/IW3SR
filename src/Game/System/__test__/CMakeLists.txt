# Schedule is free of engine state, so it is compiled straight into the test rather than reached
# through the game library, which cannot be loaded without the game.
target_sources(IW3SR.Tests PRIVATE
	Schedule.test.cpp
	../Schedule.cpp)
