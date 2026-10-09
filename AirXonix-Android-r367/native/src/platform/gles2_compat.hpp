#pragma once
// Minimal OpenGL ES 2.0 ABI declarations for PortMaster cross-builds.
// Runtime symbols are provided by the firmware's libGLESv2/Mali stack.
#include <cstddef>
#include <cstdint>
using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLvoid = void;
using GLbyte = std::int8_t;
using GLshort = std::int16_t;
using GLint = int;
using GLsizei = int;
using GLubyte = std::uint8_t;
using GLushort = std::uint16_t;
using GLuint = unsigned int;
using GLfloat = float;
using GLchar = char;
using GLsizeiptr = std::ptrdiff_t;
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NO_ERROR 0
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_TRIANGLES 0x0004
#define GL_BLEND 0x0BE2
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_DITHER 0x0BD0
#define GL_LEQUAL 0x0203
#define GL_ONE 1
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_FLOAT 0x1406
#define GL_RGBA 0x1908
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE0 0x84C0
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
extern "C" {
void glActiveTexture(GLenum);
void glAttachShader(GLuint,GLuint);
void glBindAttribLocation(GLuint,GLuint,const GLchar*);
void glBindBuffer(GLenum,GLuint);
void glBindTexture(GLenum,GLuint);
void glBlendFunc(GLenum,GLenum);
void glBufferData(GLenum,GLsizeiptr,const GLvoid*,GLenum);
void glClear(GLbitfield);
void glClearColor(GLfloat,GLfloat,GLfloat,GLfloat);
void glCompileShader(GLuint);
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum);
void glDeleteBuffers(GLsizei,const GLuint*);
void glDeleteProgram(GLuint);
void glDeleteShader(GLuint);
void glDeleteTextures(GLsizei,const GLuint*);
void glDepthFunc(GLenum);
void glDepthMask(GLboolean);
void glDisable(GLenum);
void glDrawElements(GLenum,GLsizei,GLenum,const GLvoid*);
void glEnable(GLenum);
void glEnableVertexAttribArray(GLuint);
void glGenBuffers(GLsizei,GLuint*);
void glGenTextures(GLsizei,GLuint*);
GLenum glGetError(void);
const GLubyte* glGetString(GLenum);
void glGetProgramiv(GLuint,GLenum,GLint*);
void glGetShaderInfoLog(GLuint,GLsizei,GLsizei*,GLchar*);
void glGetShaderiv(GLuint,GLenum,GLint*);
GLint glGetUniformLocation(GLuint,const GLchar*);
void glLinkProgram(GLuint);
void glShaderSource(GLuint,GLsizei,const GLchar* const*,const GLint*);
void glTexImage2D(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid*);
void glTexParameteri(GLenum,GLenum,GLint);
void glUniform1f(GLint,GLfloat);
void glUniform2f(GLint,GLfloat,GLfloat);
void glUniform3f(GLint,GLfloat,GLfloat,GLfloat);
void glUniform1i(GLint,GLint);
void glUseProgram(GLuint);
void glVertexAttribPointer(GLuint,GLint,GLenum,GLboolean,GLsizei,const GLvoid*);
void glViewport(GLint,GLint,GLsizei,GLsizei);
}
