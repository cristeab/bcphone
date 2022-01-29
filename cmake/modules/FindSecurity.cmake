#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find Security
# Once done, this will define
#
#  Security_FOUND - system has Security
#  Security_INCLUDE_DIRS - the Security include directories
#  Security_LIBRARIES - link these to use Security

include(FindPkgMacros)
findpkg_begin(Security)

# construct search paths
set(Security_PREFIX_PATH ${Security_HOME} $ENV{Security_HOME})
create_search_paths(Security)
# redo search if prefix path changed
clear_if_changed(Security_PREFIX_PATH
  Security_LIBRARY_FWK
  Security_LIBRARY_REL
  Security_LIBRARY_DBG
  Security_INCLUDE_DIR
)

set(Security_LIBRARY_NAMES Security)
get_debug_names(Security_LIBRARY_NAMES)

use_pkgconfig(Security_PKGC Security)

findpkg_framework(Security)

find_path(Security_INCLUDE_DIR NAMES Security.h HINTS ${Security_INC_SEARCH_PATH} ${Security_PKGC_INCLUDE_DIRS} PATH_SUFFIXES Security)
find_library(Security_LIBRARY_REL NAMES ${Security_LIBRARY_NAMES} HINTS ${Security_LIB_SEARCH_PATH} ${Security_PKGC_LIBRARY_DIRS})
find_library(Security_LIBRARY_DBG NAMES ${Security_LIBRARY_NAMES_DBG} HINTS ${Security_LIB_SEARCH_PATH} ${Security_PKGC_LIBRARY_DIRS})
make_library_set(Security_LIBRARY)
findpkg_finish(Security)
add_parent_dir(Security_INCLUDE_DIRS Security_INCLUDE_DIR)

