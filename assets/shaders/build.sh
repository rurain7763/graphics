#!/bin/bash

glslangValidator -V shader.vert -o shader.vert.spv
glslangValidator -V shader.frag -o shader.frag.spv
glslangValidator -V sky.vert -o sky.vert.spv
glslangValidator -V sky.frag -o sky.frag.spv
glslangValidator -V object.vert -o object.vert.spv
glslangValidator -V sprite.frag -o sprite.frag.spv
glslangValidator -V finalize.frag -o finalize.frag.spv
glslangValidator -V fullscreen.vert -o fullscreen.vert.spv
