#!/usr/bin/env python

import os
import sys
import subprocess
from sys import argv

GLSL_STAGE_ID = ["vert", "tese", "tesc", "geom", "frag", "comp"]
GLSL_STAGE_NAME = ["vertex", "tesselation evaluation", "tesselation control", "geometry", "fragment", "compute"]
GLSL_STAGE_MACRO = ["_VS_", "_TES_", "_TCS_", "_GS_", "_FS_", "_CS_"]
GLSL_ENTRY_POINTS = ["vsMain", "tesMain", "tcsMain", "gsMain", "fsMain", "csMain"]

def getGLSLCompiler(baseDir):
    glslcBin = None
    if sys.platform == "darwin":
        glslcBin = os.path.join(baseDir, *['extern', 'macos', 'vulkan', 'macOS', 'bin', 'glslc'])
        if os.path.exists(glslcBin) and os.path.isfile(glslcBin):
            print("Found GLSL compiler: " + glslcBin)
        else:
            glslcBin = None
    return glslcBin

def findShaderFiles(baseDir):
    shaderFiles = []
    for root, dirs, files in os.walk(baseDir):
        for fName in files:
            base, ext = os.path.splitext(fName)
            if ext == ".glsl":
                shaderFiles.append(os.path.join(root, fName))
    return shaderFiles

def getShaderFiles(baseDir):
    shaderFiles = []
    if len(argv) == 1:
        shaderFiles += findShaderFiles(baseDir)
    else:
        for entry in argv[1:]:
            if os.path.isfile(entry):
                shaderFiles.append(os.path.abspath(entry))
            elif os.path.isdir(entry):
                shaderFiles += findShaderFiles(entry)
    return shaderFiles

def needToCompile(shaderFile, inlineFile):
    result = False
    if os.path.exists(inlineFile):
        smTime = os.path.getmtime(shaderFile)
        imTime = os.path.getmtime(inlineFile)
        if imTime < smTime:
            print("Inline shader %s is out of date, compiling" % inlineFile)
            result = True
    else:
        print("Inline shader %s does not exist, compiling" % inlineFile)
        result = True
    if not result:
        print "Inline shader file up to date, skipping"
    return result

def preprocessShader(shaderFile):
    stages = []
    shaderName = "UNKNOWN"
    with open(shaderFile, "r") as lines:
        for line in lines:
            if line.startswith("//@@"):
                shaderName = line[4:].strip()
            for i, entryPoint in enumerate(GLSL_ENTRY_POINTS):
                declaration = "void %s()" % entryPoint
                if declaration in line:
                    stages.append(i)
                    break
    return (shaderName, stages)

def binaryToHexArray(binary):
    result = ""
    for i, val in enumerate(binary):
        line = ""
        if i > 0:
            line += ","
        if (i % 24) == 0:
            line += "\n    "
        else:
            line += " "
        line += "0x%02x" % ord(val[0])
        result += line
    return "{%s\n};" % result

def compileStages(shaderFile, stages, glslcBin):
    bytecode = {}
    for idx in stages:
        print("Compiling %s shader" % GLSL_STAGE_NAME[idx])
        cmd = [
            glslcBin,
            "-std=450core",
            "-fshader-stage=%s" % GLSL_STAGE_ID[idx],
            "-D%s=main" % GLSL_ENTRY_POINTS[idx],
            "-D%s" % GLSL_STAGE_MACRO[idx],
            shaderFile, "-o", "-"
        ]
        print("%s" % " ".join(cmd))
        try:
            binary = subprocess.check_output(cmd)
            bytecode[GLSL_STAGE_ID[idx]] = binaryToHexArray(binary)
            print("Success, output SPIRV size: %d byte(s)" % len(binary))
        except (subprocess.CalledProcessError):
            return None
    return bytecode

def main():
    baseDir = os.getcwd()
    glslcBin = getGLSLCompiler(baseDir)
    if glslcBin:
        shaderFiles = getShaderFiles(baseDir)
        for shaderFile in shaderFiles:
            print("Compiling " + shaderFile)
            base, ext = os.path.splitext(shaderFile)
            inlineFile = base + ".inl"
            if needToCompile(shaderFile, inlineFile):
                shaderName, stages = preprocessShader(shaderFile)
                print("Detected internal name: %s" % shaderName)
                stagesBin = compileStages(shaderFile, stages, glslcBin)
                with open(inlineFile, "w") as output:
                    for idx, id in enumerate(GLSL_STAGE_ID):
                        if id in stagesBin:
                            stage = stagesBin[id]
                            output.write("static const char %s_%s[] =\n%s\n\n" % (shaderName, id.upper(), stage))
    else:
        print("Failed to find GLSL compiler")
        sys.exit(1)

if __name__ == "__main__":
    main()
