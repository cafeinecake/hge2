project( HGECXIMAGE )
cmake_minimum_required( VERSION 2.8 )

#------ sources ------
set( HGECX_SRC
	tif_xfile.cpp
	xfile.h
	ximabmp.cpp	ximabmp.h
	ximacfg.h
	ximadef.h
	ximadsp.cpp
	ximaenc.cpp
	ximaexif.cpp
	ximage.cpp	ximage.h
	ximagif.cpp	ximagif.h
	ximahist.cpp
	ximaico.cpp	ximaico.h
	ximainfo.cpp
	ximaint.cpp
	ximaiter.h
	ximajas.cpp	ximajas.h
	ximajbg.cpp	ximajbg.h
	ximajpg.cpp	ximajpg.h
	ximalpha.cpp
	ximalyr.cpp
	ximamng.cpp	ximamng.h
	ximapal.cpp
	ximapcx.cpp	ximapcx.h
	ximapng.cpp	ximapng.h
	ximapsd.cpp	ximapsd.h
	ximaraw.cpp	ximaraw.h
	ximasel.cpp
	ximaska.cpp	ximaska.h
	ximatga.cpp	ximatga.h
	ximath.cpp	ximath.h
	ximatif.cpp	ximatif.h
	ximatran.cpp
	ximawbmp.cpp	ximawbmp.h
	ximawmf.cpp	ximawmf.h
	ximawnd.cpp
	xiofile.h
	xmemfile.cpp	xmemfile.h
	)

#------ additional includes ------
include_directories( ${HGE_SOURCE_DIR}/include )
include_directories( ${HGE_SOURCE_DIR}/deps )
include_directories( ${HGE_SOURCE_DIR}/deps/zlib-1.2.5 )

#------ link directories listed ------
link_directories( ${HGE_SOURCE_DIR}/linklib )

#------ target -----
add_library( hgecximage STATIC ${HGECX_SRC} )
