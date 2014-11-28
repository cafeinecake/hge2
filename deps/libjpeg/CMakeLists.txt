project( HGELIBJPEG )
cmake_minimum_required( VERSION 2.8 )

#------ sources ------
set( HGELIBJPEG_SRC
	jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c 
        jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c 
        jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c 
        jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c 
        jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c 
        jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c 
        jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c 
        jquant2.c jutils.c jmemmgr.c 
    )

#------ additional includes ------
include_directories( ${CMAKE_SOURCE_DIR}/include )

#------ link directories listed ------
link_directories( ${CMAKE_SOURCE_DIR}/linklib )

#------ target -----
add_library( hgelibjpeg STATIC ${HGELIBJPEG_SRC} )
