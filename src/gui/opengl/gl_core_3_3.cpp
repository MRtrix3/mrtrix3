/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */
#include <algorithm>
#include <cstring>
#include <cstddef>
#include "gui/opengl/gl_core_3_3.h"

#if defined(__APPLE__)
#include <dlfcn.h>

static void* AppleGLGetProcAddress (const char* name)
{
  static void* image = NULL;
  if (NULL == image)
    image = dlopen ("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);

  return (image ? dlsym (image, (const char*) name) : NULL);
}
#endif /* __APPLE__ */

#if defined(__sgi) || defined (__sun)
#include <dlfcn.h>
#include <cstdio>

static void* SunGetProcAddress (const GLubyte* name)
{
  static void* h = NULL;
  static void* gpa;

  if (h == NULL)
  {
    if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL) return NULL;
    gpa = dlsym(h, "glXGetProcAddress");
  }

  if (gpa != NULL)
    return ((void*(*)(const GLubyte*))gpa)(name);
  else
    return dlsym(h, (const char*)name);
}
#endif /* __sgi || __sun */

#if defined(_WIN32)

#ifdef _MSC_VER
#pragma warning(disable: 4055)
#pragma warning(disable: 4054)
#endif

static int TestPointer(const PROC pTest)
{
	ptrdiff_t iTest;
	if(!pTest) return 0;
	iTest = (ptrdiff_t)pTest;
	
	if(iTest == 1 || iTest == 2 || iTest == 3 || iTest == -1) return 0;
	
	return 1;
}

static PROC WinGetProcAddress(const char *name)
{
	HMODULE glMod = NULL;
	PROC pFunc = wglGetProcAddress((LPCSTR)name);
	if(TestPointer(pFunc))
	{
		return pFunc;
	}
	glMod = GetModuleHandleA("OpenGL32.dll");
	return (PROC)GetProcAddress(glMod, (LPCSTR)name);
}
	
#define IntGetProcAddress(name) WinGetProcAddress(name)
#else
	#if defined(__APPLE__)
		#define IntGetProcAddress(name) AppleGLGetProcAddress(name)
	#else
		#if defined(__sgi) || defined(__sun)
			#define IntGetProcAddress(name) SunGetProcAddress(name)
		#else /* GLX */
		    #include <GL/glx.h>

			#define IntGetProcAddress(name) (*glXGetProcAddressARB)((const GLubyte*)name)
		#endif
	#endif
#endif

namespace gl
{
	namespace exts
	{
	}
	
	// Extension: 1.0
	typedef void (CODEGEN_FUNCPTR *PFNBLENDFUNCPROC)(GLenum, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARPROC)(GLbitfield);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARDEPTHPROC)(GLdouble);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARSTENCILPROC)(GLint);
	typedef void (CODEGEN_FUNCPTR *PFNCOLORMASKPROC)(GLboolean, GLboolean, GLboolean, GLboolean);
	typedef void (CODEGEN_FUNCPTR *PFNCULLFACEPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNDEPTHFUNCPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNDEPTHMASKPROC)(GLboolean);
	typedef void (CODEGEN_FUNCPTR *PFNDEPTHRANGEPROC)(GLdouble, GLdouble);
	typedef void (CODEGEN_FUNCPTR *PFNDISABLEPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWBUFFERPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNENABLEPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNFINISHPROC)();
	typedef void (CODEGEN_FUNCPTR *PFNFLUSHPROC)();
	typedef void (CODEGEN_FUNCPTR *PFNFRONTFACEPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNGETBOOLEANVPROC)(GLenum, GLboolean *);
	typedef void (CODEGEN_FUNCPTR *PFNGETDOUBLEVPROC)(GLenum, GLdouble *);
	typedef GLenum (CODEGEN_FUNCPTR *PFNGETERRORPROC)();
	typedef void (CODEGEN_FUNCPTR *PFNGETFLOATVPROC)(GLenum, GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNGETINTEGERVPROC)(GLenum, GLint *);
	typedef const GLubyte * (CODEGEN_FUNCPTR *PFNGETSTRINGPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNGETTEXIMAGEPROC)(GLenum, GLint, GLenum, GLenum, GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNGETTEXLEVELPARAMETERFVPROC)(GLenum, GLint, GLenum, GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNGETTEXLEVELPARAMETERIVPROC)(GLenum, GLint, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETTEXPARAMETERFVPROC)(GLenum, GLenum, GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNGETTEXPARAMETERIVPROC)(GLenum, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNHINTPROC)(GLenum, GLenum);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISENABLEDPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNLINEWIDTHPROC)(GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNLOGICOPPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNPIXELSTOREFPROC)(GLenum, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNPIXELSTOREIPROC)(GLenum, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNPOINTSIZEPROC)(GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNPOLYGONMODEPROC)(GLenum, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNREADBUFFERPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNREADPIXELSPROC)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNSCISSORPROC)(GLint, GLint, GLsizei, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNSTENCILFUNCPROC)(GLenum, GLint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNSTENCILMASKPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNSTENCILOPPROC)(GLenum, GLenum, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNTEXIMAGE1DPROC)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERFPROC)(GLenum, GLenum, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERFVPROC)(GLenum, GLenum, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERIVPROC)(GLenum, GLenum, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);
	
	// Extension: 1.1
	typedef void (CODEGEN_FUNCPTR *PFNBINDTEXTUREPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNCOPYTEXIMAGE1DPROC)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNCOPYTEXIMAGE2DPROC)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNCOPYTEXSUBIMAGE1DPROC)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNCOPYTEXSUBIMAGE2DPROC)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNDELETETEXTURESPROC)(GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWELEMENTSPROC)(GLenum, GLsizei, GLenum, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNGENTEXTURESPROC)(GLsizei, GLuint *);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISTEXTUREPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNPOLYGONOFFSETPROC)(GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNTEXSUBIMAGE1DPROC)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNTEXSUBIMAGE2DPROC)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
	
	// Extension: 1.2
	typedef void (CODEGEN_FUNCPTR *PFNBLENDCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNBLENDEQUATIONPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNCOPYTEXSUBIMAGE3DPROC)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWRANGEELEMENTSPROC)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNTEXIMAGE3DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNTEXSUBIMAGE3DPROC)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
	
	// Extension: 1.3
	typedef void (CODEGEN_FUNCPTR *PFNACTIVETEXTUREPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXIMAGE1DPROC)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXIMAGE2DPROC)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXIMAGE3DPROC)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXSUBIMAGE1DPROC)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXSUBIMAGE2DPROC)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXSUBIMAGE3DPROC)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNGETCOMPRESSEDTEXIMAGEPROC)(GLenum, GLint, GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLECOVERAGEPROC)(GLfloat, GLboolean);
	
	// Extension: 1.4
	typedef void (CODEGEN_FUNCPTR *PFNBLENDFUNCSEPARATEPROC)(GLenum, GLenum, GLenum, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNMULTIDRAWARRAYSPROC)(GLenum, const GLint *, const GLsizei *, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNMULTIDRAWELEMENTSPROC)(GLenum, const GLsizei *, GLenum, const GLvoid *const*, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNPOINTPARAMETERFPROC)(GLenum, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNPOINTPARAMETERFVPROC)(GLenum, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNPOINTPARAMETERIPROC)(GLenum, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNPOINTPARAMETERIVPROC)(GLenum, const GLint *);
	
	// Extension: 1.5
	typedef void (CODEGEN_FUNCPTR *PFNBEGINQUERYPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNBINDBUFFERPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNBUFFERDATAPROC)(GLenum, GLsizeiptr, const GLvoid *, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNBUFFERSUBDATAPROC)(GLenum, GLintptr, GLsizeiptr, const GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNDELETEBUFFERSPROC)(GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNDELETEQUERIESPROC)(GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNENDQUERYPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNGENBUFFERSPROC)(GLsizei, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGENQUERIESPROC)(GLsizei, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETBUFFERPARAMETERIVPROC)(GLenum, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETBUFFERPOINTERVPROC)(GLenum, GLenum, GLvoid **);
	typedef void (CODEGEN_FUNCPTR *PFNGETBUFFERSUBDATAPROC)(GLenum, GLintptr, GLsizeiptr, GLvoid *);
	typedef void (CODEGEN_FUNCPTR *PFNGETQUERYOBJECTIVPROC)(GLuint, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETQUERYOBJECTUIVPROC)(GLuint, GLenum, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETQUERYIVPROC)(GLenum, GLenum, GLint *);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISBUFFERPROC)(GLuint);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISQUERYPROC)(GLuint);
	typedef void * (CODEGEN_FUNCPTR *PFNMAPBUFFERPROC)(GLenum, GLenum);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNUNMAPBUFFERPROC)(GLenum);
	
	// Extension: 2.0
	typedef void (CODEGEN_FUNCPTR *PFNATTACHSHADERPROC)(GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNBINDATTRIBLOCATIONPROC)(GLuint, GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNBLENDEQUATIONSEPARATEPROC)(GLenum, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNCOMPILESHADERPROC)(GLuint);
	typedef GLuint (CODEGEN_FUNCPTR *PFNCREATEPROGRAMPROC)();
	typedef GLuint (CODEGEN_FUNCPTR *PFNCREATESHADERPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNDELETEPROGRAMPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNDELETESHADERPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNDETACHSHADERPROC)(GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNDISABLEVERTEXATTRIBARRAYPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWBUFFERSPROC)(GLsizei, const GLenum *);
	typedef void (CODEGEN_FUNCPTR *PFNENABLEVERTEXATTRIBARRAYPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEATTRIBPROC)(GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEUNIFORMPROC)(GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETATTACHEDSHADERSPROC)(GLuint, GLsizei, GLsizei *, GLuint *);
	typedef GLint (CODEGEN_FUNCPTR *PFNGETATTRIBLOCATIONPROC)(GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETPROGRAMIVPROC)(GLuint, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSHADERSOURCEPROC)(GLuint, GLsizei, GLsizei *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSHADERIVPROC)(GLuint, GLenum, GLint *);
	typedef GLint (CODEGEN_FUNCPTR *PFNGETUNIFORMLOCATIONPROC)(GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETUNIFORMFVPROC)(GLuint, GLint, GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNGETUNIFORMIVPROC)(GLuint, GLint, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBPOINTERVPROC)(GLuint, GLenum, GLvoid **);
	typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBDVPROC)(GLuint, GLenum, GLdouble *);
	typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBFVPROC)(GLuint, GLenum, GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBIVPROC)(GLuint, GLenum, GLint *);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISPROGRAMPROC)(GLuint);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISSHADERPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNLINKPROGRAMPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar *const*, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNSTENCILFUNCSEPARATEPROC)(GLenum, GLenum, GLint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNSTENCILMASKSEPARATEPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNSTENCILOPSEPARATEPROC)(GLenum, GLenum, GLenum, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1FPROC)(GLint, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1FVPROC)(GLint, GLsizei, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1IPROC)(GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1IVPROC)(GLint, GLsizei, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2FVPROC)(GLint, GLsizei, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2IPROC)(GLint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2IVPROC)(GLint, GLsizei, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3FVPROC)(GLint, GLsizei, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3IPROC)(GLint, GLint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3IVPROC)(GLint, GLsizei, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4FVPROC)(GLint, GLsizei, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4IPROC)(GLint, GLint, GLint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4IVPROC)(GLint, GLsizei, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX2FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX3FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUSEPROGRAMPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVALIDATEPROGRAMPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1DPROC)(GLuint, GLdouble);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1DVPROC)(GLuint, const GLdouble *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1FPROC)(GLuint, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1FVPROC)(GLuint, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1SPROC)(GLuint, GLshort);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1SVPROC)(GLuint, const GLshort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2DPROC)(GLuint, GLdouble, GLdouble);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2DVPROC)(GLuint, const GLdouble *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2FPROC)(GLuint, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2FVPROC)(GLuint, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2SPROC)(GLuint, GLshort, GLshort);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2SVPROC)(GLuint, const GLshort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3DPROC)(GLuint, GLdouble, GLdouble, GLdouble);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3DVPROC)(GLuint, const GLdouble *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3FPROC)(GLuint, GLfloat, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3FVPROC)(GLuint, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3SPROC)(GLuint, GLshort, GLshort, GLshort);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3SVPROC)(GLuint, const GLshort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NBVPROC)(GLuint, const GLbyte *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NIVPROC)(GLuint, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NSVPROC)(GLuint, const GLshort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUBPROC)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUBVPROC)(GLuint, const GLubyte *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUIVPROC)(GLuint, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUSVPROC)(GLuint, const GLushort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4BVPROC)(GLuint, const GLbyte *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4DPROC)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4DVPROC)(GLuint, const GLdouble *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4FPROC)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4FVPROC)(GLuint, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4IVPROC)(GLuint, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4SPROC)(GLuint, GLshort, GLshort, GLshort, GLshort);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4SVPROC)(GLuint, const GLshort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4UBVPROC)(GLuint, const GLubyte *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4UIVPROC)(GLuint, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4USVPROC)(GLuint, const GLushort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
	
	// Extension: 2.1
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX2X3FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX2X4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX3X2FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX3X4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX4X2FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX4X3FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
	
	// Extension: 3.0
	typedef void (CODEGEN_FUNCPTR *PFNBEGINCONDITIONALRENDERPROC)(GLuint, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNBEGINTRANSFORMFEEDBACKPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNBINDBUFFERBASEPROC)(GLenum, GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNBINDBUFFERRANGEPROC)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);
	typedef void (CODEGEN_FUNCPTR *PFNBINDFRAGDATALOCATIONPROC)(GLuint, GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNBINDFRAMEBUFFERPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNBINDRENDERBUFFERPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNBINDVERTEXARRAYPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNBLITFRAMEBUFFERPROC)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
	typedef GLenum (CODEGEN_FUNCPTR *PFNCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNCLAMPCOLORPROC)(GLenum, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARBUFFERFIPROC)(GLenum, GLint, GLfloat, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARBUFFERFVPROC)(GLenum, GLint, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARBUFFERIVPROC)(GLenum, GLint, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNCLEARBUFFERUIVPROC)(GLenum, GLint, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNCOLORMASKIPROC)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean);
	typedef void (CODEGEN_FUNCPTR *PFNDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNDELETERENDERBUFFERSPROC)(GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNDISABLEIPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNENABLEIPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNENDCONDITIONALRENDERPROC)();
	typedef void (CODEGEN_FUNCPTR *PFNENDTRANSFORMFEEDBACKPROC)();
	typedef void (CODEGEN_FUNCPTR *PFNFLUSHMAPPEDBUFFERRANGEPROC)(GLenum, GLintptr, GLsizeiptr);
	typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERRENDERBUFFERPROC)(GLenum, GLenum, GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURE1DPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURE3DPROC)(GLenum, GLenum, GLenum, GLuint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURELAYERPROC)(GLenum, GLenum, GLuint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNGENFRAMEBUFFERSPROC)(GLsizei, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGENRENDERBUFFERSPROC)(GLsizei, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGENVERTEXARRAYSPROC)(GLsizei, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGENERATEMIPMAPPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNGETBOOLEANI_VPROC)(GLenum, GLuint, GLboolean *);
	typedef GLint (CODEGEN_FUNCPTR *PFNGETFRAGDATALOCATIONPROC)(GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)(GLenum, GLenum, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETINTEGERI_VPROC)(GLenum, GLuint, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETRENDERBUFFERPARAMETERIVPROC)(GLenum, GLenum, GLint *);
	typedef const GLubyte * (CODEGEN_FUNCPTR *PFNGETSTRINGIPROC)(GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNGETTEXPARAMETERIIVPROC)(GLenum, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETTEXPARAMETERIUIVPROC)(GLenum, GLenum, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETTRANSFORMFEEDBACKVARYINGPROC)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETUNIFORMUIVPROC)(GLuint, GLint, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBIIVPROC)(GLuint, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBIUIVPROC)(GLuint, GLenum, GLuint *);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISENABLEDIPROC)(GLenum, GLuint);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISFRAMEBUFFERPROC)(GLuint);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISRENDERBUFFERPROC)(GLuint);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISVERTEXARRAYPROC)(GLuint);
	typedef void * (CODEGEN_FUNCPTR *PFNMAPBUFFERRANGEPROC)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
	typedef void (CODEGEN_FUNCPTR *PFNRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNRENDERBUFFERSTORAGEMULTISAMPLEPROC)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERIIVPROC)(GLenum, GLenum, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERIUIVPROC)(GLenum, GLenum, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNTRANSFORMFEEDBACKVARYINGSPROC)(GLuint, GLsizei, const GLchar *const*, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1UIPROC)(GLint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1UIVPROC)(GLint, GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2UIPROC)(GLint, GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2UIVPROC)(GLint, GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3UIPROC)(GLint, GLuint, GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3UIVPROC)(GLint, GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4UIPROC)(GLint, GLuint, GLuint, GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4UIVPROC)(GLint, GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1IPROC)(GLuint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1IVPROC)(GLuint, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1UIPROC)(GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1UIVPROC)(GLuint, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2IPROC)(GLuint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2IVPROC)(GLuint, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2UIPROC)(GLuint, GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2UIVPROC)(GLuint, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3IPROC)(GLuint, GLint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3IVPROC)(GLuint, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3UIPROC)(GLuint, GLuint, GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3UIVPROC)(GLuint, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4BVPROC)(GLuint, const GLbyte *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4IPROC)(GLuint, GLint, GLint, GLint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4IVPROC)(GLuint, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4SVPROC)(GLuint, const GLshort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4UBVPROC)(GLuint, const GLubyte *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4UIPROC)(GLuint, GLuint, GLuint, GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4UIVPROC)(GLuint, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4USVPROC)(GLuint, const GLushort *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBIPOINTERPROC)(GLuint, GLint, GLenum, GLsizei, const GLvoid *);
	
	// Extension: 3.1
	typedef void (CODEGEN_FUNCPTR *PFNCOPYBUFFERSUBDATAPROC)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWARRAYSINSTANCEDPROC)(GLenum, GLint, GLsizei, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWELEMENTSINSTANCEDPROC)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei);
	typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEUNIFORMBLOCKNAMEPROC)(GLuint, GLuint, GLsizei, GLsizei *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEUNIFORMBLOCKIVPROC)(GLuint, GLuint, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEUNIFORMNAMEPROC)(GLuint, GLuint, GLsizei, GLsizei *, GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEUNIFORMSIVPROC)(GLuint, GLsizei, const GLuint *, GLenum, GLint *);
	typedef GLuint (CODEGEN_FUNCPTR *PFNGETUNIFORMBLOCKINDEXPROC)(GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETUNIFORMINDICESPROC)(GLuint, GLsizei, const GLchar *const*, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNPRIMITIVERESTARTINDEXPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNTEXBUFFERPROC)(GLenum, GLenum, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNUNIFORMBLOCKBINDINGPROC)(GLuint, GLuint, GLuint);
	
	// Extension: 3.2
	typedef GLenum (CODEGEN_FUNCPTR *PFNCLIENTWAITSYNCPROC)(GLsync, GLbitfield, GLuint64);
	typedef void (CODEGEN_FUNCPTR *PFNDELETESYNCPROC)(GLsync);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWELEMENTSBASEVERTEXPROC)(GLenum, GLsizei, GLenum, const GLvoid *, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWELEMENTSINSTANCEDBASEVERTEXPROC)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNDRAWRANGEELEMENTSBASEVERTEXPROC)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *, GLint);
	typedef GLsync (CODEGEN_FUNCPTR *PFNFENCESYNCPROC)(GLenum, GLbitfield);
	typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTUREPROC)(GLenum, GLenum, GLuint, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNGETBUFFERPARAMETERI64VPROC)(GLenum, GLenum, GLint64 *);
	typedef void (CODEGEN_FUNCPTR *PFNGETINTEGER64I_VPROC)(GLenum, GLuint, GLint64 *);
	typedef void (CODEGEN_FUNCPTR *PFNGETINTEGER64VPROC)(GLenum, GLint64 *);
	typedef void (CODEGEN_FUNCPTR *PFNGETMULTISAMPLEFVPROC)(GLenum, GLuint, GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSYNCIVPROC)(GLsync, GLenum, GLsizei, GLsizei *, GLint *);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISSYNCPROC)(GLsync);
	typedef void (CODEGEN_FUNCPTR *PFNMULTIDRAWELEMENTSBASEVERTEXPROC)(GLenum, const GLsizei *, GLenum, const GLvoid *const*, GLsizei, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNPROVOKINGVERTEXPROC)(GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLEMASKIPROC)(GLuint, GLbitfield);
	typedef void (CODEGEN_FUNCPTR *PFNTEXIMAGE2DMULTISAMPLEPROC)(GLenum, GLsizei, GLint, GLsizei, GLsizei, GLboolean);
	typedef void (CODEGEN_FUNCPTR *PFNTEXIMAGE3DMULTISAMPLEPROC)(GLenum, GLsizei, GLint, GLsizei, GLsizei, GLsizei, GLboolean);
	typedef void (CODEGEN_FUNCPTR *PFNWAITSYNCPROC)(GLsync, GLbitfield, GLuint64);
	
	// Extension: 3.3
	typedef void (CODEGEN_FUNCPTR *PFNBINDFRAGDATALOCATIONINDEXEDPROC)(GLuint, GLuint, GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNBINDSAMPLERPROC)(GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNDELETESAMPLERSPROC)(GLsizei, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGENSAMPLERSPROC)(GLsizei, GLuint *);
	typedef GLint (CODEGEN_FUNCPTR *PFNGETFRAGDATAINDEXPROC)(GLuint, const GLchar *);
	typedef void (CODEGEN_FUNCPTR *PFNGETQUERYOBJECTI64VPROC)(GLuint, GLenum, GLint64 *);
	typedef void (CODEGEN_FUNCPTR *PFNGETQUERYOBJECTUI64VPROC)(GLuint, GLenum, GLuint64 *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSAMPLERPARAMETERIIVPROC)(GLuint, GLenum, GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSAMPLERPARAMETERIUIVPROC)(GLuint, GLenum, GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSAMPLERPARAMETERFVPROC)(GLuint, GLenum, GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNGETSAMPLERPARAMETERIVPROC)(GLuint, GLenum, GLint *);
	typedef GLboolean (CODEGEN_FUNCPTR *PFNISSAMPLERPROC)(GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNQUERYCOUNTERPROC)(GLuint, GLenum);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLERPARAMETERIIVPROC)(GLuint, GLenum, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLERPARAMETERIUIVPROC)(GLuint, GLenum, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLERPARAMETERFPROC)(GLuint, GLenum, GLfloat);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLERPARAMETERFVPROC)(GLuint, GLenum, const GLfloat *);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLERPARAMETERIPROC)(GLuint, GLenum, GLint);
	typedef void (CODEGEN_FUNCPTR *PFNSAMPLERPARAMETERIVPROC)(GLuint, GLenum, const GLint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBDIVISORPROC)(GLuint, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP1UIPROC)(GLuint, GLenum, GLboolean, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP1UIVPROC)(GLuint, GLenum, GLboolean, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP2UIPROC)(GLuint, GLenum, GLboolean, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP2UIVPROC)(GLuint, GLenum, GLboolean, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP3UIPROC)(GLuint, GLenum, GLboolean, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP3UIVPROC)(GLuint, GLenum, GLboolean, const GLuint *);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP4UIPROC)(GLuint, GLenum, GLboolean, GLuint);
	typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBP4UIVPROC)(GLuint, GLenum, GLboolean, const GLuint *);
	
	
	// Extension: 1.0
	PFNBLENDFUNCPROC BlendFunc;
	PFNCLEARPROC Clear;
	PFNCLEARCOLORPROC ClearColor;
	PFNCLEARDEPTHPROC ClearDepth;
	PFNCLEARSTENCILPROC ClearStencil;
	PFNCOLORMASKPROC ColorMask;
	PFNCULLFACEPROC CullFace;
	PFNDEPTHFUNCPROC DepthFunc;
	PFNDEPTHMASKPROC DepthMask;
	PFNDEPTHRANGEPROC DepthRange;
	PFNDISABLEPROC Disable;
	PFNDRAWBUFFERPROC DrawBuffer;
	PFNENABLEPROC Enable;
	PFNFINISHPROC Finish;
	PFNFLUSHPROC Flush;
	PFNFRONTFACEPROC FrontFace;
	PFNGETBOOLEANVPROC GetBooleanv;
	PFNGETDOUBLEVPROC GetDoublev;
	PFNGETERRORPROC GetError;
	PFNGETFLOATVPROC GetFloatv;
	PFNGETINTEGERVPROC GetIntegerv;
	PFNGETSTRINGPROC GetString;
	PFNGETTEXIMAGEPROC GetTexImage;
	PFNGETTEXLEVELPARAMETERFVPROC GetTexLevelParameterfv;
	PFNGETTEXLEVELPARAMETERIVPROC GetTexLevelParameteriv;
	PFNGETTEXPARAMETERFVPROC GetTexParameterfv;
	PFNGETTEXPARAMETERIVPROC GetTexParameteriv;
	PFNHINTPROC Hint;
	PFNISENABLEDPROC IsEnabled;
	PFNLINEWIDTHPROC LineWidth;
	PFNLOGICOPPROC LogicOp;
	PFNPIXELSTOREFPROC PixelStoref;
	PFNPIXELSTOREIPROC PixelStorei;
	PFNPOINTSIZEPROC PointSize;
	PFNPOLYGONMODEPROC PolygonMode;
	PFNREADBUFFERPROC ReadBuffer;
	PFNREADPIXELSPROC ReadPixels;
	PFNSCISSORPROC Scissor;
	PFNSTENCILFUNCPROC StencilFunc;
	PFNSTENCILMASKPROC StencilMask;
	PFNSTENCILOPPROC StencilOp;
	PFNTEXIMAGE1DPROC TexImage1D;
	PFNTEXIMAGE2DPROC TexImage2D;
	PFNTEXPARAMETERFPROC TexParameterf;
	PFNTEXPARAMETERFVPROC TexParameterfv;
	PFNTEXPARAMETERIPROC TexParameteri;
	PFNTEXPARAMETERIVPROC TexParameteriv;
	PFNVIEWPORTPROC Viewport;
	
	// Extension: 1.1
	PFNBINDTEXTUREPROC BindTexture;
	PFNCOPYTEXIMAGE1DPROC CopyTexImage1D;
	PFNCOPYTEXIMAGE2DPROC CopyTexImage2D;
	PFNCOPYTEXSUBIMAGE1DPROC CopyTexSubImage1D;
	PFNCOPYTEXSUBIMAGE2DPROC CopyTexSubImage2D;
	PFNDELETETEXTURESPROC DeleteTextures;
	PFNDRAWARRAYSPROC DrawArrays;
	PFNDRAWELEMENTSPROC DrawElements;
	PFNGENTEXTURESPROC GenTextures;
	PFNISTEXTUREPROC IsTexture;
	PFNPOLYGONOFFSETPROC PolygonOffset;
	PFNTEXSUBIMAGE1DPROC TexSubImage1D;
	PFNTEXSUBIMAGE2DPROC TexSubImage2D;
	
	// Extension: 1.2
	PFNBLENDCOLORPROC BlendColor;
	PFNBLENDEQUATIONPROC BlendEquation;
	PFNCOPYTEXSUBIMAGE3DPROC CopyTexSubImage3D;
	PFNDRAWRANGEELEMENTSPROC DrawRangeElements;
	PFNTEXIMAGE3DPROC TexImage3D;
	PFNTEXSUBIMAGE3DPROC TexSubImage3D;
	
	// Extension: 1.3
	PFNACTIVETEXTUREPROC ActiveTexture;
	PFNCOMPRESSEDTEXIMAGE1DPROC CompressedTexImage1D;
	PFNCOMPRESSEDTEXIMAGE2DPROC CompressedTexImage2D;
	PFNCOMPRESSEDTEXIMAGE3DPROC CompressedTexImage3D;
	PFNCOMPRESSEDTEXSUBIMAGE1DPROC CompressedTexSubImage1D;
	PFNCOMPRESSEDTEXSUBIMAGE2DPROC CompressedTexSubImage2D;
	PFNCOMPRESSEDTEXSUBIMAGE3DPROC CompressedTexSubImage3D;
	PFNGETCOMPRESSEDTEXIMAGEPROC GetCompressedTexImage;
	PFNSAMPLECOVERAGEPROC SampleCoverage;
	
	// Extension: 1.4
	PFNBLENDFUNCSEPARATEPROC BlendFuncSeparate;
	PFNMULTIDRAWARRAYSPROC MultiDrawArrays;
	PFNMULTIDRAWELEMENTSPROC MultiDrawElements;
	PFNPOINTPARAMETERFPROC PointParameterf;
	PFNPOINTPARAMETERFVPROC PointParameterfv;
	PFNPOINTPARAMETERIPROC PointParameteri;
	PFNPOINTPARAMETERIVPROC PointParameteriv;
	
	// Extension: 1.5
	PFNBEGINQUERYPROC BeginQuery;
	PFNBINDBUFFERPROC BindBuffer;
	PFNBUFFERDATAPROC BufferData;
	PFNBUFFERSUBDATAPROC BufferSubData;
	PFNDELETEBUFFERSPROC DeleteBuffers;
	PFNDELETEQUERIESPROC DeleteQueries;
	PFNENDQUERYPROC EndQuery;
	PFNGENBUFFERSPROC GenBuffers;
	PFNGENQUERIESPROC GenQueries;
	PFNGETBUFFERPARAMETERIVPROC GetBufferParameteriv;
	PFNGETBUFFERPOINTERVPROC GetBufferPointerv;
	PFNGETBUFFERSUBDATAPROC GetBufferSubData;
	PFNGETQUERYOBJECTIVPROC GetQueryObjectiv;
	PFNGETQUERYOBJECTUIVPROC GetQueryObjectuiv;
	PFNGETQUERYIVPROC GetQueryiv;
	PFNISBUFFERPROC IsBuffer;
	PFNISQUERYPROC IsQuery;
	PFNMAPBUFFERPROC MapBuffer;
	PFNUNMAPBUFFERPROC UnmapBuffer;
	
	// Extension: 2.0
	PFNATTACHSHADERPROC AttachShader;
	PFNBINDATTRIBLOCATIONPROC BindAttribLocation;
	PFNBLENDEQUATIONSEPARATEPROC BlendEquationSeparate;
	PFNCOMPILESHADERPROC CompileShader;
	PFNCREATEPROGRAMPROC CreateProgram;
	PFNCREATESHADERPROC CreateShader;
	PFNDELETEPROGRAMPROC DeleteProgram;
	PFNDELETESHADERPROC DeleteShader;
	PFNDETACHSHADERPROC DetachShader;
	PFNDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;
	PFNDRAWBUFFERSPROC DrawBuffers;
	PFNENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
	PFNGETACTIVEATTRIBPROC GetActiveAttrib;
	PFNGETACTIVEUNIFORMPROC GetActiveUniform;
	PFNGETATTACHEDSHADERSPROC GetAttachedShaders;
	PFNGETATTRIBLOCATIONPROC GetAttribLocation;
	PFNGETPROGRAMINFOLOGPROC GetProgramInfoLog;
	PFNGETPROGRAMIVPROC GetProgramiv;
	PFNGETSHADERINFOLOGPROC GetShaderInfoLog;
	PFNGETSHADERSOURCEPROC GetShaderSource;
	PFNGETSHADERIVPROC GetShaderiv;
	PFNGETUNIFORMLOCATIONPROC GetUniformLocation;
	PFNGETUNIFORMFVPROC GetUniformfv;
	PFNGETUNIFORMIVPROC GetUniformiv;
	PFNGETVERTEXATTRIBPOINTERVPROC GetVertexAttribPointerv;
	PFNGETVERTEXATTRIBDVPROC GetVertexAttribdv;
	PFNGETVERTEXATTRIBFVPROC GetVertexAttribfv;
	PFNGETVERTEXATTRIBIVPROC GetVertexAttribiv;
	PFNISPROGRAMPROC IsProgram;
	PFNISSHADERPROC IsShader;
	PFNLINKPROGRAMPROC LinkProgram;
	PFNSHADERSOURCEPROC ShaderSource;
	PFNSTENCILFUNCSEPARATEPROC StencilFuncSeparate;
	PFNSTENCILMASKSEPARATEPROC StencilMaskSeparate;
	PFNSTENCILOPSEPARATEPROC StencilOpSeparate;
	PFNUNIFORM1FPROC Uniform1f;
	PFNUNIFORM1FVPROC Uniform1fv;
	PFNUNIFORM1IPROC Uniform1i;
	PFNUNIFORM1IVPROC Uniform1iv;
	PFNUNIFORM2FPROC Uniform2f;
	PFNUNIFORM2FVPROC Uniform2fv;
	PFNUNIFORM2IPROC Uniform2i;
	PFNUNIFORM2IVPROC Uniform2iv;
	PFNUNIFORM3FPROC Uniform3f;
	PFNUNIFORM3FVPROC Uniform3fv;
	PFNUNIFORM3IPROC Uniform3i;
	PFNUNIFORM3IVPROC Uniform3iv;
	PFNUNIFORM4FPROC Uniform4f;
	PFNUNIFORM4FVPROC Uniform4fv;
	PFNUNIFORM4IPROC Uniform4i;
	PFNUNIFORM4IVPROC Uniform4iv;
	PFNUNIFORMMATRIX2FVPROC UniformMatrix2fv;
	PFNUNIFORMMATRIX3FVPROC UniformMatrix3fv;
	PFNUNIFORMMATRIX4FVPROC UniformMatrix4fv;
	PFNUSEPROGRAMPROC UseProgram;
	PFNVALIDATEPROGRAMPROC ValidateProgram;
	PFNVERTEXATTRIB1DPROC VertexAttrib1d;
	PFNVERTEXATTRIB1DVPROC VertexAttrib1dv;
	PFNVERTEXATTRIB1FPROC VertexAttrib1f;
	PFNVERTEXATTRIB1FVPROC VertexAttrib1fv;
	PFNVERTEXATTRIB1SPROC VertexAttrib1s;
	PFNVERTEXATTRIB1SVPROC VertexAttrib1sv;
	PFNVERTEXATTRIB2DPROC VertexAttrib2d;
	PFNVERTEXATTRIB2DVPROC VertexAttrib2dv;
	PFNVERTEXATTRIB2FPROC VertexAttrib2f;
	PFNVERTEXATTRIB2FVPROC VertexAttrib2fv;
	PFNVERTEXATTRIB2SPROC VertexAttrib2s;
	PFNVERTEXATTRIB2SVPROC VertexAttrib2sv;
	PFNVERTEXATTRIB3DPROC VertexAttrib3d;
	PFNVERTEXATTRIB3DVPROC VertexAttrib3dv;
	PFNVERTEXATTRIB3FPROC VertexAttrib3f;
	PFNVERTEXATTRIB3FVPROC VertexAttrib3fv;
	PFNVERTEXATTRIB3SPROC VertexAttrib3s;
	PFNVERTEXATTRIB3SVPROC VertexAttrib3sv;
	PFNVERTEXATTRIB4NBVPROC VertexAttrib4Nbv;
	PFNVERTEXATTRIB4NIVPROC VertexAttrib4Niv;
	PFNVERTEXATTRIB4NSVPROC VertexAttrib4Nsv;
	PFNVERTEXATTRIB4NUBPROC VertexAttrib4Nub;
	PFNVERTEXATTRIB4NUBVPROC VertexAttrib4Nubv;
	PFNVERTEXATTRIB4NUIVPROC VertexAttrib4Nuiv;
	PFNVERTEXATTRIB4NUSVPROC VertexAttrib4Nusv;
	PFNVERTEXATTRIB4BVPROC VertexAttrib4bv;
	PFNVERTEXATTRIB4DPROC VertexAttrib4d;
	PFNVERTEXATTRIB4DVPROC VertexAttrib4dv;
	PFNVERTEXATTRIB4FPROC VertexAttrib4f;
	PFNVERTEXATTRIB4FVPROC VertexAttrib4fv;
	PFNVERTEXATTRIB4IVPROC VertexAttrib4iv;
	PFNVERTEXATTRIB4SPROC VertexAttrib4s;
	PFNVERTEXATTRIB4SVPROC VertexAttrib4sv;
	PFNVERTEXATTRIB4UBVPROC VertexAttrib4ubv;
	PFNVERTEXATTRIB4UIVPROC VertexAttrib4uiv;
	PFNVERTEXATTRIB4USVPROC VertexAttrib4usv;
	PFNVERTEXATTRIBPOINTERPROC VertexAttribPointer;
	
	// Extension: 2.1
	PFNUNIFORMMATRIX2X3FVPROC UniformMatrix2x3fv;
	PFNUNIFORMMATRIX2X4FVPROC UniformMatrix2x4fv;
	PFNUNIFORMMATRIX3X2FVPROC UniformMatrix3x2fv;
	PFNUNIFORMMATRIX3X4FVPROC UniformMatrix3x4fv;
	PFNUNIFORMMATRIX4X2FVPROC UniformMatrix4x2fv;
	PFNUNIFORMMATRIX4X3FVPROC UniformMatrix4x3fv;
	
	// Extension: 3.0
	PFNBEGINCONDITIONALRENDERPROC BeginConditionalRender;
	PFNBEGINTRANSFORMFEEDBACKPROC BeginTransformFeedback;
	PFNBINDBUFFERBASEPROC BindBufferBase;
	PFNBINDBUFFERRANGEPROC BindBufferRange;
	PFNBINDFRAGDATALOCATIONPROC BindFragDataLocation;
	PFNBINDFRAMEBUFFERPROC BindFramebuffer;
	PFNBINDRENDERBUFFERPROC BindRenderbuffer;
	PFNBINDVERTEXARRAYPROC BindVertexArray;
	PFNBLITFRAMEBUFFERPROC BlitFramebuffer;
	PFNCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus;
	PFNCLAMPCOLORPROC ClampColor;
	PFNCLEARBUFFERFIPROC ClearBufferfi;
	PFNCLEARBUFFERFVPROC ClearBufferfv;
	PFNCLEARBUFFERIVPROC ClearBufferiv;
	PFNCLEARBUFFERUIVPROC ClearBufferuiv;
	PFNCOLORMASKIPROC ColorMaski;
	PFNDELETEFRAMEBUFFERSPROC DeleteFramebuffers;
	PFNDELETERENDERBUFFERSPROC DeleteRenderbuffers;
	PFNDELETEVERTEXARRAYSPROC DeleteVertexArrays;
	PFNDISABLEIPROC Disablei;
	PFNENABLEIPROC Enablei;
	PFNENDCONDITIONALRENDERPROC EndConditionalRender;
	PFNENDTRANSFORMFEEDBACKPROC EndTransformFeedback;
	PFNFLUSHMAPPEDBUFFERRANGEPROC FlushMappedBufferRange;
	PFNFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer;
	PFNFRAMEBUFFERTEXTURE1DPROC FramebufferTexture1D;
	PFNFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D;
	PFNFRAMEBUFFERTEXTURE3DPROC FramebufferTexture3D;
	PFNFRAMEBUFFERTEXTURELAYERPROC FramebufferTextureLayer;
	PFNGENFRAMEBUFFERSPROC GenFramebuffers;
	PFNGENRENDERBUFFERSPROC GenRenderbuffers;
	PFNGENVERTEXARRAYSPROC GenVertexArrays;
	PFNGENERATEMIPMAPPROC GenerateMipmap;
	PFNGETBOOLEANI_VPROC GetBooleani_v;
	PFNGETFRAGDATALOCATIONPROC GetFragDataLocation;
	PFNGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC GetFramebufferAttachmentParameteriv;
	PFNGETINTEGERI_VPROC GetIntegeri_v;
	PFNGETRENDERBUFFERPARAMETERIVPROC GetRenderbufferParameteriv;
	PFNGETSTRINGIPROC GetStringi;
	PFNGETTEXPARAMETERIIVPROC GetTexParameterIiv;
	PFNGETTEXPARAMETERIUIVPROC GetTexParameterIuiv;
	PFNGETTRANSFORMFEEDBACKVARYINGPROC GetTransformFeedbackVarying;
	PFNGETUNIFORMUIVPROC GetUniformuiv;
	PFNGETVERTEXATTRIBIIVPROC GetVertexAttribIiv;
	PFNGETVERTEXATTRIBIUIVPROC GetVertexAttribIuiv;
	PFNISENABLEDIPROC IsEnabledi;
	PFNISFRAMEBUFFERPROC IsFramebuffer;
	PFNISRENDERBUFFERPROC IsRenderbuffer;
	PFNISVERTEXARRAYPROC IsVertexArray;
	PFNMAPBUFFERRANGEPROC MapBufferRange;
	PFNRENDERBUFFERSTORAGEPROC RenderbufferStorage;
	PFNRENDERBUFFERSTORAGEMULTISAMPLEPROC RenderbufferStorageMultisample;
	PFNTEXPARAMETERIIVPROC TexParameterIiv;
	PFNTEXPARAMETERIUIVPROC TexParameterIuiv;
	PFNTRANSFORMFEEDBACKVARYINGSPROC TransformFeedbackVaryings;
	PFNUNIFORM1UIPROC Uniform1ui;
	PFNUNIFORM1UIVPROC Uniform1uiv;
	PFNUNIFORM2UIPROC Uniform2ui;
	PFNUNIFORM2UIVPROC Uniform2uiv;
	PFNUNIFORM3UIPROC Uniform3ui;
	PFNUNIFORM3UIVPROC Uniform3uiv;
	PFNUNIFORM4UIPROC Uniform4ui;
	PFNUNIFORM4UIVPROC Uniform4uiv;
	PFNVERTEXATTRIBI1IPROC VertexAttribI1i;
	PFNVERTEXATTRIBI1IVPROC VertexAttribI1iv;
	PFNVERTEXATTRIBI1UIPROC VertexAttribI1ui;
	PFNVERTEXATTRIBI1UIVPROC VertexAttribI1uiv;
	PFNVERTEXATTRIBI2IPROC VertexAttribI2i;
	PFNVERTEXATTRIBI2IVPROC VertexAttribI2iv;
	PFNVERTEXATTRIBI2UIPROC VertexAttribI2ui;
	PFNVERTEXATTRIBI2UIVPROC VertexAttribI2uiv;
	PFNVERTEXATTRIBI3IPROC VertexAttribI3i;
	PFNVERTEXATTRIBI3IVPROC VertexAttribI3iv;
	PFNVERTEXATTRIBI3UIPROC VertexAttribI3ui;
	PFNVERTEXATTRIBI3UIVPROC VertexAttribI3uiv;
	PFNVERTEXATTRIBI4BVPROC VertexAttribI4bv;
	PFNVERTEXATTRIBI4IPROC VertexAttribI4i;
	PFNVERTEXATTRIBI4IVPROC VertexAttribI4iv;
	PFNVERTEXATTRIBI4SVPROC VertexAttribI4sv;
	PFNVERTEXATTRIBI4UBVPROC VertexAttribI4ubv;
	PFNVERTEXATTRIBI4UIPROC VertexAttribI4ui;
	PFNVERTEXATTRIBI4UIVPROC VertexAttribI4uiv;
	PFNVERTEXATTRIBI4USVPROC VertexAttribI4usv;
	PFNVERTEXATTRIBIPOINTERPROC VertexAttribIPointer;
	
	// Extension: 3.1
	PFNCOPYBUFFERSUBDATAPROC CopyBufferSubData;
	PFNDRAWARRAYSINSTANCEDPROC DrawArraysInstanced;
	PFNDRAWELEMENTSINSTANCEDPROC DrawElementsInstanced;
	PFNGETACTIVEUNIFORMBLOCKNAMEPROC GetActiveUniformBlockName;
	PFNGETACTIVEUNIFORMBLOCKIVPROC GetActiveUniformBlockiv;
	PFNGETACTIVEUNIFORMNAMEPROC GetActiveUniformName;
	PFNGETACTIVEUNIFORMSIVPROC GetActiveUniformsiv;
	PFNGETUNIFORMBLOCKINDEXPROC GetUniformBlockIndex;
	PFNGETUNIFORMINDICESPROC GetUniformIndices;
	PFNPRIMITIVERESTARTINDEXPROC PrimitiveRestartIndex;
	PFNTEXBUFFERPROC TexBuffer;
	PFNUNIFORMBLOCKBINDINGPROC UniformBlockBinding;
	
	// Extension: 3.2
	PFNCLIENTWAITSYNCPROC ClientWaitSync;
	PFNDELETESYNCPROC DeleteSync;
	PFNDRAWELEMENTSBASEVERTEXPROC DrawElementsBaseVertex;
	PFNDRAWELEMENTSINSTANCEDBASEVERTEXPROC DrawElementsInstancedBaseVertex;
	PFNDRAWRANGEELEMENTSBASEVERTEXPROC DrawRangeElementsBaseVertex;
	PFNFENCESYNCPROC FenceSync;
	PFNFRAMEBUFFERTEXTUREPROC FramebufferTexture;
	PFNGETBUFFERPARAMETERI64VPROC GetBufferParameteri64v;
	PFNGETINTEGER64I_VPROC GetInteger64i_v;
	PFNGETINTEGER64VPROC GetInteger64v;
	PFNGETMULTISAMPLEFVPROC GetMultisamplefv;
	PFNGETSYNCIVPROC GetSynciv;
	PFNISSYNCPROC IsSync;
	PFNMULTIDRAWELEMENTSBASEVERTEXPROC MultiDrawElementsBaseVertex;
	PFNPROVOKINGVERTEXPROC ProvokingVertex;
	PFNSAMPLEMASKIPROC SampleMaski;
	PFNTEXIMAGE2DMULTISAMPLEPROC TexImage2DMultisample;
	PFNTEXIMAGE3DMULTISAMPLEPROC TexImage3DMultisample;
	PFNWAITSYNCPROC WaitSync;
	
	// Extension: 3.3
	PFNBINDFRAGDATALOCATIONINDEXEDPROC BindFragDataLocationIndexed;
	PFNBINDSAMPLERPROC BindSampler;
	PFNDELETESAMPLERSPROC DeleteSamplers;
	PFNGENSAMPLERSPROC GenSamplers;
	PFNGETFRAGDATAINDEXPROC GetFragDataIndex;
	PFNGETQUERYOBJECTI64VPROC GetQueryObjecti64v;
	PFNGETQUERYOBJECTUI64VPROC GetQueryObjectui64v;
	PFNGETSAMPLERPARAMETERIIVPROC GetSamplerParameterIiv;
	PFNGETSAMPLERPARAMETERIUIVPROC GetSamplerParameterIuiv;
	PFNGETSAMPLERPARAMETERFVPROC GetSamplerParameterfv;
	PFNGETSAMPLERPARAMETERIVPROC GetSamplerParameteriv;
	PFNISSAMPLERPROC IsSampler;
	PFNQUERYCOUNTERPROC QueryCounter;
	PFNSAMPLERPARAMETERIIVPROC SamplerParameterIiv;
	PFNSAMPLERPARAMETERIUIVPROC SamplerParameterIuiv;
	PFNSAMPLERPARAMETERFPROC SamplerParameterf;
	PFNSAMPLERPARAMETERFVPROC SamplerParameterfv;
	PFNSAMPLERPARAMETERIPROC SamplerParameteri;
	PFNSAMPLERPARAMETERIVPROC SamplerParameteriv;
	PFNVERTEXATTRIBDIVISORPROC VertexAttribDivisor;
	PFNVERTEXATTRIBP1UIPROC VertexAttribP1ui;
	PFNVERTEXATTRIBP1UIVPROC VertexAttribP1uiv;
	PFNVERTEXATTRIBP2UIPROC VertexAttribP2ui;
	PFNVERTEXATTRIBP2UIVPROC VertexAttribP2uiv;
	PFNVERTEXATTRIBP3UIPROC VertexAttribP3ui;
	PFNVERTEXATTRIBP3UIVPROC VertexAttribP3uiv;
	PFNVERTEXATTRIBP4UIPROC VertexAttribP4ui;
	PFNVERTEXATTRIBP4UIVPROC VertexAttribP4uiv;
	
	
	// Extension: 1.0
	static void CODEGEN_FUNCPTR Switch_BlendFunc(GLenum sfactor, GLenum dfactor)
	{
		BlendFunc = (PFNBLENDFUNCPROC)IntGetProcAddress("glBlendFunc");
		BlendFunc(sfactor, dfactor);
	}

	static void CODEGEN_FUNCPTR Switch_Clear(GLbitfield mask)
	{
		Clear = (PFNCLEARPROC)IntGetProcAddress("glClear");
		Clear(mask);
	}

	static void CODEGEN_FUNCPTR Switch_ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	{
		ClearColor = (PFNCLEARCOLORPROC)IntGetProcAddress("glClearColor");
		ClearColor(red, green, blue, alpha);
	}

	static void CODEGEN_FUNCPTR Switch_ClearDepth(GLdouble depth)
	{
		ClearDepth = (PFNCLEARDEPTHPROC)IntGetProcAddress("glClearDepth");
		ClearDepth(depth);
	}

	static void CODEGEN_FUNCPTR Switch_ClearStencil(GLint s)
	{
		ClearStencil = (PFNCLEARSTENCILPROC)IntGetProcAddress("glClearStencil");
		ClearStencil(s);
	}

	static void CODEGEN_FUNCPTR Switch_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
	{
		ColorMask = (PFNCOLORMASKPROC)IntGetProcAddress("glColorMask");
		ColorMask(red, green, blue, alpha);
	}

	static void CODEGEN_FUNCPTR Switch_CullFace(GLenum mode)
	{
		CullFace = (PFNCULLFACEPROC)IntGetProcAddress("glCullFace");
		CullFace(mode);
	}

	static void CODEGEN_FUNCPTR Switch_DepthFunc(GLenum func)
	{
		DepthFunc = (PFNDEPTHFUNCPROC)IntGetProcAddress("glDepthFunc");
		DepthFunc(func);
	}

	static void CODEGEN_FUNCPTR Switch_DepthMask(GLboolean flag)
	{
		DepthMask = (PFNDEPTHMASKPROC)IntGetProcAddress("glDepthMask");
		DepthMask(flag);
	}

	static void CODEGEN_FUNCPTR Switch_DepthRange(GLdouble ren_near, GLdouble ren_far)
	{
		DepthRange = (PFNDEPTHRANGEPROC)IntGetProcAddress("glDepthRange");
		DepthRange(ren_near, ren_far);
	}

	static void CODEGEN_FUNCPTR Switch_Disable(GLenum cap)
	{
		Disable = (PFNDISABLEPROC)IntGetProcAddress("glDisable");
		Disable(cap);
	}

	static void CODEGEN_FUNCPTR Switch_DrawBuffer(GLenum mode)
	{
		DrawBuffer = (PFNDRAWBUFFERPROC)IntGetProcAddress("glDrawBuffer");
		DrawBuffer(mode);
	}

	static void CODEGEN_FUNCPTR Switch_Enable(GLenum cap)
	{
		Enable = (PFNENABLEPROC)IntGetProcAddress("glEnable");
		Enable(cap);
	}

	static void CODEGEN_FUNCPTR Switch_Finish()
	{
		Finish = (PFNFINISHPROC)IntGetProcAddress("glFinish");
		Finish();
	}

	static void CODEGEN_FUNCPTR Switch_Flush()
	{
		Flush = (PFNFLUSHPROC)IntGetProcAddress("glFlush");
		Flush();
	}

	static void CODEGEN_FUNCPTR Switch_FrontFace(GLenum mode)
	{
		FrontFace = (PFNFRONTFACEPROC)IntGetProcAddress("glFrontFace");
		FrontFace(mode);
	}

	static void CODEGEN_FUNCPTR Switch_GetBooleanv(GLenum pname, GLboolean * params)
	{
		GetBooleanv = (PFNGETBOOLEANVPROC)IntGetProcAddress("glGetBooleanv");
		GetBooleanv(pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetDoublev(GLenum pname, GLdouble * params)
	{
		GetDoublev = (PFNGETDOUBLEVPROC)IntGetProcAddress("glGetDoublev");
		GetDoublev(pname, params);
	}

	static GLenum CODEGEN_FUNCPTR Switch_GetError()
	{
		GetError = (PFNGETERRORPROC)IntGetProcAddress("glGetError");
		return GetError();
	}

	static void CODEGEN_FUNCPTR Switch_GetFloatv(GLenum pname, GLfloat * params)
	{
		GetFloatv = (PFNGETFLOATVPROC)IntGetProcAddress("glGetFloatv");
		GetFloatv(pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetIntegerv(GLenum pname, GLint * params)
	{
		GetIntegerv = (PFNGETINTEGERVPROC)IntGetProcAddress("glGetIntegerv");
		GetIntegerv(pname, params);
	}

	static const GLubyte * CODEGEN_FUNCPTR Switch_GetString(GLenum name)
	{
		GetString = (PFNGETSTRINGPROC)IntGetProcAddress("glGetString");
		return GetString(name);
	}

	static void CODEGEN_FUNCPTR Switch_GetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels)
	{
		GetTexImage = (PFNGETTEXIMAGEPROC)IntGetProcAddress("glGetTexImage");
		GetTexImage(target, level, format, type, pixels);
	}

	static void CODEGEN_FUNCPTR Switch_GetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params)
	{
		GetTexLevelParameterfv = (PFNGETTEXLEVELPARAMETERFVPROC)IntGetProcAddress("glGetTexLevelParameterfv");
		GetTexLevelParameterfv(target, level, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params)
	{
		GetTexLevelParameteriv = (PFNGETTEXLEVELPARAMETERIVPROC)IntGetProcAddress("glGetTexLevelParameteriv");
		GetTexLevelParameteriv(target, level, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetTexParameterfv(GLenum target, GLenum pname, GLfloat * params)
	{
		GetTexParameterfv = (PFNGETTEXPARAMETERFVPROC)IntGetProcAddress("glGetTexParameterfv");
		GetTexParameterfv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetTexParameteriv(GLenum target, GLenum pname, GLint * params)
	{
		GetTexParameteriv = (PFNGETTEXPARAMETERIVPROC)IntGetProcAddress("glGetTexParameteriv");
		GetTexParameteriv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_Hint(GLenum target, GLenum mode)
	{
		Hint = (PFNHINTPROC)IntGetProcAddress("glHint");
		Hint(target, mode);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsEnabled(GLenum cap)
	{
		IsEnabled = (PFNISENABLEDPROC)IntGetProcAddress("glIsEnabled");
		return IsEnabled(cap);
	}

	static void CODEGEN_FUNCPTR Switch_LineWidth(GLfloat width)
	{
		LineWidth = (PFNLINEWIDTHPROC)IntGetProcAddress("glLineWidth");
		LineWidth(width);
	}

	static void CODEGEN_FUNCPTR Switch_LogicOp(GLenum opcode)
	{
		LogicOp = (PFNLOGICOPPROC)IntGetProcAddress("glLogicOp");
		LogicOp(opcode);
	}

	static void CODEGEN_FUNCPTR Switch_PixelStoref(GLenum pname, GLfloat param)
	{
		PixelStoref = (PFNPIXELSTOREFPROC)IntGetProcAddress("glPixelStoref");
		PixelStoref(pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_PixelStorei(GLenum pname, GLint param)
	{
		PixelStorei = (PFNPIXELSTOREIPROC)IntGetProcAddress("glPixelStorei");
		PixelStorei(pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_PointSize(GLfloat size)
	{
		PointSize = (PFNPOINTSIZEPROC)IntGetProcAddress("glPointSize");
		PointSize(size);
	}

	static void CODEGEN_FUNCPTR Switch_PolygonMode(GLenum face, GLenum mode)
	{
		PolygonMode = (PFNPOLYGONMODEPROC)IntGetProcAddress("glPolygonMode");
		PolygonMode(face, mode);
	}

	static void CODEGEN_FUNCPTR Switch_ReadBuffer(GLenum mode)
	{
		ReadBuffer = (PFNREADBUFFERPROC)IntGetProcAddress("glReadBuffer");
		ReadBuffer(mode);
	}

	static void CODEGEN_FUNCPTR Switch_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
	{
		ReadPixels = (PFNREADPIXELSPROC)IntGetProcAddress("glReadPixels");
		ReadPixels(x, y, width, height, format, type, pixels);
	}

	static void CODEGEN_FUNCPTR Switch_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		Scissor = (PFNSCISSORPROC)IntGetProcAddress("glScissor");
		Scissor(x, y, width, height);
	}

	static void CODEGEN_FUNCPTR Switch_StencilFunc(GLenum func, GLint ref, GLuint mask)
	{
		StencilFunc = (PFNSTENCILFUNCPROC)IntGetProcAddress("glStencilFunc");
		StencilFunc(func, ref, mask);
	}

	static void CODEGEN_FUNCPTR Switch_StencilMask(GLuint mask)
	{
		StencilMask = (PFNSTENCILMASKPROC)IntGetProcAddress("glStencilMask");
		StencilMask(mask);
	}

	static void CODEGEN_FUNCPTR Switch_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
	{
		StencilOp = (PFNSTENCILOPPROC)IntGetProcAddress("glStencilOp");
		StencilOp(fail, zfail, zpass);
	}

	static void CODEGEN_FUNCPTR Switch_TexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
	{
		TexImage1D = (PFNTEXIMAGE1DPROC)IntGetProcAddress("glTexImage1D");
		TexImage1D(target, level, internalformat, width, border, format, type, pixels);
	}

	static void CODEGEN_FUNCPTR Switch_TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
	{
		TexImage2D = (PFNTEXIMAGE2DPROC)IntGetProcAddress("glTexImage2D");
		TexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
	}

	static void CODEGEN_FUNCPTR Switch_TexParameterf(GLenum target, GLenum pname, GLfloat param)
	{
		TexParameterf = (PFNTEXPARAMETERFPROC)IntGetProcAddress("glTexParameterf");
		TexParameterf(target, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_TexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
	{
		TexParameterfv = (PFNTEXPARAMETERFVPROC)IntGetProcAddress("glTexParameterfv");
		TexParameterfv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_TexParameteri(GLenum target, GLenum pname, GLint param)
	{
		TexParameteri = (PFNTEXPARAMETERIPROC)IntGetProcAddress("glTexParameteri");
		TexParameteri(target, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_TexParameteriv(GLenum target, GLenum pname, const GLint * params)
	{
		TexParameteriv = (PFNTEXPARAMETERIVPROC)IntGetProcAddress("glTexParameteriv");
		TexParameteriv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		Viewport = (PFNVIEWPORTPROC)IntGetProcAddress("glViewport");
		Viewport(x, y, width, height);
	}

	
	// Extension: 1.1
	static void CODEGEN_FUNCPTR Switch_BindTexture(GLenum target, GLuint texture)
	{
		BindTexture = (PFNBINDTEXTUREPROC)IntGetProcAddress("glBindTexture");
		BindTexture(target, texture);
	}

	static void CODEGEN_FUNCPTR Switch_CopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
	{
		CopyTexImage1D = (PFNCOPYTEXIMAGE1DPROC)IntGetProcAddress("glCopyTexImage1D");
		CopyTexImage1D(target, level, internalformat, x, y, width, border);
	}

	static void CODEGEN_FUNCPTR Switch_CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
	{
		CopyTexImage2D = (PFNCOPYTEXIMAGE2DPROC)IntGetProcAddress("glCopyTexImage2D");
		CopyTexImage2D(target, level, internalformat, x, y, width, height, border);
	}

	static void CODEGEN_FUNCPTR Switch_CopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
	{
		CopyTexSubImage1D = (PFNCOPYTEXSUBIMAGE1DPROC)IntGetProcAddress("glCopyTexSubImage1D");
		CopyTexSubImage1D(target, level, xoffset, x, y, width);
	}

	static void CODEGEN_FUNCPTR Switch_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
	{
		CopyTexSubImage2D = (PFNCOPYTEXSUBIMAGE2DPROC)IntGetProcAddress("glCopyTexSubImage2D");
		CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteTextures(GLsizei n, const GLuint * textures)
	{
		DeleteTextures = (PFNDELETETEXTURESPROC)IntGetProcAddress("glDeleteTextures");
		DeleteTextures(n, textures);
	}

	static void CODEGEN_FUNCPTR Switch_DrawArrays(GLenum mode, GLint first, GLsizei count)
	{
		DrawArrays = (PFNDRAWARRAYSPROC)IntGetProcAddress("glDrawArrays");
		DrawArrays(mode, first, count);
	}

	static void CODEGEN_FUNCPTR Switch_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices)
	{
		DrawElements = (PFNDRAWELEMENTSPROC)IntGetProcAddress("glDrawElements");
		DrawElements(mode, count, type, indices);
	}

	static void CODEGEN_FUNCPTR Switch_GenTextures(GLsizei n, GLuint * textures)
	{
		GenTextures = (PFNGENTEXTURESPROC)IntGetProcAddress("glGenTextures");
		GenTextures(n, textures);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsTexture(GLuint texture)
	{
		IsTexture = (PFNISTEXTUREPROC)IntGetProcAddress("glIsTexture");
		return IsTexture(texture);
	}

	static void CODEGEN_FUNCPTR Switch_PolygonOffset(GLfloat factor, GLfloat units)
	{
		PolygonOffset = (PFNPOLYGONOFFSETPROC)IntGetProcAddress("glPolygonOffset");
		PolygonOffset(factor, units);
	}

	static void CODEGEN_FUNCPTR Switch_TexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
	{
		TexSubImage1D = (PFNTEXSUBIMAGE1DPROC)IntGetProcAddress("glTexSubImage1D");
		TexSubImage1D(target, level, xoffset, width, format, type, pixels);
	}

	static void CODEGEN_FUNCPTR Switch_TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
	{
		TexSubImage2D = (PFNTEXSUBIMAGE2DPROC)IntGetProcAddress("glTexSubImage2D");
		TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
	}

	
	// Extension: 1.2
	static void CODEGEN_FUNCPTR Switch_BlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	{
		BlendColor = (PFNBLENDCOLORPROC)IntGetProcAddress("glBlendColor");
		BlendColor(red, green, blue, alpha);
	}

	static void CODEGEN_FUNCPTR Switch_BlendEquation(GLenum mode)
	{
		BlendEquation = (PFNBLENDEQUATIONPROC)IntGetProcAddress("glBlendEquation");
		BlendEquation(mode);
	}

	static void CODEGEN_FUNCPTR Switch_CopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
	{
		CopyTexSubImage3D = (PFNCOPYTEXSUBIMAGE3DPROC)IntGetProcAddress("glCopyTexSubImage3D");
		CopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
	}

	static void CODEGEN_FUNCPTR Switch_DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
	{
		DrawRangeElements = (PFNDRAWRANGEELEMENTSPROC)IntGetProcAddress("glDrawRangeElements");
		DrawRangeElements(mode, start, end, count, type, indices);
	}

	static void CODEGEN_FUNCPTR Switch_TexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
	{
		TexImage3D = (PFNTEXIMAGE3DPROC)IntGetProcAddress("glTexImage3D");
		TexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
	}

	static void CODEGEN_FUNCPTR Switch_TexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
	{
		TexSubImage3D = (PFNTEXSUBIMAGE3DPROC)IntGetProcAddress("glTexSubImage3D");
		TexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
	}

	
	// Extension: 1.3
	static void CODEGEN_FUNCPTR Switch_ActiveTexture(GLenum texture)
	{
		ActiveTexture = (PFNACTIVETEXTUREPROC)IntGetProcAddress("glActiveTexture");
		ActiveTexture(texture);
	}

	static void CODEGEN_FUNCPTR Switch_CompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data)
	{
		CompressedTexImage1D = (PFNCOMPRESSEDTEXIMAGE1DPROC)IntGetProcAddress("glCompressedTexImage1D");
		CompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
	}

	static void CODEGEN_FUNCPTR Switch_CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data)
	{
		CompressedTexImage2D = (PFNCOMPRESSEDTEXIMAGE2DPROC)IntGetProcAddress("glCompressedTexImage2D");
		CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
	}

	static void CODEGEN_FUNCPTR Switch_CompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data)
	{
		CompressedTexImage3D = (PFNCOMPRESSEDTEXIMAGE3DPROC)IntGetProcAddress("glCompressedTexImage3D");
		CompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
	}

	static void CODEGEN_FUNCPTR Switch_CompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data)
	{
		CompressedTexSubImage1D = (PFNCOMPRESSEDTEXSUBIMAGE1DPROC)IntGetProcAddress("glCompressedTexSubImage1D");
		CompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
	}

	static void CODEGEN_FUNCPTR Switch_CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data)
	{
		CompressedTexSubImage2D = (PFNCOMPRESSEDTEXSUBIMAGE2DPROC)IntGetProcAddress("glCompressedTexSubImage2D");
		CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
	}

	static void CODEGEN_FUNCPTR Switch_CompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data)
	{
		CompressedTexSubImage3D = (PFNCOMPRESSEDTEXSUBIMAGE3DPROC)IntGetProcAddress("glCompressedTexSubImage3D");
		CompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
	}

	static void CODEGEN_FUNCPTR Switch_GetCompressedTexImage(GLenum target, GLint level, GLvoid * img)
	{
		GetCompressedTexImage = (PFNGETCOMPRESSEDTEXIMAGEPROC)IntGetProcAddress("glGetCompressedTexImage");
		GetCompressedTexImage(target, level, img);
	}

	static void CODEGEN_FUNCPTR Switch_SampleCoverage(GLfloat value, GLboolean invert)
	{
		SampleCoverage = (PFNSAMPLECOVERAGEPROC)IntGetProcAddress("glSampleCoverage");
		SampleCoverage(value, invert);
	}

	
	// Extension: 1.4
	static void CODEGEN_FUNCPTR Switch_BlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
	{
		BlendFuncSeparate = (PFNBLENDFUNCSEPARATEPROC)IntGetProcAddress("glBlendFuncSeparate");
		BlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
	}

	static void CODEGEN_FUNCPTR Switch_MultiDrawArrays(GLenum mode, const GLint * first, const GLsizei * count, GLsizei drawcount)
	{
		MultiDrawArrays = (PFNMULTIDRAWARRAYSPROC)IntGetProcAddress("glMultiDrawArrays");
		MultiDrawArrays(mode, first, count, drawcount);
	}

	static void CODEGEN_FUNCPTR Switch_MultiDrawElements(GLenum mode, const GLsizei * count, GLenum type, const GLvoid *const* indices, GLsizei drawcount)
	{
		MultiDrawElements = (PFNMULTIDRAWELEMENTSPROC)IntGetProcAddress("glMultiDrawElements");
		MultiDrawElements(mode, count, type, indices, drawcount);
	}

	static void CODEGEN_FUNCPTR Switch_PointParameterf(GLenum pname, GLfloat param)
	{
		PointParameterf = (PFNPOINTPARAMETERFPROC)IntGetProcAddress("glPointParameterf");
		PointParameterf(pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_PointParameterfv(GLenum pname, const GLfloat * params)
	{
		PointParameterfv = (PFNPOINTPARAMETERFVPROC)IntGetProcAddress("glPointParameterfv");
		PointParameterfv(pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_PointParameteri(GLenum pname, GLint param)
	{
		PointParameteri = (PFNPOINTPARAMETERIPROC)IntGetProcAddress("glPointParameteri");
		PointParameteri(pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_PointParameteriv(GLenum pname, const GLint * params)
	{
		PointParameteriv = (PFNPOINTPARAMETERIVPROC)IntGetProcAddress("glPointParameteriv");
		PointParameteriv(pname, params);
	}

	
	// Extension: 1.5
	static void CODEGEN_FUNCPTR Switch_BeginQuery(GLenum target, GLuint id)
	{
		BeginQuery = (PFNBEGINQUERYPROC)IntGetProcAddress("glBeginQuery");
		BeginQuery(target, id);
	}

	static void CODEGEN_FUNCPTR Switch_BindBuffer(GLenum target, GLuint buffer)
	{
		BindBuffer = (PFNBINDBUFFERPROC)IntGetProcAddress("glBindBuffer");
		BindBuffer(target, buffer);
	}

	static void CODEGEN_FUNCPTR Switch_BufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage)
	{
		BufferData = (PFNBUFFERDATAPROC)IntGetProcAddress("glBufferData");
		BufferData(target, size, data, usage);
	}

	static void CODEGEN_FUNCPTR Switch_BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data)
	{
		BufferSubData = (PFNBUFFERSUBDATAPROC)IntGetProcAddress("glBufferSubData");
		BufferSubData(target, offset, size, data);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteBuffers(GLsizei n, const GLuint * buffers)
	{
		DeleteBuffers = (PFNDELETEBUFFERSPROC)IntGetProcAddress("glDeleteBuffers");
		DeleteBuffers(n, buffers);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteQueries(GLsizei n, const GLuint * ids)
	{
		DeleteQueries = (PFNDELETEQUERIESPROC)IntGetProcAddress("glDeleteQueries");
		DeleteQueries(n, ids);
	}

	static void CODEGEN_FUNCPTR Switch_EndQuery(GLenum target)
	{
		EndQuery = (PFNENDQUERYPROC)IntGetProcAddress("glEndQuery");
		EndQuery(target);
	}

	static void CODEGEN_FUNCPTR Switch_GenBuffers(GLsizei n, GLuint * buffers)
	{
		GenBuffers = (PFNGENBUFFERSPROC)IntGetProcAddress("glGenBuffers");
		GenBuffers(n, buffers);
	}

	static void CODEGEN_FUNCPTR Switch_GenQueries(GLsizei n, GLuint * ids)
	{
		GenQueries = (PFNGENQUERIESPROC)IntGetProcAddress("glGenQueries");
		GenQueries(n, ids);
	}

	static void CODEGEN_FUNCPTR Switch_GetBufferParameteriv(GLenum target, GLenum pname, GLint * params)
	{
		GetBufferParameteriv = (PFNGETBUFFERPARAMETERIVPROC)IntGetProcAddress("glGetBufferParameteriv");
		GetBufferParameteriv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetBufferPointerv(GLenum target, GLenum pname, GLvoid ** params)
	{
		GetBufferPointerv = (PFNGETBUFFERPOINTERVPROC)IntGetProcAddress("glGetBufferPointerv");
		GetBufferPointerv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid * data)
	{
		GetBufferSubData = (PFNGETBUFFERSUBDATAPROC)IntGetProcAddress("glGetBufferSubData");
		GetBufferSubData(target, offset, size, data);
	}

	static void CODEGEN_FUNCPTR Switch_GetQueryObjectiv(GLuint id, GLenum pname, GLint * params)
	{
		GetQueryObjectiv = (PFNGETQUERYOBJECTIVPROC)IntGetProcAddress("glGetQueryObjectiv");
		GetQueryObjectiv(id, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetQueryObjectuiv(GLuint id, GLenum pname, GLuint * params)
	{
		GetQueryObjectuiv = (PFNGETQUERYOBJECTUIVPROC)IntGetProcAddress("glGetQueryObjectuiv");
		GetQueryObjectuiv(id, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetQueryiv(GLenum target, GLenum pname, GLint * params)
	{
		GetQueryiv = (PFNGETQUERYIVPROC)IntGetProcAddress("glGetQueryiv");
		GetQueryiv(target, pname, params);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsBuffer(GLuint buffer)
	{
		IsBuffer = (PFNISBUFFERPROC)IntGetProcAddress("glIsBuffer");
		return IsBuffer(buffer);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsQuery(GLuint id)
	{
		IsQuery = (PFNISQUERYPROC)IntGetProcAddress("glIsQuery");
		return IsQuery(id);
	}

	static void * CODEGEN_FUNCPTR Switch_MapBuffer(GLenum target, GLenum access)
	{
		MapBuffer = (PFNMAPBUFFERPROC)IntGetProcAddress("glMapBuffer");
		return MapBuffer(target, access);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_UnmapBuffer(GLenum target)
	{
		UnmapBuffer = (PFNUNMAPBUFFERPROC)IntGetProcAddress("glUnmapBuffer");
		return UnmapBuffer(target);
	}

	
	// Extension: 2.0
	static void CODEGEN_FUNCPTR Switch_AttachShader(GLuint program, GLuint shader)
	{
		AttachShader = (PFNATTACHSHADERPROC)IntGetProcAddress("glAttachShader");
		AttachShader(program, shader);
	}

	static void CODEGEN_FUNCPTR Switch_BindAttribLocation(GLuint program, GLuint index, const GLchar * name)
	{
		BindAttribLocation = (PFNBINDATTRIBLOCATIONPROC)IntGetProcAddress("glBindAttribLocation");
		BindAttribLocation(program, index, name);
	}

	static void CODEGEN_FUNCPTR Switch_BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
	{
		BlendEquationSeparate = (PFNBLENDEQUATIONSEPARATEPROC)IntGetProcAddress("glBlendEquationSeparate");
		BlendEquationSeparate(modeRGB, modeAlpha);
	}

	static void CODEGEN_FUNCPTR Switch_CompileShader(GLuint shader)
	{
		CompileShader = (PFNCOMPILESHADERPROC)IntGetProcAddress("glCompileShader");
		CompileShader(shader);
	}

	static GLuint CODEGEN_FUNCPTR Switch_CreateProgram()
	{
		CreateProgram = (PFNCREATEPROGRAMPROC)IntGetProcAddress("glCreateProgram");
		return CreateProgram();
	}

	static GLuint CODEGEN_FUNCPTR Switch_CreateShader(GLenum type)
	{
		CreateShader = (PFNCREATESHADERPROC)IntGetProcAddress("glCreateShader");
		return CreateShader(type);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteProgram(GLuint program)
	{
		DeleteProgram = (PFNDELETEPROGRAMPROC)IntGetProcAddress("glDeleteProgram");
		DeleteProgram(program);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteShader(GLuint shader)
	{
		DeleteShader = (PFNDELETESHADERPROC)IntGetProcAddress("glDeleteShader");
		DeleteShader(shader);
	}

	static void CODEGEN_FUNCPTR Switch_DetachShader(GLuint program, GLuint shader)
	{
		DetachShader = (PFNDETACHSHADERPROC)IntGetProcAddress("glDetachShader");
		DetachShader(program, shader);
	}

	static void CODEGEN_FUNCPTR Switch_DisableVertexAttribArray(GLuint index)
	{
		DisableVertexAttribArray = (PFNDISABLEVERTEXATTRIBARRAYPROC)IntGetProcAddress("glDisableVertexAttribArray");
		DisableVertexAttribArray(index);
	}

	static void CODEGEN_FUNCPTR Switch_DrawBuffers(GLsizei n, const GLenum * bufs)
	{
		DrawBuffers = (PFNDRAWBUFFERSPROC)IntGetProcAddress("glDrawBuffers");
		DrawBuffers(n, bufs);
	}

	static void CODEGEN_FUNCPTR Switch_EnableVertexAttribArray(GLuint index)
	{
		EnableVertexAttribArray = (PFNENABLEVERTEXATTRIBARRAYPROC)IntGetProcAddress("glEnableVertexAttribArray");
		EnableVertexAttribArray(index);
	}

	static void CODEGEN_FUNCPTR Switch_GetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
	{
		GetActiveAttrib = (PFNGETACTIVEATTRIBPROC)IntGetProcAddress("glGetActiveAttrib");
		GetActiveAttrib(program, index, bufSize, length, size, type, name);
	}

	static void CODEGEN_FUNCPTR Switch_GetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
	{
		GetActiveUniform = (PFNGETACTIVEUNIFORMPROC)IntGetProcAddress("glGetActiveUniform");
		GetActiveUniform(program, index, bufSize, length, size, type, name);
	}

	static void CODEGEN_FUNCPTR Switch_GetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei * count, GLuint * shaders)
	{
		GetAttachedShaders = (PFNGETATTACHEDSHADERSPROC)IntGetProcAddress("glGetAttachedShaders");
		GetAttachedShaders(program, maxCount, count, shaders);
	}

	static GLint CODEGEN_FUNCPTR Switch_GetAttribLocation(GLuint program, const GLchar * name)
	{
		GetAttribLocation = (PFNGETATTRIBLOCATIONPROC)IntGetProcAddress("glGetAttribLocation");
		return GetAttribLocation(program, name);
	}

	static void CODEGEN_FUNCPTR Switch_GetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
	{
		GetProgramInfoLog = (PFNGETPROGRAMINFOLOGPROC)IntGetProcAddress("glGetProgramInfoLog");
		GetProgramInfoLog(program, bufSize, length, infoLog);
	}

	static void CODEGEN_FUNCPTR Switch_GetProgramiv(GLuint program, GLenum pname, GLint * params)
	{
		GetProgramiv = (PFNGETPROGRAMIVPROC)IntGetProcAddress("glGetProgramiv");
		GetProgramiv(program, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
	{
		GetShaderInfoLog = (PFNGETSHADERINFOLOGPROC)IntGetProcAddress("glGetShaderInfoLog");
		GetShaderInfoLog(shader, bufSize, length, infoLog);
	}

	static void CODEGEN_FUNCPTR Switch_GetShaderSource(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source)
	{
		GetShaderSource = (PFNGETSHADERSOURCEPROC)IntGetProcAddress("glGetShaderSource");
		GetShaderSource(shader, bufSize, length, source);
	}

	static void CODEGEN_FUNCPTR Switch_GetShaderiv(GLuint shader, GLenum pname, GLint * params)
	{
		GetShaderiv = (PFNGETSHADERIVPROC)IntGetProcAddress("glGetShaderiv");
		GetShaderiv(shader, pname, params);
	}

	static GLint CODEGEN_FUNCPTR Switch_GetUniformLocation(GLuint program, const GLchar * name)
	{
		GetUniformLocation = (PFNGETUNIFORMLOCATIONPROC)IntGetProcAddress("glGetUniformLocation");
		return GetUniformLocation(program, name);
	}

	static void CODEGEN_FUNCPTR Switch_GetUniformfv(GLuint program, GLint location, GLfloat * params)
	{
		GetUniformfv = (PFNGETUNIFORMFVPROC)IntGetProcAddress("glGetUniformfv");
		GetUniformfv(program, location, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetUniformiv(GLuint program, GLint location, GLint * params)
	{
		GetUniformiv = (PFNGETUNIFORMIVPROC)IntGetProcAddress("glGetUniformiv");
		GetUniformiv(program, location, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid ** pointer)
	{
		GetVertexAttribPointerv = (PFNGETVERTEXATTRIBPOINTERVPROC)IntGetProcAddress("glGetVertexAttribPointerv");
		GetVertexAttribPointerv(index, pname, pointer);
	}

	static void CODEGEN_FUNCPTR Switch_GetVertexAttribdv(GLuint index, GLenum pname, GLdouble * params)
	{
		GetVertexAttribdv = (PFNGETVERTEXATTRIBDVPROC)IntGetProcAddress("glGetVertexAttribdv");
		GetVertexAttribdv(index, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetVertexAttribfv(GLuint index, GLenum pname, GLfloat * params)
	{
		GetVertexAttribfv = (PFNGETVERTEXATTRIBFVPROC)IntGetProcAddress("glGetVertexAttribfv");
		GetVertexAttribfv(index, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetVertexAttribiv(GLuint index, GLenum pname, GLint * params)
	{
		GetVertexAttribiv = (PFNGETVERTEXATTRIBIVPROC)IntGetProcAddress("glGetVertexAttribiv");
		GetVertexAttribiv(index, pname, params);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsProgram(GLuint program)
	{
		IsProgram = (PFNISPROGRAMPROC)IntGetProcAddress("glIsProgram");
		return IsProgram(program);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsShader(GLuint shader)
	{
		IsShader = (PFNISSHADERPROC)IntGetProcAddress("glIsShader");
		return IsShader(shader);
	}

	static void CODEGEN_FUNCPTR Switch_LinkProgram(GLuint program)
	{
		LinkProgram = (PFNLINKPROGRAMPROC)IntGetProcAddress("glLinkProgram");
		LinkProgram(program);
	}

	static void CODEGEN_FUNCPTR Switch_ShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length)
	{
		ShaderSource = (PFNSHADERSOURCEPROC)IntGetProcAddress("glShaderSource");
		ShaderSource(shader, count, string, length);
	}

	static void CODEGEN_FUNCPTR Switch_StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
	{
		StencilFuncSeparate = (PFNSTENCILFUNCSEPARATEPROC)IntGetProcAddress("glStencilFuncSeparate");
		StencilFuncSeparate(face, func, ref, mask);
	}

	static void CODEGEN_FUNCPTR Switch_StencilMaskSeparate(GLenum face, GLuint mask)
	{
		StencilMaskSeparate = (PFNSTENCILMASKSEPARATEPROC)IntGetProcAddress("glStencilMaskSeparate");
		StencilMaskSeparate(face, mask);
	}

	static void CODEGEN_FUNCPTR Switch_StencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
	{
		StencilOpSeparate = (PFNSTENCILOPSEPARATEPROC)IntGetProcAddress("glStencilOpSeparate");
		StencilOpSeparate(face, sfail, dpfail, dppass);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform1f(GLint location, GLfloat v0)
	{
		Uniform1f = (PFNUNIFORM1FPROC)IntGetProcAddress("glUniform1f");
		Uniform1f(location, v0);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform1fv(GLint location, GLsizei count, const GLfloat * value)
	{
		Uniform1fv = (PFNUNIFORM1FVPROC)IntGetProcAddress("glUniform1fv");
		Uniform1fv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform1i(GLint location, GLint v0)
	{
		Uniform1i = (PFNUNIFORM1IPROC)IntGetProcAddress("glUniform1i");
		Uniform1i(location, v0);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform1iv(GLint location, GLsizei count, const GLint * value)
	{
		Uniform1iv = (PFNUNIFORM1IVPROC)IntGetProcAddress("glUniform1iv");
		Uniform1iv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform2f(GLint location, GLfloat v0, GLfloat v1)
	{
		Uniform2f = (PFNUNIFORM2FPROC)IntGetProcAddress("glUniform2f");
		Uniform2f(location, v0, v1);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform2fv(GLint location, GLsizei count, const GLfloat * value)
	{
		Uniform2fv = (PFNUNIFORM2FVPROC)IntGetProcAddress("glUniform2fv");
		Uniform2fv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform2i(GLint location, GLint v0, GLint v1)
	{
		Uniform2i = (PFNUNIFORM2IPROC)IntGetProcAddress("glUniform2i");
		Uniform2i(location, v0, v1);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform2iv(GLint location, GLsizei count, const GLint * value)
	{
		Uniform2iv = (PFNUNIFORM2IVPROC)IntGetProcAddress("glUniform2iv");
		Uniform2iv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
	{
		Uniform3f = (PFNUNIFORM3FPROC)IntGetProcAddress("glUniform3f");
		Uniform3f(location, v0, v1, v2);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform3fv(GLint location, GLsizei count, const GLfloat * value)
	{
		Uniform3fv = (PFNUNIFORM3FVPROC)IntGetProcAddress("glUniform3fv");
		Uniform3fv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform3i(GLint location, GLint v0, GLint v1, GLint v2)
	{
		Uniform3i = (PFNUNIFORM3IPROC)IntGetProcAddress("glUniform3i");
		Uniform3i(location, v0, v1, v2);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform3iv(GLint location, GLsizei count, const GLint * value)
	{
		Uniform3iv = (PFNUNIFORM3IVPROC)IntGetProcAddress("glUniform3iv");
		Uniform3iv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
	{
		Uniform4f = (PFNUNIFORM4FPROC)IntGetProcAddress("glUniform4f");
		Uniform4f(location, v0, v1, v2, v3);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform4fv(GLint location, GLsizei count, const GLfloat * value)
	{
		Uniform4fv = (PFNUNIFORM4FVPROC)IntGetProcAddress("glUniform4fv");
		Uniform4fv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
	{
		Uniform4i = (PFNUNIFORM4IPROC)IntGetProcAddress("glUniform4i");
		Uniform4i(location, v0, v1, v2, v3);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform4iv(GLint location, GLsizei count, const GLint * value)
	{
		Uniform4iv = (PFNUNIFORM4IVPROC)IntGetProcAddress("glUniform4iv");
		Uniform4iv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix2fv = (PFNUNIFORMMATRIX2FVPROC)IntGetProcAddress("glUniformMatrix2fv");
		UniformMatrix2fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix3fv = (PFNUNIFORMMATRIX3FVPROC)IntGetProcAddress("glUniformMatrix3fv");
		UniformMatrix3fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix4fv = (PFNUNIFORMMATRIX4FVPROC)IntGetProcAddress("glUniformMatrix4fv");
		UniformMatrix4fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UseProgram(GLuint program)
	{
		UseProgram = (PFNUSEPROGRAMPROC)IntGetProcAddress("glUseProgram");
		UseProgram(program);
	}

	static void CODEGEN_FUNCPTR Switch_ValidateProgram(GLuint program)
	{
		ValidateProgram = (PFNVALIDATEPROGRAMPROC)IntGetProcAddress("glValidateProgram");
		ValidateProgram(program);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib1d(GLuint index, GLdouble x)
	{
		VertexAttrib1d = (PFNVERTEXATTRIB1DPROC)IntGetProcAddress("glVertexAttrib1d");
		VertexAttrib1d(index, x);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib1dv(GLuint index, const GLdouble * v)
	{
		VertexAttrib1dv = (PFNVERTEXATTRIB1DVPROC)IntGetProcAddress("glVertexAttrib1dv");
		VertexAttrib1dv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib1f(GLuint index, GLfloat x)
	{
		VertexAttrib1f = (PFNVERTEXATTRIB1FPROC)IntGetProcAddress("glVertexAttrib1f");
		VertexAttrib1f(index, x);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib1fv(GLuint index, const GLfloat * v)
	{
		VertexAttrib1fv = (PFNVERTEXATTRIB1FVPROC)IntGetProcAddress("glVertexAttrib1fv");
		VertexAttrib1fv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib1s(GLuint index, GLshort x)
	{
		VertexAttrib1s = (PFNVERTEXATTRIB1SPROC)IntGetProcAddress("glVertexAttrib1s");
		VertexAttrib1s(index, x);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib1sv(GLuint index, const GLshort * v)
	{
		VertexAttrib1sv = (PFNVERTEXATTRIB1SVPROC)IntGetProcAddress("glVertexAttrib1sv");
		VertexAttrib1sv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib2d(GLuint index, GLdouble x, GLdouble y)
	{
		VertexAttrib2d = (PFNVERTEXATTRIB2DPROC)IntGetProcAddress("glVertexAttrib2d");
		VertexAttrib2d(index, x, y);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib2dv(GLuint index, const GLdouble * v)
	{
		VertexAttrib2dv = (PFNVERTEXATTRIB2DVPROC)IntGetProcAddress("glVertexAttrib2dv");
		VertexAttrib2dv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
	{
		VertexAttrib2f = (PFNVERTEXATTRIB2FPROC)IntGetProcAddress("glVertexAttrib2f");
		VertexAttrib2f(index, x, y);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib2fv(GLuint index, const GLfloat * v)
	{
		VertexAttrib2fv = (PFNVERTEXATTRIB2FVPROC)IntGetProcAddress("glVertexAttrib2fv");
		VertexAttrib2fv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib2s(GLuint index, GLshort x, GLshort y)
	{
		VertexAttrib2s = (PFNVERTEXATTRIB2SPROC)IntGetProcAddress("glVertexAttrib2s");
		VertexAttrib2s(index, x, y);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib2sv(GLuint index, const GLshort * v)
	{
		VertexAttrib2sv = (PFNVERTEXATTRIB2SVPROC)IntGetProcAddress("glVertexAttrib2sv");
		VertexAttrib2sv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z)
	{
		VertexAttrib3d = (PFNVERTEXATTRIB3DPROC)IntGetProcAddress("glVertexAttrib3d");
		VertexAttrib3d(index, x, y, z);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib3dv(GLuint index, const GLdouble * v)
	{
		VertexAttrib3dv = (PFNVERTEXATTRIB3DVPROC)IntGetProcAddress("glVertexAttrib3dv");
		VertexAttrib3dv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z)
	{
		VertexAttrib3f = (PFNVERTEXATTRIB3FPROC)IntGetProcAddress("glVertexAttrib3f");
		VertexAttrib3f(index, x, y, z);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib3fv(GLuint index, const GLfloat * v)
	{
		VertexAttrib3fv = (PFNVERTEXATTRIB3FVPROC)IntGetProcAddress("glVertexAttrib3fv");
		VertexAttrib3fv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z)
	{
		VertexAttrib3s = (PFNVERTEXATTRIB3SPROC)IntGetProcAddress("glVertexAttrib3s");
		VertexAttrib3s(index, x, y, z);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib3sv(GLuint index, const GLshort * v)
	{
		VertexAttrib3sv = (PFNVERTEXATTRIB3SVPROC)IntGetProcAddress("glVertexAttrib3sv");
		VertexAttrib3sv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4Nbv(GLuint index, const GLbyte * v)
	{
		VertexAttrib4Nbv = (PFNVERTEXATTRIB4NBVPROC)IntGetProcAddress("glVertexAttrib4Nbv");
		VertexAttrib4Nbv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4Niv(GLuint index, const GLint * v)
	{
		VertexAttrib4Niv = (PFNVERTEXATTRIB4NIVPROC)IntGetProcAddress("glVertexAttrib4Niv");
		VertexAttrib4Niv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4Nsv(GLuint index, const GLshort * v)
	{
		VertexAttrib4Nsv = (PFNVERTEXATTRIB4NSVPROC)IntGetProcAddress("glVertexAttrib4Nsv");
		VertexAttrib4Nsv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
	{
		VertexAttrib4Nub = (PFNVERTEXATTRIB4NUBPROC)IntGetProcAddress("glVertexAttrib4Nub");
		VertexAttrib4Nub(index, x, y, z, w);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4Nubv(GLuint index, const GLubyte * v)
	{
		VertexAttrib4Nubv = (PFNVERTEXATTRIB4NUBVPROC)IntGetProcAddress("glVertexAttrib4Nubv");
		VertexAttrib4Nubv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4Nuiv(GLuint index, const GLuint * v)
	{
		VertexAttrib4Nuiv = (PFNVERTEXATTRIB4NUIVPROC)IntGetProcAddress("glVertexAttrib4Nuiv");
		VertexAttrib4Nuiv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4Nusv(GLuint index, const GLushort * v)
	{
		VertexAttrib4Nusv = (PFNVERTEXATTRIB4NUSVPROC)IntGetProcAddress("glVertexAttrib4Nusv");
		VertexAttrib4Nusv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4bv(GLuint index, const GLbyte * v)
	{
		VertexAttrib4bv = (PFNVERTEXATTRIB4BVPROC)IntGetProcAddress("glVertexAttrib4bv");
		VertexAttrib4bv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
	{
		VertexAttrib4d = (PFNVERTEXATTRIB4DPROC)IntGetProcAddress("glVertexAttrib4d");
		VertexAttrib4d(index, x, y, z, w);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4dv(GLuint index, const GLdouble * v)
	{
		VertexAttrib4dv = (PFNVERTEXATTRIB4DVPROC)IntGetProcAddress("glVertexAttrib4dv");
		VertexAttrib4dv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
	{
		VertexAttrib4f = (PFNVERTEXATTRIB4FPROC)IntGetProcAddress("glVertexAttrib4f");
		VertexAttrib4f(index, x, y, z, w);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4fv(GLuint index, const GLfloat * v)
	{
		VertexAttrib4fv = (PFNVERTEXATTRIB4FVPROC)IntGetProcAddress("glVertexAttrib4fv");
		VertexAttrib4fv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4iv(GLuint index, const GLint * v)
	{
		VertexAttrib4iv = (PFNVERTEXATTRIB4IVPROC)IntGetProcAddress("glVertexAttrib4iv");
		VertexAttrib4iv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
	{
		VertexAttrib4s = (PFNVERTEXATTRIB4SPROC)IntGetProcAddress("glVertexAttrib4s");
		VertexAttrib4s(index, x, y, z, w);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4sv(GLuint index, const GLshort * v)
	{
		VertexAttrib4sv = (PFNVERTEXATTRIB4SVPROC)IntGetProcAddress("glVertexAttrib4sv");
		VertexAttrib4sv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4ubv(GLuint index, const GLubyte * v)
	{
		VertexAttrib4ubv = (PFNVERTEXATTRIB4UBVPROC)IntGetProcAddress("glVertexAttrib4ubv");
		VertexAttrib4ubv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4uiv(GLuint index, const GLuint * v)
	{
		VertexAttrib4uiv = (PFNVERTEXATTRIB4UIVPROC)IntGetProcAddress("glVertexAttrib4uiv");
		VertexAttrib4uiv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttrib4usv(GLuint index, const GLushort * v)
	{
		VertexAttrib4usv = (PFNVERTEXATTRIB4USVPROC)IntGetProcAddress("glVertexAttrib4usv");
		VertexAttrib4usv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
	{
		VertexAttribPointer = (PFNVERTEXATTRIBPOINTERPROC)IntGetProcAddress("glVertexAttribPointer");
		VertexAttribPointer(index, size, type, normalized, stride, pointer);
	}

	
	// Extension: 2.1
	static void CODEGEN_FUNCPTR Switch_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix2x3fv = (PFNUNIFORMMATRIX2X3FVPROC)IntGetProcAddress("glUniformMatrix2x3fv");
		UniformMatrix2x3fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix2x4fv = (PFNUNIFORMMATRIX2X4FVPROC)IntGetProcAddress("glUniformMatrix2x4fv");
		UniformMatrix2x4fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix3x2fv = (PFNUNIFORMMATRIX3X2FVPROC)IntGetProcAddress("glUniformMatrix3x2fv");
		UniformMatrix3x2fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix3x4fv = (PFNUNIFORMMATRIX3X4FVPROC)IntGetProcAddress("glUniformMatrix3x4fv");
		UniformMatrix3x4fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix4x2fv = (PFNUNIFORMMATRIX4X2FVPROC)IntGetProcAddress("glUniformMatrix4x2fv");
		UniformMatrix4x2fv(location, count, transpose, value);
	}

	static void CODEGEN_FUNCPTR Switch_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
	{
		UniformMatrix4x3fv = (PFNUNIFORMMATRIX4X3FVPROC)IntGetProcAddress("glUniformMatrix4x3fv");
		UniformMatrix4x3fv(location, count, transpose, value);
	}

	
	// Extension: 3.0
	static void CODEGEN_FUNCPTR Switch_BeginConditionalRender(GLuint id, GLenum mode)
	{
		BeginConditionalRender = (PFNBEGINCONDITIONALRENDERPROC)IntGetProcAddress("glBeginConditionalRender");
		BeginConditionalRender(id, mode);
	}

	static void CODEGEN_FUNCPTR Switch_BeginTransformFeedback(GLenum primitiveMode)
	{
		BeginTransformFeedback = (PFNBEGINTRANSFORMFEEDBACKPROC)IntGetProcAddress("glBeginTransformFeedback");
		BeginTransformFeedback(primitiveMode);
	}

	static void CODEGEN_FUNCPTR Switch_BindBufferBase(GLenum target, GLuint index, GLuint buffer)
	{
		BindBufferBase = (PFNBINDBUFFERBASEPROC)IntGetProcAddress("glBindBufferBase");
		BindBufferBase(target, index, buffer);
	}

	static void CODEGEN_FUNCPTR Switch_BindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
	{
		BindBufferRange = (PFNBINDBUFFERRANGEPROC)IntGetProcAddress("glBindBufferRange");
		BindBufferRange(target, index, buffer, offset, size);
	}

	static void CODEGEN_FUNCPTR Switch_BindFragDataLocation(GLuint program, GLuint color, const GLchar * name)
	{
		BindFragDataLocation = (PFNBINDFRAGDATALOCATIONPROC)IntGetProcAddress("glBindFragDataLocation");
		BindFragDataLocation(program, color, name);
	}

	static void CODEGEN_FUNCPTR Switch_BindFramebuffer(GLenum target, GLuint framebuffer)
	{
		BindFramebuffer = (PFNBINDFRAMEBUFFERPROC)IntGetProcAddress("glBindFramebuffer");
		BindFramebuffer(target, framebuffer);
	}

	static void CODEGEN_FUNCPTR Switch_BindRenderbuffer(GLenum target, GLuint renderbuffer)
	{
		BindRenderbuffer = (PFNBINDRENDERBUFFERPROC)IntGetProcAddress("glBindRenderbuffer");
		BindRenderbuffer(target, renderbuffer);
	}

	static void CODEGEN_FUNCPTR Switch_BindVertexArray(GLuint ren_array)
	{
		BindVertexArray = (PFNBINDVERTEXARRAYPROC)IntGetProcAddress("glBindVertexArray");
		BindVertexArray(ren_array);
	}

	static void CODEGEN_FUNCPTR Switch_BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
	{
		BlitFramebuffer = (PFNBLITFRAMEBUFFERPROC)IntGetProcAddress("glBlitFramebuffer");
		BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	}

	static GLenum CODEGEN_FUNCPTR Switch_CheckFramebufferStatus(GLenum target)
	{
		CheckFramebufferStatus = (PFNCHECKFRAMEBUFFERSTATUSPROC)IntGetProcAddress("glCheckFramebufferStatus");
		return CheckFramebufferStatus(target);
	}

	static void CODEGEN_FUNCPTR Switch_ClampColor(GLenum target, GLenum clamp)
	{
		ClampColor = (PFNCLAMPCOLORPROC)IntGetProcAddress("glClampColor");
		ClampColor(target, clamp);
	}

	static void CODEGEN_FUNCPTR Switch_ClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
	{
		ClearBufferfi = (PFNCLEARBUFFERFIPROC)IntGetProcAddress("glClearBufferfi");
		ClearBufferfi(buffer, drawbuffer, depth, stencil);
	}

	static void CODEGEN_FUNCPTR Switch_ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value)
	{
		ClearBufferfv = (PFNCLEARBUFFERFVPROC)IntGetProcAddress("glClearBufferfv");
		ClearBufferfv(buffer, drawbuffer, value);
	}

	static void CODEGEN_FUNCPTR Switch_ClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value)
	{
		ClearBufferiv = (PFNCLEARBUFFERIVPROC)IntGetProcAddress("glClearBufferiv");
		ClearBufferiv(buffer, drawbuffer, value);
	}

	static void CODEGEN_FUNCPTR Switch_ClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value)
	{
		ClearBufferuiv = (PFNCLEARBUFFERUIVPROC)IntGetProcAddress("glClearBufferuiv");
		ClearBufferuiv(buffer, drawbuffer, value);
	}

	static void CODEGEN_FUNCPTR Switch_ColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a)
	{
		ColorMaski = (PFNCOLORMASKIPROC)IntGetProcAddress("glColorMaski");
		ColorMaski(index, r, g, b, a);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteFramebuffers(GLsizei n, const GLuint * framebuffers)
	{
		DeleteFramebuffers = (PFNDELETEFRAMEBUFFERSPROC)IntGetProcAddress("glDeleteFramebuffers");
		DeleteFramebuffers(n, framebuffers);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers)
	{
		DeleteRenderbuffers = (PFNDELETERENDERBUFFERSPROC)IntGetProcAddress("glDeleteRenderbuffers");
		DeleteRenderbuffers(n, renderbuffers);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteVertexArrays(GLsizei n, const GLuint * arrays)
	{
		DeleteVertexArrays = (PFNDELETEVERTEXARRAYSPROC)IntGetProcAddress("glDeleteVertexArrays");
		DeleteVertexArrays(n, arrays);
	}

	static void CODEGEN_FUNCPTR Switch_Disablei(GLenum target, GLuint index)
	{
		Disablei = (PFNDISABLEIPROC)IntGetProcAddress("glDisablei");
		Disablei(target, index);
	}

	static void CODEGEN_FUNCPTR Switch_Enablei(GLenum target, GLuint index)
	{
		Enablei = (PFNENABLEIPROC)IntGetProcAddress("glEnablei");
		Enablei(target, index);
	}

	static void CODEGEN_FUNCPTR Switch_EndConditionalRender()
	{
		EndConditionalRender = (PFNENDCONDITIONALRENDERPROC)IntGetProcAddress("glEndConditionalRender");
		EndConditionalRender();
	}

	static void CODEGEN_FUNCPTR Switch_EndTransformFeedback()
	{
		EndTransformFeedback = (PFNENDTRANSFORMFEEDBACKPROC)IntGetProcAddress("glEndTransformFeedback");
		EndTransformFeedback();
	}

	static void CODEGEN_FUNCPTR Switch_FlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
	{
		FlushMappedBufferRange = (PFNFLUSHMAPPEDBUFFERRANGEPROC)IntGetProcAddress("glFlushMappedBufferRange");
		FlushMappedBufferRange(target, offset, length);
	}

	static void CODEGEN_FUNCPTR Switch_FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
	{
		FramebufferRenderbuffer = (PFNFRAMEBUFFERRENDERBUFFERPROC)IntGetProcAddress("glFramebufferRenderbuffer");
		FramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
	}

	static void CODEGEN_FUNCPTR Switch_FramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
	{
		FramebufferTexture1D = (PFNFRAMEBUFFERTEXTURE1DPROC)IntGetProcAddress("glFramebufferTexture1D");
		FramebufferTexture1D(target, attachment, textarget, texture, level);
	}

	static void CODEGEN_FUNCPTR Switch_FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
	{
		FramebufferTexture2D = (PFNFRAMEBUFFERTEXTURE2DPROC)IntGetProcAddress("glFramebufferTexture2D");
		FramebufferTexture2D(target, attachment, textarget, texture, level);
	}

	static void CODEGEN_FUNCPTR Switch_FramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
	{
		FramebufferTexture3D = (PFNFRAMEBUFFERTEXTURE3DPROC)IntGetProcAddress("glFramebufferTexture3D");
		FramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
	}

	static void CODEGEN_FUNCPTR Switch_FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
	{
		FramebufferTextureLayer = (PFNFRAMEBUFFERTEXTURELAYERPROC)IntGetProcAddress("glFramebufferTextureLayer");
		FramebufferTextureLayer(target, attachment, texture, level, layer);
	}

	static void CODEGEN_FUNCPTR Switch_GenFramebuffers(GLsizei n, GLuint * framebuffers)
	{
		GenFramebuffers = (PFNGENFRAMEBUFFERSPROC)IntGetProcAddress("glGenFramebuffers");
		GenFramebuffers(n, framebuffers);
	}

	static void CODEGEN_FUNCPTR Switch_GenRenderbuffers(GLsizei n, GLuint * renderbuffers)
	{
		GenRenderbuffers = (PFNGENRENDERBUFFERSPROC)IntGetProcAddress("glGenRenderbuffers");
		GenRenderbuffers(n, renderbuffers);
	}

	static void CODEGEN_FUNCPTR Switch_GenVertexArrays(GLsizei n, GLuint * arrays)
	{
		GenVertexArrays = (PFNGENVERTEXARRAYSPROC)IntGetProcAddress("glGenVertexArrays");
		GenVertexArrays(n, arrays);
	}

	static void CODEGEN_FUNCPTR Switch_GenerateMipmap(GLenum target)
	{
		GenerateMipmap = (PFNGENERATEMIPMAPPROC)IntGetProcAddress("glGenerateMipmap");
		GenerateMipmap(target);
	}

	static void CODEGEN_FUNCPTR Switch_GetBooleani_v(GLenum target, GLuint index, GLboolean * data)
	{
		GetBooleani_v = (PFNGETBOOLEANI_VPROC)IntGetProcAddress("glGetBooleani_v");
		GetBooleani_v(target, index, data);
	}

	static GLint CODEGEN_FUNCPTR Switch_GetFragDataLocation(GLuint program, const GLchar * name)
	{
		GetFragDataLocation = (PFNGETFRAGDATALOCATIONPROC)IntGetProcAddress("glGetFragDataLocation");
		return GetFragDataLocation(program, name);
	}

	static void CODEGEN_FUNCPTR Switch_GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint * params)
	{
		GetFramebufferAttachmentParameteriv = (PFNGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)IntGetProcAddress("glGetFramebufferAttachmentParameteriv");
		GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetIntegeri_v(GLenum target, GLuint index, GLint * data)
	{
		GetIntegeri_v = (PFNGETINTEGERI_VPROC)IntGetProcAddress("glGetIntegeri_v");
		GetIntegeri_v(target, index, data);
	}

	static void CODEGEN_FUNCPTR Switch_GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint * params)
	{
		GetRenderbufferParameteriv = (PFNGETRENDERBUFFERPARAMETERIVPROC)IntGetProcAddress("glGetRenderbufferParameteriv");
		GetRenderbufferParameteriv(target, pname, params);
	}

	static const GLubyte * CODEGEN_FUNCPTR Switch_GetStringi(GLenum name, GLuint index)
	{
		GetStringi = (PFNGETSTRINGIPROC)IntGetProcAddress("glGetStringi");
		return GetStringi(name, index);
	}

	static void CODEGEN_FUNCPTR Switch_GetTexParameterIiv(GLenum target, GLenum pname, GLint * params)
	{
		GetTexParameterIiv = (PFNGETTEXPARAMETERIIVPROC)IntGetProcAddress("glGetTexParameterIiv");
		GetTexParameterIiv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetTexParameterIuiv(GLenum target, GLenum pname, GLuint * params)
	{
		GetTexParameterIuiv = (PFNGETTEXPARAMETERIUIVPROC)IntGetProcAddress("glGetTexParameterIuiv");
		GetTexParameterIuiv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name)
	{
		GetTransformFeedbackVarying = (PFNGETTRANSFORMFEEDBACKVARYINGPROC)IntGetProcAddress("glGetTransformFeedbackVarying");
		GetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
	}

	static void CODEGEN_FUNCPTR Switch_GetUniformuiv(GLuint program, GLint location, GLuint * params)
	{
		GetUniformuiv = (PFNGETUNIFORMUIVPROC)IntGetProcAddress("glGetUniformuiv");
		GetUniformuiv(program, location, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetVertexAttribIiv(GLuint index, GLenum pname, GLint * params)
	{
		GetVertexAttribIiv = (PFNGETVERTEXATTRIBIIVPROC)IntGetProcAddress("glGetVertexAttribIiv");
		GetVertexAttribIiv(index, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetVertexAttribIuiv(GLuint index, GLenum pname, GLuint * params)
	{
		GetVertexAttribIuiv = (PFNGETVERTEXATTRIBIUIVPROC)IntGetProcAddress("glGetVertexAttribIuiv");
		GetVertexAttribIuiv(index, pname, params);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsEnabledi(GLenum target, GLuint index)
	{
		IsEnabledi = (PFNISENABLEDIPROC)IntGetProcAddress("glIsEnabledi");
		return IsEnabledi(target, index);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsFramebuffer(GLuint framebuffer)
	{
		IsFramebuffer = (PFNISFRAMEBUFFERPROC)IntGetProcAddress("glIsFramebuffer");
		return IsFramebuffer(framebuffer);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsRenderbuffer(GLuint renderbuffer)
	{
		IsRenderbuffer = (PFNISRENDERBUFFERPROC)IntGetProcAddress("glIsRenderbuffer");
		return IsRenderbuffer(renderbuffer);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsVertexArray(GLuint ren_array)
	{
		IsVertexArray = (PFNISVERTEXARRAYPROC)IntGetProcAddress("glIsVertexArray");
		return IsVertexArray(ren_array);
	}

	static void * CODEGEN_FUNCPTR Switch_MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
	{
		MapBufferRange = (PFNMAPBUFFERRANGEPROC)IntGetProcAddress("glMapBufferRange");
		return MapBufferRange(target, offset, length, access);
	}

	static void CODEGEN_FUNCPTR Switch_RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
	{
		RenderbufferStorage = (PFNRENDERBUFFERSTORAGEPROC)IntGetProcAddress("glRenderbufferStorage");
		RenderbufferStorage(target, internalformat, width, height);
	}

	static void CODEGEN_FUNCPTR Switch_RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
	{
		RenderbufferStorageMultisample = (PFNRENDERBUFFERSTORAGEMULTISAMPLEPROC)IntGetProcAddress("glRenderbufferStorageMultisample");
		RenderbufferStorageMultisample(target, samples, internalformat, width, height);
	}

	static void CODEGEN_FUNCPTR Switch_TexParameterIiv(GLenum target, GLenum pname, const GLint * params)
	{
		TexParameterIiv = (PFNTEXPARAMETERIIVPROC)IntGetProcAddress("glTexParameterIiv");
		TexParameterIiv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_TexParameterIuiv(GLenum target, GLenum pname, const GLuint * params)
	{
		TexParameterIuiv = (PFNTEXPARAMETERIUIVPROC)IntGetProcAddress("glTexParameterIuiv");
		TexParameterIuiv(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_TransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode)
	{
		TransformFeedbackVaryings = (PFNTRANSFORMFEEDBACKVARYINGSPROC)IntGetProcAddress("glTransformFeedbackVaryings");
		TransformFeedbackVaryings(program, count, varyings, bufferMode);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform1ui(GLint location, GLuint v0)
	{
		Uniform1ui = (PFNUNIFORM1UIPROC)IntGetProcAddress("glUniform1ui");
		Uniform1ui(location, v0);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform1uiv(GLint location, GLsizei count, const GLuint * value)
	{
		Uniform1uiv = (PFNUNIFORM1UIVPROC)IntGetProcAddress("glUniform1uiv");
		Uniform1uiv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform2ui(GLint location, GLuint v0, GLuint v1)
	{
		Uniform2ui = (PFNUNIFORM2UIPROC)IntGetProcAddress("glUniform2ui");
		Uniform2ui(location, v0, v1);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform2uiv(GLint location, GLsizei count, const GLuint * value)
	{
		Uniform2uiv = (PFNUNIFORM2UIVPROC)IntGetProcAddress("glUniform2uiv");
		Uniform2uiv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
	{
		Uniform3ui = (PFNUNIFORM3UIPROC)IntGetProcAddress("glUniform3ui");
		Uniform3ui(location, v0, v1, v2);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform3uiv(GLint location, GLsizei count, const GLuint * value)
	{
		Uniform3uiv = (PFNUNIFORM3UIVPROC)IntGetProcAddress("glUniform3uiv");
		Uniform3uiv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
	{
		Uniform4ui = (PFNUNIFORM4UIPROC)IntGetProcAddress("glUniform4ui");
		Uniform4ui(location, v0, v1, v2, v3);
	}

	static void CODEGEN_FUNCPTR Switch_Uniform4uiv(GLint location, GLsizei count, const GLuint * value)
	{
		Uniform4uiv = (PFNUNIFORM4UIVPROC)IntGetProcAddress("glUniform4uiv");
		Uniform4uiv(location, count, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI1i(GLuint index, GLint x)
	{
		VertexAttribI1i = (PFNVERTEXATTRIBI1IPROC)IntGetProcAddress("glVertexAttribI1i");
		VertexAttribI1i(index, x);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI1iv(GLuint index, const GLint * v)
	{
		VertexAttribI1iv = (PFNVERTEXATTRIBI1IVPROC)IntGetProcAddress("glVertexAttribI1iv");
		VertexAttribI1iv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI1ui(GLuint index, GLuint x)
	{
		VertexAttribI1ui = (PFNVERTEXATTRIBI1UIPROC)IntGetProcAddress("glVertexAttribI1ui");
		VertexAttribI1ui(index, x);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI1uiv(GLuint index, const GLuint * v)
	{
		VertexAttribI1uiv = (PFNVERTEXATTRIBI1UIVPROC)IntGetProcAddress("glVertexAttribI1uiv");
		VertexAttribI1uiv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI2i(GLuint index, GLint x, GLint y)
	{
		VertexAttribI2i = (PFNVERTEXATTRIBI2IPROC)IntGetProcAddress("glVertexAttribI2i");
		VertexAttribI2i(index, x, y);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI2iv(GLuint index, const GLint * v)
	{
		VertexAttribI2iv = (PFNVERTEXATTRIBI2IVPROC)IntGetProcAddress("glVertexAttribI2iv");
		VertexAttribI2iv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI2ui(GLuint index, GLuint x, GLuint y)
	{
		VertexAttribI2ui = (PFNVERTEXATTRIBI2UIPROC)IntGetProcAddress("glVertexAttribI2ui");
		VertexAttribI2ui(index, x, y);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI2uiv(GLuint index, const GLuint * v)
	{
		VertexAttribI2uiv = (PFNVERTEXATTRIBI2UIVPROC)IntGetProcAddress("glVertexAttribI2uiv");
		VertexAttribI2uiv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI3i(GLuint index, GLint x, GLint y, GLint z)
	{
		VertexAttribI3i = (PFNVERTEXATTRIBI3IPROC)IntGetProcAddress("glVertexAttribI3i");
		VertexAttribI3i(index, x, y, z);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI3iv(GLuint index, const GLint * v)
	{
		VertexAttribI3iv = (PFNVERTEXATTRIBI3IVPROC)IntGetProcAddress("glVertexAttribI3iv");
		VertexAttribI3iv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z)
	{
		VertexAttribI3ui = (PFNVERTEXATTRIBI3UIPROC)IntGetProcAddress("glVertexAttribI3ui");
		VertexAttribI3ui(index, x, y, z);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI3uiv(GLuint index, const GLuint * v)
	{
		VertexAttribI3uiv = (PFNVERTEXATTRIBI3UIVPROC)IntGetProcAddress("glVertexAttribI3uiv");
		VertexAttribI3uiv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4bv(GLuint index, const GLbyte * v)
	{
		VertexAttribI4bv = (PFNVERTEXATTRIBI4BVPROC)IntGetProcAddress("glVertexAttribI4bv");
		VertexAttribI4bv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w)
	{
		VertexAttribI4i = (PFNVERTEXATTRIBI4IPROC)IntGetProcAddress("glVertexAttribI4i");
		VertexAttribI4i(index, x, y, z, w);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4iv(GLuint index, const GLint * v)
	{
		VertexAttribI4iv = (PFNVERTEXATTRIBI4IVPROC)IntGetProcAddress("glVertexAttribI4iv");
		VertexAttribI4iv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4sv(GLuint index, const GLshort * v)
	{
		VertexAttribI4sv = (PFNVERTEXATTRIBI4SVPROC)IntGetProcAddress("glVertexAttribI4sv");
		VertexAttribI4sv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4ubv(GLuint index, const GLubyte * v)
	{
		VertexAttribI4ubv = (PFNVERTEXATTRIBI4UBVPROC)IntGetProcAddress("glVertexAttribI4ubv");
		VertexAttribI4ubv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
	{
		VertexAttribI4ui = (PFNVERTEXATTRIBI4UIPROC)IntGetProcAddress("glVertexAttribI4ui");
		VertexAttribI4ui(index, x, y, z, w);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4uiv(GLuint index, const GLuint * v)
	{
		VertexAttribI4uiv = (PFNVERTEXATTRIBI4UIVPROC)IntGetProcAddress("glVertexAttribI4uiv");
		VertexAttribI4uiv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribI4usv(GLuint index, const GLushort * v)
	{
		VertexAttribI4usv = (PFNVERTEXATTRIBI4USVPROC)IntGetProcAddress("glVertexAttribI4usv");
		VertexAttribI4usv(index, v);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
	{
		VertexAttribIPointer = (PFNVERTEXATTRIBIPOINTERPROC)IntGetProcAddress("glVertexAttribIPointer");
		VertexAttribIPointer(index, size, type, stride, pointer);
	}

	
	// Extension: 3.1
	static void CODEGEN_FUNCPTR Switch_CopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
	{
		CopyBufferSubData = (PFNCOPYBUFFERSUBDATAPROC)IntGetProcAddress("glCopyBufferSubData");
		CopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
	}

	static void CODEGEN_FUNCPTR Switch_DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
	{
		DrawArraysInstanced = (PFNDRAWARRAYSINSTANCEDPROC)IntGetProcAddress("glDrawArraysInstanced");
		DrawArraysInstanced(mode, first, count, instancecount);
	}

	static void CODEGEN_FUNCPTR Switch_DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei instancecount)
	{
		DrawElementsInstanced = (PFNDRAWELEMENTSINSTANCEDPROC)IntGetProcAddress("glDrawElementsInstanced");
		DrawElementsInstanced(mode, count, type, indices, instancecount);
	}

	static void CODEGEN_FUNCPTR Switch_GetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName)
	{
		GetActiveUniformBlockName = (PFNGETACTIVEUNIFORMBLOCKNAMEPROC)IntGetProcAddress("glGetActiveUniformBlockName");
		GetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
	}

	static void CODEGEN_FUNCPTR Switch_GetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint * params)
	{
		GetActiveUniformBlockiv = (PFNGETACTIVEUNIFORMBLOCKIVPROC)IntGetProcAddress("glGetActiveUniformBlockiv");
		GetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformName)
	{
		GetActiveUniformName = (PFNGETACTIVEUNIFORMNAMEPROC)IntGetProcAddress("glGetActiveUniformName");
		GetActiveUniformName(program, uniformIndex, bufSize, length, uniformName);
	}

	static void CODEGEN_FUNCPTR Switch_GetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint * params)
	{
		GetActiveUniformsiv = (PFNGETACTIVEUNIFORMSIVPROC)IntGetProcAddress("glGetActiveUniformsiv");
		GetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
	}

	static GLuint CODEGEN_FUNCPTR Switch_GetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName)
	{
		GetUniformBlockIndex = (PFNGETUNIFORMBLOCKINDEXPROC)IntGetProcAddress("glGetUniformBlockIndex");
		return GetUniformBlockIndex(program, uniformBlockName);
	}

	static void CODEGEN_FUNCPTR Switch_GetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const* uniformNames, GLuint * uniformIndices)
	{
		GetUniformIndices = (PFNGETUNIFORMINDICESPROC)IntGetProcAddress("glGetUniformIndices");
		GetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
	}

	static void CODEGEN_FUNCPTR Switch_PrimitiveRestartIndex(GLuint index)
	{
		PrimitiveRestartIndex = (PFNPRIMITIVERESTARTINDEXPROC)IntGetProcAddress("glPrimitiveRestartIndex");
		PrimitiveRestartIndex(index);
	}

	static void CODEGEN_FUNCPTR Switch_TexBuffer(GLenum target, GLenum internalformat, GLuint buffer)
	{
		TexBuffer = (PFNTEXBUFFERPROC)IntGetProcAddress("glTexBuffer");
		TexBuffer(target, internalformat, buffer);
	}

	static void CODEGEN_FUNCPTR Switch_UniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
	{
		UniformBlockBinding = (PFNUNIFORMBLOCKBINDINGPROC)IntGetProcAddress("glUniformBlockBinding");
		UniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
	}

	
	// Extension: 3.2
	static GLenum CODEGEN_FUNCPTR Switch_ClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
	{
		ClientWaitSync = (PFNCLIENTWAITSYNCPROC)IntGetProcAddress("glClientWaitSync");
		return ClientWaitSync(sync, flags, timeout);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteSync(GLsync sync)
	{
		DeleteSync = (PFNDELETESYNCPROC)IntGetProcAddress("glDeleteSync");
		DeleteSync(sync);
	}

	static void CODEGEN_FUNCPTR Switch_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLint basevertex)
	{
		DrawElementsBaseVertex = (PFNDRAWELEMENTSBASEVERTEXPROC)IntGetProcAddress("glDrawElementsBaseVertex");
		DrawElementsBaseVertex(mode, count, type, indices, basevertex);
	}

	static void CODEGEN_FUNCPTR Switch_DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei instancecount, GLint basevertex)
	{
		DrawElementsInstancedBaseVertex = (PFNDRAWELEMENTSINSTANCEDBASEVERTEXPROC)IntGetProcAddress("glDrawElementsInstancedBaseVertex");
		DrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
	}

	static void CODEGEN_FUNCPTR Switch_DrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices, GLint basevertex)
	{
		DrawRangeElementsBaseVertex = (PFNDRAWRANGEELEMENTSBASEVERTEXPROC)IntGetProcAddress("glDrawRangeElementsBaseVertex");
		DrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
	}

	static GLsync CODEGEN_FUNCPTR Switch_FenceSync(GLenum condition, GLbitfield flags)
	{
		FenceSync = (PFNFENCESYNCPROC)IntGetProcAddress("glFenceSync");
		return FenceSync(condition, flags);
	}

	static void CODEGEN_FUNCPTR Switch_FramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)
	{
		FramebufferTexture = (PFNFRAMEBUFFERTEXTUREPROC)IntGetProcAddress("glFramebufferTexture");
		FramebufferTexture(target, attachment, texture, level);
	}

	static void CODEGEN_FUNCPTR Switch_GetBufferParameteri64v(GLenum target, GLenum pname, GLint64 * params)
	{
		GetBufferParameteri64v = (PFNGETBUFFERPARAMETERI64VPROC)IntGetProcAddress("glGetBufferParameteri64v");
		GetBufferParameteri64v(target, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetInteger64i_v(GLenum target, GLuint index, GLint64 * data)
	{
		GetInteger64i_v = (PFNGETINTEGER64I_VPROC)IntGetProcAddress("glGetInteger64i_v");
		GetInteger64i_v(target, index, data);
	}

	static void CODEGEN_FUNCPTR Switch_GetInteger64v(GLenum pname, GLint64 * params)
	{
		GetInteger64v = (PFNGETINTEGER64VPROC)IntGetProcAddress("glGetInteger64v");
		GetInteger64v(pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetMultisamplefv(GLenum pname, GLuint index, GLfloat * val)
	{
		GetMultisamplefv = (PFNGETMULTISAMPLEFVPROC)IntGetProcAddress("glGetMultisamplefv");
		GetMultisamplefv(pname, index, val);
	}

	static void CODEGEN_FUNCPTR Switch_GetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei * length, GLint * values)
	{
		GetSynciv = (PFNGETSYNCIVPROC)IntGetProcAddress("glGetSynciv");
		GetSynciv(sync, pname, bufSize, length, values);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsSync(GLsync sync)
	{
		IsSync = (PFNISSYNCPROC)IntGetProcAddress("glIsSync");
		return IsSync(sync);
	}

	static void CODEGEN_FUNCPTR Switch_MultiDrawElementsBaseVertex(GLenum mode, const GLsizei * count, GLenum type, const GLvoid *const* indices, GLsizei drawcount, const GLint * basevertex)
	{
		MultiDrawElementsBaseVertex = (PFNMULTIDRAWELEMENTSBASEVERTEXPROC)IntGetProcAddress("glMultiDrawElementsBaseVertex");
		MultiDrawElementsBaseVertex(mode, count, type, indices, drawcount, basevertex);
	}

	static void CODEGEN_FUNCPTR Switch_ProvokingVertex(GLenum mode)
	{
		ProvokingVertex = (PFNPROVOKINGVERTEXPROC)IntGetProcAddress("glProvokingVertex");
		ProvokingVertex(mode);
	}

	static void CODEGEN_FUNCPTR Switch_SampleMaski(GLuint index, GLbitfield mask)
	{
		SampleMaski = (PFNSAMPLEMASKIPROC)IntGetProcAddress("glSampleMaski");
		SampleMaski(index, mask);
	}

	static void CODEGEN_FUNCPTR Switch_TexImage2DMultisample(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
	{
		TexImage2DMultisample = (PFNTEXIMAGE2DMULTISAMPLEPROC)IntGetProcAddress("glTexImage2DMultisample");
		TexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
	}

	static void CODEGEN_FUNCPTR Switch_TexImage3DMultisample(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations)
	{
		TexImage3DMultisample = (PFNTEXIMAGE3DMULTISAMPLEPROC)IntGetProcAddress("glTexImage3DMultisample");
		TexImage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
	}

	static void CODEGEN_FUNCPTR Switch_WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
	{
		WaitSync = (PFNWAITSYNCPROC)IntGetProcAddress("glWaitSync");
		WaitSync(sync, flags, timeout);
	}

	
	// Extension: 3.3
	static void CODEGEN_FUNCPTR Switch_BindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar * name)
	{
		BindFragDataLocationIndexed = (PFNBINDFRAGDATALOCATIONINDEXEDPROC)IntGetProcAddress("glBindFragDataLocationIndexed");
		BindFragDataLocationIndexed(program, colorNumber, index, name);
	}

	static void CODEGEN_FUNCPTR Switch_BindSampler(GLuint unit, GLuint sampler)
	{
		BindSampler = (PFNBINDSAMPLERPROC)IntGetProcAddress("glBindSampler");
		BindSampler(unit, sampler);
	}

	static void CODEGEN_FUNCPTR Switch_DeleteSamplers(GLsizei count, const GLuint * samplers)
	{
		DeleteSamplers = (PFNDELETESAMPLERSPROC)IntGetProcAddress("glDeleteSamplers");
		DeleteSamplers(count, samplers);
	}

	static void CODEGEN_FUNCPTR Switch_GenSamplers(GLsizei count, GLuint * samplers)
	{
		GenSamplers = (PFNGENSAMPLERSPROC)IntGetProcAddress("glGenSamplers");
		GenSamplers(count, samplers);
	}

	static GLint CODEGEN_FUNCPTR Switch_GetFragDataIndex(GLuint program, const GLchar * name)
	{
		GetFragDataIndex = (PFNGETFRAGDATAINDEXPROC)IntGetProcAddress("glGetFragDataIndex");
		return GetFragDataIndex(program, name);
	}

	static void CODEGEN_FUNCPTR Switch_GetQueryObjecti64v(GLuint id, GLenum pname, GLint64 * params)
	{
		GetQueryObjecti64v = (PFNGETQUERYOBJECTI64VPROC)IntGetProcAddress("glGetQueryObjecti64v");
		GetQueryObjecti64v(id, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 * params)
	{
		GetQueryObjectui64v = (PFNGETQUERYOBJECTUI64VPROC)IntGetProcAddress("glGetQueryObjectui64v");
		GetQueryObjectui64v(id, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint * params)
	{
		GetSamplerParameterIiv = (PFNGETSAMPLERPARAMETERIIVPROC)IntGetProcAddress("glGetSamplerParameterIiv");
		GetSamplerParameterIiv(sampler, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint * params)
	{
		GetSamplerParameterIuiv = (PFNGETSAMPLERPARAMETERIUIVPROC)IntGetProcAddress("glGetSamplerParameterIuiv");
		GetSamplerParameterIuiv(sampler, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat * params)
	{
		GetSamplerParameterfv = (PFNGETSAMPLERPARAMETERFVPROC)IntGetProcAddress("glGetSamplerParameterfv");
		GetSamplerParameterfv(sampler, pname, params);
	}

	static void CODEGEN_FUNCPTR Switch_GetSamplerParameteriv(GLuint sampler, GLenum pname, GLint * params)
	{
		GetSamplerParameteriv = (PFNGETSAMPLERPARAMETERIVPROC)IntGetProcAddress("glGetSamplerParameteriv");
		GetSamplerParameteriv(sampler, pname, params);
	}

	static GLboolean CODEGEN_FUNCPTR Switch_IsSampler(GLuint sampler)
	{
		IsSampler = (PFNISSAMPLERPROC)IntGetProcAddress("glIsSampler");
		return IsSampler(sampler);
	}

	static void CODEGEN_FUNCPTR Switch_QueryCounter(GLuint id, GLenum target)
	{
		QueryCounter = (PFNQUERYCOUNTERPROC)IntGetProcAddress("glQueryCounter");
		QueryCounter(id, target);
	}

	static void CODEGEN_FUNCPTR Switch_SamplerParameterIiv(GLuint sampler, GLenum pname, const GLint * param)
	{
		SamplerParameterIiv = (PFNSAMPLERPARAMETERIIVPROC)IntGetProcAddress("glSamplerParameterIiv");
		SamplerParameterIiv(sampler, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_SamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint * param)
	{
		SamplerParameterIuiv = (PFNSAMPLERPARAMETERIUIVPROC)IntGetProcAddress("glSamplerParameterIuiv");
		SamplerParameterIuiv(sampler, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_SamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
	{
		SamplerParameterf = (PFNSAMPLERPARAMETERFPROC)IntGetProcAddress("glSamplerParameterf");
		SamplerParameterf(sampler, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_SamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param)
	{
		SamplerParameterfv = (PFNSAMPLERPARAMETERFVPROC)IntGetProcAddress("glSamplerParameterfv");
		SamplerParameterfv(sampler, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_SamplerParameteri(GLuint sampler, GLenum pname, GLint param)
	{
		SamplerParameteri = (PFNSAMPLERPARAMETERIPROC)IntGetProcAddress("glSamplerParameteri");
		SamplerParameteri(sampler, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_SamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param)
	{
		SamplerParameteriv = (PFNSAMPLERPARAMETERIVPROC)IntGetProcAddress("glSamplerParameteriv");
		SamplerParameteriv(sampler, pname, param);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribDivisor(GLuint index, GLuint divisor)
	{
		VertexAttribDivisor = (PFNVERTEXATTRIBDIVISORPROC)IntGetProcAddress("glVertexAttribDivisor");
		VertexAttribDivisor(index, divisor);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value)
	{
		VertexAttribP1ui = (PFNVERTEXATTRIBP1UIPROC)IntGetProcAddress("glVertexAttribP1ui");
		VertexAttribP1ui(index, type, normalized, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
	{
		VertexAttribP1uiv = (PFNVERTEXATTRIBP1UIVPROC)IntGetProcAddress("glVertexAttribP1uiv");
		VertexAttribP1uiv(index, type, normalized, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value)
	{
		VertexAttribP2ui = (PFNVERTEXATTRIBP2UIPROC)IntGetProcAddress("glVertexAttribP2ui");
		VertexAttribP2ui(index, type, normalized, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
	{
		VertexAttribP2uiv = (PFNVERTEXATTRIBP2UIVPROC)IntGetProcAddress("glVertexAttribP2uiv");
		VertexAttribP2uiv(index, type, normalized, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value)
	{
		VertexAttribP3ui = (PFNVERTEXATTRIBP3UIPROC)IntGetProcAddress("glVertexAttribP3ui");
		VertexAttribP3ui(index, type, normalized, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
	{
		VertexAttribP3uiv = (PFNVERTEXATTRIBP3UIVPROC)IntGetProcAddress("glVertexAttribP3uiv");
		VertexAttribP3uiv(index, type, normalized, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value)
	{
		VertexAttribP4ui = (PFNVERTEXATTRIBP4UIPROC)IntGetProcAddress("glVertexAttribP4ui");
		VertexAttribP4ui(index, type, normalized, value);
	}

	static void CODEGEN_FUNCPTR Switch_VertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
	{
		VertexAttribP4uiv = (PFNVERTEXATTRIBP4UIVPROC)IntGetProcAddress("glVertexAttribP4uiv");
		VertexAttribP4uiv(index, type, normalized, value);
	}

	
	
	namespace 
	{
		struct InitializeVariables
		{
			InitializeVariables()
			{
				// Extension: 1.0
				BlendFunc = Switch_BlendFunc;
				Clear = Switch_Clear;
				ClearColor = Switch_ClearColor;
				ClearDepth = Switch_ClearDepth;
				ClearStencil = Switch_ClearStencil;
				ColorMask = Switch_ColorMask;
				CullFace = Switch_CullFace;
				DepthFunc = Switch_DepthFunc;
				DepthMask = Switch_DepthMask;
				DepthRange = Switch_DepthRange;
				Disable = Switch_Disable;
				DrawBuffer = Switch_DrawBuffer;
				Enable = Switch_Enable;
				Finish = Switch_Finish;
				Flush = Switch_Flush;
				FrontFace = Switch_FrontFace;
				GetBooleanv = Switch_GetBooleanv;
				GetDoublev = Switch_GetDoublev;
				GetError = Switch_GetError;
				GetFloatv = Switch_GetFloatv;
				GetIntegerv = Switch_GetIntegerv;
				GetString = Switch_GetString;
				GetTexImage = Switch_GetTexImage;
				GetTexLevelParameterfv = Switch_GetTexLevelParameterfv;
				GetTexLevelParameteriv = Switch_GetTexLevelParameteriv;
				GetTexParameterfv = Switch_GetTexParameterfv;
				GetTexParameteriv = Switch_GetTexParameteriv;
				Hint = Switch_Hint;
				IsEnabled = Switch_IsEnabled;
				LineWidth = Switch_LineWidth;
				LogicOp = Switch_LogicOp;
				PixelStoref = Switch_PixelStoref;
				PixelStorei = Switch_PixelStorei;
				PointSize = Switch_PointSize;
				PolygonMode = Switch_PolygonMode;
				ReadBuffer = Switch_ReadBuffer;
				ReadPixels = Switch_ReadPixels;
				Scissor = Switch_Scissor;
				StencilFunc = Switch_StencilFunc;
				StencilMask = Switch_StencilMask;
				StencilOp = Switch_StencilOp;
				TexImage1D = Switch_TexImage1D;
				TexImage2D = Switch_TexImage2D;
				TexParameterf = Switch_TexParameterf;
				TexParameterfv = Switch_TexParameterfv;
				TexParameteri = Switch_TexParameteri;
				TexParameteriv = Switch_TexParameteriv;
				Viewport = Switch_Viewport;
				
				// Extension: 1.1
				BindTexture = Switch_BindTexture;
				CopyTexImage1D = Switch_CopyTexImage1D;
				CopyTexImage2D = Switch_CopyTexImage2D;
				CopyTexSubImage1D = Switch_CopyTexSubImage1D;
				CopyTexSubImage2D = Switch_CopyTexSubImage2D;
				DeleteTextures = Switch_DeleteTextures;
				DrawArrays = Switch_DrawArrays;
				DrawElements = Switch_DrawElements;
				GenTextures = Switch_GenTextures;
				IsTexture = Switch_IsTexture;
				PolygonOffset = Switch_PolygonOffset;
				TexSubImage1D = Switch_TexSubImage1D;
				TexSubImage2D = Switch_TexSubImage2D;
				
				// Extension: 1.2
				BlendColor = Switch_BlendColor;
				BlendEquation = Switch_BlendEquation;
				CopyTexSubImage3D = Switch_CopyTexSubImage3D;
				DrawRangeElements = Switch_DrawRangeElements;
				TexImage3D = Switch_TexImage3D;
				TexSubImage3D = Switch_TexSubImage3D;
				
				// Extension: 1.3
				ActiveTexture = Switch_ActiveTexture;
				CompressedTexImage1D = Switch_CompressedTexImage1D;
				CompressedTexImage2D = Switch_CompressedTexImage2D;
				CompressedTexImage3D = Switch_CompressedTexImage3D;
				CompressedTexSubImage1D = Switch_CompressedTexSubImage1D;
				CompressedTexSubImage2D = Switch_CompressedTexSubImage2D;
				CompressedTexSubImage3D = Switch_CompressedTexSubImage3D;
				GetCompressedTexImage = Switch_GetCompressedTexImage;
				SampleCoverage = Switch_SampleCoverage;
				
				// Extension: 1.4
				BlendFuncSeparate = Switch_BlendFuncSeparate;
				MultiDrawArrays = Switch_MultiDrawArrays;
				MultiDrawElements = Switch_MultiDrawElements;
				PointParameterf = Switch_PointParameterf;
				PointParameterfv = Switch_PointParameterfv;
				PointParameteri = Switch_PointParameteri;
				PointParameteriv = Switch_PointParameteriv;
				
				// Extension: 1.5
				BeginQuery = Switch_BeginQuery;
				BindBuffer = Switch_BindBuffer;
				BufferData = Switch_BufferData;
				BufferSubData = Switch_BufferSubData;
				DeleteBuffers = Switch_DeleteBuffers;
				DeleteQueries = Switch_DeleteQueries;
				EndQuery = Switch_EndQuery;
				GenBuffers = Switch_GenBuffers;
				GenQueries = Switch_GenQueries;
				GetBufferParameteriv = Switch_GetBufferParameteriv;
				GetBufferPointerv = Switch_GetBufferPointerv;
				GetBufferSubData = Switch_GetBufferSubData;
				GetQueryObjectiv = Switch_GetQueryObjectiv;
				GetQueryObjectuiv = Switch_GetQueryObjectuiv;
				GetQueryiv = Switch_GetQueryiv;
				IsBuffer = Switch_IsBuffer;
				IsQuery = Switch_IsQuery;
				MapBuffer = Switch_MapBuffer;
				UnmapBuffer = Switch_UnmapBuffer;
				
				// Extension: 2.0
				AttachShader = Switch_AttachShader;
				BindAttribLocation = Switch_BindAttribLocation;
				BlendEquationSeparate = Switch_BlendEquationSeparate;
				CompileShader = Switch_CompileShader;
				CreateProgram = Switch_CreateProgram;
				CreateShader = Switch_CreateShader;
				DeleteProgram = Switch_DeleteProgram;
				DeleteShader = Switch_DeleteShader;
				DetachShader = Switch_DetachShader;
				DisableVertexAttribArray = Switch_DisableVertexAttribArray;
				DrawBuffers = Switch_DrawBuffers;
				EnableVertexAttribArray = Switch_EnableVertexAttribArray;
				GetActiveAttrib = Switch_GetActiveAttrib;
				GetActiveUniform = Switch_GetActiveUniform;
				GetAttachedShaders = Switch_GetAttachedShaders;
				GetAttribLocation = Switch_GetAttribLocation;
				GetProgramInfoLog = Switch_GetProgramInfoLog;
				GetProgramiv = Switch_GetProgramiv;
				GetShaderInfoLog = Switch_GetShaderInfoLog;
				GetShaderSource = Switch_GetShaderSource;
				GetShaderiv = Switch_GetShaderiv;
				GetUniformLocation = Switch_GetUniformLocation;
				GetUniformfv = Switch_GetUniformfv;
				GetUniformiv = Switch_GetUniformiv;
				GetVertexAttribPointerv = Switch_GetVertexAttribPointerv;
				GetVertexAttribdv = Switch_GetVertexAttribdv;
				GetVertexAttribfv = Switch_GetVertexAttribfv;
				GetVertexAttribiv = Switch_GetVertexAttribiv;
				IsProgram = Switch_IsProgram;
				IsShader = Switch_IsShader;
				LinkProgram = Switch_LinkProgram;
				ShaderSource = Switch_ShaderSource;
				StencilFuncSeparate = Switch_StencilFuncSeparate;
				StencilMaskSeparate = Switch_StencilMaskSeparate;
				StencilOpSeparate = Switch_StencilOpSeparate;
				Uniform1f = Switch_Uniform1f;
				Uniform1fv = Switch_Uniform1fv;
				Uniform1i = Switch_Uniform1i;
				Uniform1iv = Switch_Uniform1iv;
				Uniform2f = Switch_Uniform2f;
				Uniform2fv = Switch_Uniform2fv;
				Uniform2i = Switch_Uniform2i;
				Uniform2iv = Switch_Uniform2iv;
				Uniform3f = Switch_Uniform3f;
				Uniform3fv = Switch_Uniform3fv;
				Uniform3i = Switch_Uniform3i;
				Uniform3iv = Switch_Uniform3iv;
				Uniform4f = Switch_Uniform4f;
				Uniform4fv = Switch_Uniform4fv;
				Uniform4i = Switch_Uniform4i;
				Uniform4iv = Switch_Uniform4iv;
				UniformMatrix2fv = Switch_UniformMatrix2fv;
				UniformMatrix3fv = Switch_UniformMatrix3fv;
				UniformMatrix4fv = Switch_UniformMatrix4fv;
				UseProgram = Switch_UseProgram;
				ValidateProgram = Switch_ValidateProgram;
				VertexAttrib1d = Switch_VertexAttrib1d;
				VertexAttrib1dv = Switch_VertexAttrib1dv;
				VertexAttrib1f = Switch_VertexAttrib1f;
				VertexAttrib1fv = Switch_VertexAttrib1fv;
				VertexAttrib1s = Switch_VertexAttrib1s;
				VertexAttrib1sv = Switch_VertexAttrib1sv;
				VertexAttrib2d = Switch_VertexAttrib2d;
				VertexAttrib2dv = Switch_VertexAttrib2dv;
				VertexAttrib2f = Switch_VertexAttrib2f;
				VertexAttrib2fv = Switch_VertexAttrib2fv;
				VertexAttrib2s = Switch_VertexAttrib2s;
				VertexAttrib2sv = Switch_VertexAttrib2sv;
				VertexAttrib3d = Switch_VertexAttrib3d;
				VertexAttrib3dv = Switch_VertexAttrib3dv;
				VertexAttrib3f = Switch_VertexAttrib3f;
				VertexAttrib3fv = Switch_VertexAttrib3fv;
				VertexAttrib3s = Switch_VertexAttrib3s;
				VertexAttrib3sv = Switch_VertexAttrib3sv;
				VertexAttrib4Nbv = Switch_VertexAttrib4Nbv;
				VertexAttrib4Niv = Switch_VertexAttrib4Niv;
				VertexAttrib4Nsv = Switch_VertexAttrib4Nsv;
				VertexAttrib4Nub = Switch_VertexAttrib4Nub;
				VertexAttrib4Nubv = Switch_VertexAttrib4Nubv;
				VertexAttrib4Nuiv = Switch_VertexAttrib4Nuiv;
				VertexAttrib4Nusv = Switch_VertexAttrib4Nusv;
				VertexAttrib4bv = Switch_VertexAttrib4bv;
				VertexAttrib4d = Switch_VertexAttrib4d;
				VertexAttrib4dv = Switch_VertexAttrib4dv;
				VertexAttrib4f = Switch_VertexAttrib4f;
				VertexAttrib4fv = Switch_VertexAttrib4fv;
				VertexAttrib4iv = Switch_VertexAttrib4iv;
				VertexAttrib4s = Switch_VertexAttrib4s;
				VertexAttrib4sv = Switch_VertexAttrib4sv;
				VertexAttrib4ubv = Switch_VertexAttrib4ubv;
				VertexAttrib4uiv = Switch_VertexAttrib4uiv;
				VertexAttrib4usv = Switch_VertexAttrib4usv;
				VertexAttribPointer = Switch_VertexAttribPointer;
				
				// Extension: 2.1
				UniformMatrix2x3fv = Switch_UniformMatrix2x3fv;
				UniformMatrix2x4fv = Switch_UniformMatrix2x4fv;
				UniformMatrix3x2fv = Switch_UniformMatrix3x2fv;
				UniformMatrix3x4fv = Switch_UniformMatrix3x4fv;
				UniformMatrix4x2fv = Switch_UniformMatrix4x2fv;
				UniformMatrix4x3fv = Switch_UniformMatrix4x3fv;
				
				// Extension: 3.0
				BeginConditionalRender = Switch_BeginConditionalRender;
				BeginTransformFeedback = Switch_BeginTransformFeedback;
				BindBufferBase = Switch_BindBufferBase;
				BindBufferRange = Switch_BindBufferRange;
				BindFragDataLocation = Switch_BindFragDataLocation;
				BindFramebuffer = Switch_BindFramebuffer;
				BindRenderbuffer = Switch_BindRenderbuffer;
				BindVertexArray = Switch_BindVertexArray;
				BlitFramebuffer = Switch_BlitFramebuffer;
				CheckFramebufferStatus = Switch_CheckFramebufferStatus;
				ClampColor = Switch_ClampColor;
				ClearBufferfi = Switch_ClearBufferfi;
				ClearBufferfv = Switch_ClearBufferfv;
				ClearBufferiv = Switch_ClearBufferiv;
				ClearBufferuiv = Switch_ClearBufferuiv;
				ColorMaski = Switch_ColorMaski;
				DeleteFramebuffers = Switch_DeleteFramebuffers;
				DeleteRenderbuffers = Switch_DeleteRenderbuffers;
				DeleteVertexArrays = Switch_DeleteVertexArrays;
				Disablei = Switch_Disablei;
				Enablei = Switch_Enablei;
				EndConditionalRender = Switch_EndConditionalRender;
				EndTransformFeedback = Switch_EndTransformFeedback;
				FlushMappedBufferRange = Switch_FlushMappedBufferRange;
				FramebufferRenderbuffer = Switch_FramebufferRenderbuffer;
				FramebufferTexture1D = Switch_FramebufferTexture1D;
				FramebufferTexture2D = Switch_FramebufferTexture2D;
				FramebufferTexture3D = Switch_FramebufferTexture3D;
				FramebufferTextureLayer = Switch_FramebufferTextureLayer;
				GenFramebuffers = Switch_GenFramebuffers;
				GenRenderbuffers = Switch_GenRenderbuffers;
				GenVertexArrays = Switch_GenVertexArrays;
				GenerateMipmap = Switch_GenerateMipmap;
				GetBooleani_v = Switch_GetBooleani_v;
				GetFragDataLocation = Switch_GetFragDataLocation;
				GetFramebufferAttachmentParameteriv = Switch_GetFramebufferAttachmentParameteriv;
				GetIntegeri_v = Switch_GetIntegeri_v;
				GetRenderbufferParameteriv = Switch_GetRenderbufferParameteriv;
				GetStringi = Switch_GetStringi;
				GetTexParameterIiv = Switch_GetTexParameterIiv;
				GetTexParameterIuiv = Switch_GetTexParameterIuiv;
				GetTransformFeedbackVarying = Switch_GetTransformFeedbackVarying;
				GetUniformuiv = Switch_GetUniformuiv;
				GetVertexAttribIiv = Switch_GetVertexAttribIiv;
				GetVertexAttribIuiv = Switch_GetVertexAttribIuiv;
				IsEnabledi = Switch_IsEnabledi;
				IsFramebuffer = Switch_IsFramebuffer;
				IsRenderbuffer = Switch_IsRenderbuffer;
				IsVertexArray = Switch_IsVertexArray;
				MapBufferRange = Switch_MapBufferRange;
				RenderbufferStorage = Switch_RenderbufferStorage;
				RenderbufferStorageMultisample = Switch_RenderbufferStorageMultisample;
				TexParameterIiv = Switch_TexParameterIiv;
				TexParameterIuiv = Switch_TexParameterIuiv;
				TransformFeedbackVaryings = Switch_TransformFeedbackVaryings;
				Uniform1ui = Switch_Uniform1ui;
				Uniform1uiv = Switch_Uniform1uiv;
				Uniform2ui = Switch_Uniform2ui;
				Uniform2uiv = Switch_Uniform2uiv;
				Uniform3ui = Switch_Uniform3ui;
				Uniform3uiv = Switch_Uniform3uiv;
				Uniform4ui = Switch_Uniform4ui;
				Uniform4uiv = Switch_Uniform4uiv;
				VertexAttribI1i = Switch_VertexAttribI1i;
				VertexAttribI1iv = Switch_VertexAttribI1iv;
				VertexAttribI1ui = Switch_VertexAttribI1ui;
				VertexAttribI1uiv = Switch_VertexAttribI1uiv;
				VertexAttribI2i = Switch_VertexAttribI2i;
				VertexAttribI2iv = Switch_VertexAttribI2iv;
				VertexAttribI2ui = Switch_VertexAttribI2ui;
				VertexAttribI2uiv = Switch_VertexAttribI2uiv;
				VertexAttribI3i = Switch_VertexAttribI3i;
				VertexAttribI3iv = Switch_VertexAttribI3iv;
				VertexAttribI3ui = Switch_VertexAttribI3ui;
				VertexAttribI3uiv = Switch_VertexAttribI3uiv;
				VertexAttribI4bv = Switch_VertexAttribI4bv;
				VertexAttribI4i = Switch_VertexAttribI4i;
				VertexAttribI4iv = Switch_VertexAttribI4iv;
				VertexAttribI4sv = Switch_VertexAttribI4sv;
				VertexAttribI4ubv = Switch_VertexAttribI4ubv;
				VertexAttribI4ui = Switch_VertexAttribI4ui;
				VertexAttribI4uiv = Switch_VertexAttribI4uiv;
				VertexAttribI4usv = Switch_VertexAttribI4usv;
				VertexAttribIPointer = Switch_VertexAttribIPointer;
				
				// Extension: 3.1
				CopyBufferSubData = Switch_CopyBufferSubData;
				DrawArraysInstanced = Switch_DrawArraysInstanced;
				DrawElementsInstanced = Switch_DrawElementsInstanced;
				GetActiveUniformBlockName = Switch_GetActiveUniformBlockName;
				GetActiveUniformBlockiv = Switch_GetActiveUniformBlockiv;
				GetActiveUniformName = Switch_GetActiveUniformName;
				GetActiveUniformsiv = Switch_GetActiveUniformsiv;
				GetUniformBlockIndex = Switch_GetUniformBlockIndex;
				GetUniformIndices = Switch_GetUniformIndices;
				PrimitiveRestartIndex = Switch_PrimitiveRestartIndex;
				TexBuffer = Switch_TexBuffer;
				UniformBlockBinding = Switch_UniformBlockBinding;
				
				// Extension: 3.2
				ClientWaitSync = Switch_ClientWaitSync;
				DeleteSync = Switch_DeleteSync;
				DrawElementsBaseVertex = Switch_DrawElementsBaseVertex;
				DrawElementsInstancedBaseVertex = Switch_DrawElementsInstancedBaseVertex;
				DrawRangeElementsBaseVertex = Switch_DrawRangeElementsBaseVertex;
				FenceSync = Switch_FenceSync;
				FramebufferTexture = Switch_FramebufferTexture;
				GetBufferParameteri64v = Switch_GetBufferParameteri64v;
				GetInteger64i_v = Switch_GetInteger64i_v;
				GetInteger64v = Switch_GetInteger64v;
				GetMultisamplefv = Switch_GetMultisamplefv;
				GetSynciv = Switch_GetSynciv;
				IsSync = Switch_IsSync;
				MultiDrawElementsBaseVertex = Switch_MultiDrawElementsBaseVertex;
				ProvokingVertex = Switch_ProvokingVertex;
				SampleMaski = Switch_SampleMaski;
				TexImage2DMultisample = Switch_TexImage2DMultisample;
				TexImage3DMultisample = Switch_TexImage3DMultisample;
				WaitSync = Switch_WaitSync;
				
				// Extension: 3.3
				BindFragDataLocationIndexed = Switch_BindFragDataLocationIndexed;
				BindSampler = Switch_BindSampler;
				DeleteSamplers = Switch_DeleteSamplers;
				GenSamplers = Switch_GenSamplers;
				GetFragDataIndex = Switch_GetFragDataIndex;
				GetQueryObjecti64v = Switch_GetQueryObjecti64v;
				GetQueryObjectui64v = Switch_GetQueryObjectui64v;
				GetSamplerParameterIiv = Switch_GetSamplerParameterIiv;
				GetSamplerParameterIuiv = Switch_GetSamplerParameterIuiv;
				GetSamplerParameterfv = Switch_GetSamplerParameterfv;
				GetSamplerParameteriv = Switch_GetSamplerParameteriv;
				IsSampler = Switch_IsSampler;
				QueryCounter = Switch_QueryCounter;
				SamplerParameterIiv = Switch_SamplerParameterIiv;
				SamplerParameterIuiv = Switch_SamplerParameterIuiv;
				SamplerParameterf = Switch_SamplerParameterf;
				SamplerParameterfv = Switch_SamplerParameterfv;
				SamplerParameteri = Switch_SamplerParameteri;
				SamplerParameteriv = Switch_SamplerParameteriv;
				VertexAttribDivisor = Switch_VertexAttribDivisor;
				VertexAttribP1ui = Switch_VertexAttribP1ui;
				VertexAttribP1uiv = Switch_VertexAttribP1uiv;
				VertexAttribP2ui = Switch_VertexAttribP2ui;
				VertexAttribP2uiv = Switch_VertexAttribP2uiv;
				VertexAttribP3ui = Switch_VertexAttribP3ui;
				VertexAttribP3uiv = Switch_VertexAttribP3uiv;
				VertexAttribP4ui = Switch_VertexAttribP4ui;
				VertexAttribP4uiv = Switch_VertexAttribP4uiv;
				
			}
		};

		InitializeVariables g_initVariables;
	}
	
	namespace sys
	{
		namespace 
		{
			void ClearExtensionVariables()
			{
			}
			
			struct MapEntry
			{
				const char *extName;
				bool *extVariable;
			};
			
			struct MapCompare
			{
				MapCompare(const char *test_) : test(test_) {}
				bool operator()(const MapEntry &other) { return strcmp(test, other.extName) == 0; }
				const char *test;
			};
			
			struct ClearEntry
			{
			  void operator()(MapEntry &entry) { *(entry.extVariable) = false;}
			};
			
			MapEntry g_mappingTable[1]; //This is intensionally left uninitialized. 
			
			void LoadExtByName(const char *extensionName)
			{
				MapEntry *tableEnd = &g_mappingTable[0];
				MapEntry *entry = std::find_if(&g_mappingTable[0], tableEnd, MapCompare(extensionName));
				
				if(entry != tableEnd)
					*(entry->extVariable) = true;
			}
			
			void ProcExtsFromExtList()
			{
				GLint iLoop;
				GLint iNumExtensions = 0;
				GetIntegerv(NUM_EXTENSIONS, &iNumExtensions);
			
				for(iLoop = 0; iLoop < iNumExtensions; iLoop++)
				{
					const char *strExtensionName = (const char *)GetStringi(EXTENSIONS, iLoop);
					LoadExtByName(strExtensionName);
				}
			}
		}
		void CheckExtensions()
		{
			ClearExtensionVariables();
			std::for_each(&g_mappingTable[0], &g_mappingTable[0], ClearEntry());
			
			ProcExtsFromExtList();
		}
		
	}
}
