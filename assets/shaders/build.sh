#!/bin/bash

glslangValidator -V shader.vert -o shader.vert.spv
glslangValidator -V shader.frag -o shader.frag.spv
glslangValidator -V sky.vert -o sky.vert.spv
glslangValidator -V sky.frag -o sky.frag.spv