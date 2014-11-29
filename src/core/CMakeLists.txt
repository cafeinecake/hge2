project(hgecore)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Common source
set(HGE_SRCS
	common/demo.cpp
	common/random.cpp
	common/timer.cpp
	common/hge2.cpp
	common/hge_utils.h
	${CMAKE_SOURCE_DIR}/include/hge.h
	${CMAKE_SOURCE_DIR}/include/hgeanim.h
	${CMAKE_SOURCE_DIR}/include/hgecolor.h
	${CMAKE_SOURCE_DIR}/include/hgedistort.h
	${CMAKE_SOURCE_DIR}/include/hgefont.h
	${CMAKE_SOURCE_DIR}/include/hgegui.h
	${CMAKE_SOURCE_DIR}/include/hgeguictrls.h
	${CMAKE_SOURCE_DIR}/include/hgeparticle.h
	${CMAKE_SOURCE_DIR}/include/hgerect.h
	${CMAKE_SOURCE_DIR}/include/hgeresource.h
	${CMAKE_SOURCE_DIR}/include/hgesprite.h
	${CMAKE_SOURCE_DIR}/include/hgestrings.h
	${CMAKE_SOURCE_DIR}/include/hgevector.h
	hge_portable.h hge_portable.cpp
	)

include_directories( ${CMAKE_SOURCE_DIR}/deps )

set(HGE_SRCS
	${HGE_SRCS}
	graphics.cpp
	ini.cpp
	input.cpp
	power.cpp
	resource.cpp
	system.cpp
	hge_glfuncs.h hge_impl.h
	)

# Platform specific files
if(UNIX OR APPLE)
	# Unix/Linux and OS X share these
	set(HGE_SRCS
		${HGE_SRCS}
		sound_openal.cpp
	)

	if(APPLE)
		set(HGE_SRCS
			${HGE_SRCS}
			macosx_support.mm
		)
	endif(APPLE)
else(UNIX OR APPLE)
	# Win32
	set(HGE_SRCS
	${HGE_SRCS}
	sound_libbass.cpp
	)
endif(UNIX OR APPLE)

set(HS ${CMAKE_SOURCE_DIR}/src/helpers)
set(HGE_HELPER_SRCS
	${HS}/hgeanim.cpp
	${HS}/hgecolor.cpp
	${HS}/hgedistort.cpp
	${HS}/hgefont.cpp
	${HS}/hgegui.cpp
	${HS}/hgeguictrls.cpp
	${HS}/hgeparticle.cpp
	${HS}/hgepmanager.cpp
	${HS}/hgerect.cpp
	${HS}/hgeresource.cpp
	${HS}/hgesprite.cpp
	${HS}/hgestrings.cpp
	${HS}/hgevector.cpp
	${HS}/parser.cpp
	${HS}/resources.cpp
)

if(WIN32)
	add_library(hge2core ${USE_SHARED} ${HGE_SRCS} ${HGE_HELPER_SRCS})
	target_link_libraries(hge2core ${HGE2_LIBRARIES})
else()
	add_library(hge2core ${USE_SHARED} ${HGE_SRCS})
	target_link_libraries(hge2core ${HGE2_LIBRARIES})

	add_library(hge2helpers ${USE_SHARED} ${HGE_HELPER_SRCS})
	target_link_libraries(hge2helpers hge2core ${HGE2_LIBRARIES})
endif()
