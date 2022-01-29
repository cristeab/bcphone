#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find CoreServices
# Once done, this will define
#
#  CoreServices_FOUND - system has CoreServices
#  CoreServices_INCLUDE_DIRS - the CoreServices include directories
#  CoreServices_LIBRARIES - link these to use CoreServices

include(FindPkgMacros)
findpkg_begin(CoreServices)

# construct search paths
set(CoreServices_PREFIX_PATH ${CoreServices_HOME} $ENV{CoreServices_HOME})
create_search_paths(CoreServices)
# redo search if prefix path changed
clear_if_changed(CoreServices_PREFIX_PATH
  CoreServices_LIBRARY_FWK
  CoreServices_LIBRARY_REL
  CoreServices_LIBRARY_DBG
  CoreServices_INCLUDE_DIR
)

set(CoreServices_LIBRARY_NAMES CoreServices)
get_debug_names(CoreServices_LIBRARY_NAMES)

use_pkgconfig(CoreServices_PKGC CoreServices)

findpkg_framework(CoreServices)

find_path(CoreServices_INCLUDE_DIR NAMES CoreServices.h HINTS ${CoreServices_INC_SEARCH_PATH} ${CoreServices_PKGC_INCLUDE_DIRS} PATH_SUFFIXES CoreServices)
find_library(CoreServices_LIBRARY_REL NAMES ${CoreServices_LIBRARY_NAMES} HINTS ${CoreServices_LIB_SEARCH_PATH} ${CoreServices_PKGC_LIBRARY_DIRS})
find_library(CoreServices_LIBRARY_DBG NAMES ${CoreServices_LIBRARY_NAMES_DBG} HINTS ${CoreServices_LIB_SEARCH_PATH} ${CoreServices_PKGC_LIBRARY_DIRS})
make_library_set(CoreServices_LIBRARY)
findpkg_finish(CoreServices)
add_parent_dir(CoreServices_INCLUDE_DIRS CoreServices_INCLUDE_DIR)

