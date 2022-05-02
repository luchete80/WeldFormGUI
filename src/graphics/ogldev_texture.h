#ifndef TEXTURE_H
#define	TEXTURE_H

#include <string>

//#include <GL/glew.h>
#include <glad/glad.h>
//#include <Magick++.h>

class Texture {
public:
    Texture(GLenum TextureTarget, const std::string& FileName);

    bool Load();

    void Bind(GLenum TextureUnit);

private:
    std::string m_fileName;
    GLenum m_textureTarget;
    GLuint m_textureObj;
    // Magick::Image m_image;
    // Magick::Blob m_blob;
};

//according to LearnOpenGL
// struct Texture {
    // unsigned int id;
    // string type;
    // string path;
// };


#endif	/* TEXTURE_H */

