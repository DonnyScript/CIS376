# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "Arithma_Tech_autogen"
  "CMakeFiles\\Arithma_Tech_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Arithma_Tech_autogen.dir\\ParseCache.txt"
  )
endif()
