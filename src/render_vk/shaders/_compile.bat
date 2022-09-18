@REM glslc image.vert -o image.vert.spv
@REM glslc image.frag -o image.frag.spv

glslangValidator -V -x -o image.vert.u32 image.vert
glslangValidator -V -x -o image.frag.u32 image.frag

pause

