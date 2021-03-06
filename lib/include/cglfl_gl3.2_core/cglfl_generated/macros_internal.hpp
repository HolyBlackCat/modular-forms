#pragma once

// This file is a part of CGLFL (configurable OpenGL function loader).
// Generated, do no edit!
//
// Version: 1.0.0
// API: gl 3.2 (core profile)
// Extensions: none

#define CGLFL_PRIMARY_FUNC_COUNT 316
#define CGLFL_PRIMARY_FUNCS \
    glActiveTexture \
    glAttachShader \
    glBeginConditionalRender \
    glBeginQuery \
    glBeginTransformFeedback \
    glBindAttribLocation \
    glBindBuffer \
    glBindBufferBase \
    glBindBufferRange \
    glBindFragDataLocation \
    glBindFramebuffer \
    glBindRenderbuffer \
    glBindTexture \
    glBindVertexArray \
    glBlendColor \
    glBlendEquation \
    glBlendEquationSeparate \
    glBlendFunc \
    glBlendFuncSeparate \
    glBlitFramebuffer \
    glBufferData \
    glBufferSubData \
    glCheckFramebufferStatus \
    glClampColor \
    glClear \
    glClearBufferfi \
    glClearBufferfv \
    glClearBufferiv \
    glClearBufferuiv \
    glClearColor \
    glClearDepth \
    glClearStencil \
    glClientWaitSync \
    glColorMask \
    glColorMaski \
    glCompileShader \
    glCompressedTexImage1D \
    glCompressedTexImage2D \
    glCompressedTexImage3D \
    glCompressedTexSubImage1D \
    glCompressedTexSubImage2D \
    glCompressedTexSubImage3D \
    glCopyBufferSubData \
    glCopyTexImage1D \
    glCopyTexImage2D \
    glCopyTexSubImage1D \
    glCopyTexSubImage2D \
    glCopyTexSubImage3D \
    glCreateProgram \
    glCreateShader \
    glCullFace \
    glDeleteBuffers \
    glDeleteFramebuffers \
    glDeleteProgram \
    glDeleteQueries \
    glDeleteRenderbuffers \
    glDeleteShader \
    glDeleteSync \
    glDeleteTextures \
    glDeleteVertexArrays \
    glDepthFunc \
    glDepthMask \
    glDepthRange \
    glDetachShader \
    glDisable \
    glDisableVertexAttribArray \
    glDisablei \
    glDrawArrays \
    glDrawArraysInstanced \
    glDrawBuffer \
    glDrawBuffers \
    glDrawElements \
    glDrawElementsBaseVertex \
    glDrawElementsInstanced \
    glDrawElementsInstancedBaseVertex \
    glDrawRangeElements \
    glDrawRangeElementsBaseVertex \
    glEnable \
    glEnableVertexAttribArray \
    glEnablei \
    glEndConditionalRender \
    glEndQuery \
    glEndTransformFeedback \
    glFenceSync \
    glFinish \
    glFlush \
    glFlushMappedBufferRange \
    glFramebufferRenderbuffer \
    glFramebufferTexture \
    glFramebufferTexture1D \
    glFramebufferTexture2D \
    glFramebufferTexture3D \
    glFramebufferTextureLayer \
    glFrontFace \
    glGenBuffers \
    glGenFramebuffers \
    glGenQueries \
    glGenRenderbuffers \
    glGenTextures \
    glGenVertexArrays \
    glGenerateMipmap \
    glGetActiveAttrib \
    glGetActiveUniform \
    glGetActiveUniformBlockName \
    glGetActiveUniformBlockiv \
    glGetActiveUniformName \
    glGetActiveUniformsiv \
    glGetAttachedShaders \
    glGetAttribLocation \
    glGetBooleani_v \
    glGetBooleanv \
    glGetBufferParameteri64v \
    glGetBufferParameteriv \
    glGetBufferPointerv \
    glGetBufferSubData \
    glGetCompressedTexImage \
    glGetDoublev \
    glGetError \
    glGetFloatv \
    glGetFragDataLocation \
    glGetFramebufferAttachmentParameteriv \
    glGetInteger64i_v \
    glGetInteger64v \
    glGetIntegeri_v \
    glGetIntegerv \
    glGetMultisamplefv \
    glGetProgramInfoLog \
    glGetProgramiv \
    glGetQueryObjectiv \
    glGetQueryObjectuiv \
    glGetQueryiv \
    glGetRenderbufferParameteriv \
    glGetShaderInfoLog \
    glGetShaderSource \
    glGetShaderiv \
    glGetString \
    glGetStringi \
    glGetSynciv \
    glGetTexImage \
    glGetTexLevelParameterfv \
    glGetTexLevelParameteriv \
    glGetTexParameterIiv \
    glGetTexParameterIuiv \
    glGetTexParameterfv \
    glGetTexParameteriv \
    glGetTransformFeedbackVarying \
    glGetUniformBlockIndex \
    glGetUniformIndices \
    glGetUniformLocation \
    glGetUniformfv \
    glGetUniformiv \
    glGetUniformuiv \
    glGetVertexAttribIiv \
    glGetVertexAttribIuiv \
    glGetVertexAttribPointerv \
    glGetVertexAttribdv \
    glGetVertexAttribfv \
    glGetVertexAttribiv \
    glHint \
    glIsBuffer \
    glIsEnabled \
    glIsEnabledi \
    glIsFramebuffer \
    glIsProgram \
    glIsQuery \
    glIsRenderbuffer \
    glIsShader \
    glIsSync \
    glIsTexture \
    glIsVertexArray \
    glLineWidth \
    glLinkProgram \
    glLogicOp \
    glMapBuffer \
    glMapBufferRange \
    glMultiDrawArrays \
    glMultiDrawElements \
    glMultiDrawElementsBaseVertex \
    glPixelStoref \
    glPixelStorei \
    glPointParameterf \
    glPointParameterfv \
    glPointParameteri \
    glPointParameteriv \
    glPointSize \
    glPolygonMode \
    glPolygonOffset \
    glPrimitiveRestartIndex \
    glProvokingVertex \
    glReadBuffer \
    glReadPixels \
    glRenderbufferStorage \
    glRenderbufferStorageMultisample \
    glSampleCoverage \
    glSampleMaski \
    glScissor \
    glShaderSource \
    glStencilFunc \
    glStencilFuncSeparate \
    glStencilMask \
    glStencilMaskSeparate \
    glStencilOp \
    glStencilOpSeparate \
    glTexBuffer \
    glTexImage1D \
    glTexImage2D \
    glTexImage2DMultisample \
    glTexImage3D \
    glTexImage3DMultisample \
    glTexParameterIiv \
    glTexParameterIuiv \
    glTexParameterf \
    glTexParameterfv \
    glTexParameteri \
    glTexParameteriv \
    glTexSubImage1D \
    glTexSubImage2D \
    glTexSubImage3D \
    glTransformFeedbackVaryings \
    glUniform1f \
    glUniform1fv \
    glUniform1i \
    glUniform1iv \
    glUniform1ui \
    glUniform1uiv \
    glUniform2f \
    glUniform2fv \
    glUniform2i \
    glUniform2iv \
    glUniform2ui \
    glUniform2uiv \
    glUniform3f \
    glUniform3fv \
    glUniform3i \
    glUniform3iv \
    glUniform3ui \
    glUniform3uiv \
    glUniform4f \
    glUniform4fv \
    glUniform4i \
    glUniform4iv \
    glUniform4ui \
    glUniform4uiv \
    glUniformBlockBinding \
    glUniformMatrix2fv \
    glUniformMatrix2x3fv \
    glUniformMatrix2x4fv \
    glUniformMatrix3fv \
    glUniformMatrix3x2fv \
    glUniformMatrix3x4fv \
    glUniformMatrix4fv \
    glUniformMatrix4x2fv \
    glUniformMatrix4x3fv \
    glUnmapBuffer \
    glUseProgram \
    glValidateProgram \
    glVertexAttrib1d \
    glVertexAttrib1dv \
    glVertexAttrib1f \
    glVertexAttrib1fv \
    glVertexAttrib1s \
    glVertexAttrib1sv \
    glVertexAttrib2d \
    glVertexAttrib2dv \
    glVertexAttrib2f \
    glVertexAttrib2fv \
    glVertexAttrib2s \
    glVertexAttrib2sv \
    glVertexAttrib3d \
    glVertexAttrib3dv \
    glVertexAttrib3f \
    glVertexAttrib3fv \
    glVertexAttrib3s \
    glVertexAttrib3sv \
    glVertexAttrib4Nbv \
    glVertexAttrib4Niv \
    glVertexAttrib4Nsv \
    glVertexAttrib4Nub \
    glVertexAttrib4Nubv \
    glVertexAttrib4Nuiv \
    glVertexAttrib4Nusv \
    glVertexAttrib4bv \
    glVertexAttrib4d \
    glVertexAttrib4dv \
    glVertexAttrib4f \
    glVertexAttrib4fv \
    glVertexAttrib4iv \
    glVertexAttrib4s \
    glVertexAttrib4sv \
    glVertexAttrib4ubv \
    glVertexAttrib4uiv \
    glVertexAttrib4usv \
    glVertexAttribI1i \
    glVertexAttribI1iv \
    glVertexAttribI1ui \
    glVertexAttribI1uiv \
    glVertexAttribI2i \
    glVertexAttribI2iv \
    glVertexAttribI2ui \
    glVertexAttribI2uiv \
    glVertexAttribI3i \
    glVertexAttribI3iv \
    glVertexAttribI3ui \
    glVertexAttribI3uiv \
    glVertexAttribI4bv \
    glVertexAttribI4i \
    glVertexAttribI4iv \
    glVertexAttribI4sv \
    glVertexAttribI4ubv \
    glVertexAttribI4ui \
    glVertexAttribI4uiv \
    glVertexAttribI4usv \
    glVertexAttribIPointer \
    glVertexAttribPointer \
    glViewport \
    glWaitSync

#define CGLFL_EXT_COUNT 0
#define CGLFL_EXTS(X) // None
