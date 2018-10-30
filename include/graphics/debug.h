#ifndef DEBUG_H
#define DEBUG_H

#include <GL/glew.h>
#include <map>

#include "util/logging.h"

extern std::map<GLenum,std::string> GL_ERROR_STRINGS;
#define checkErrors() {GLenum err;while((err = glGetError()) != GL_NO_ERROR){warn("OpenGL Error {}", GL_ERROR_STRINGS.find(err) != GL_ERROR_STRINGS.end() ? GL_ERROR_STRINGS[err] : std::to_string(err));}}


#endif // DEBUG_H
