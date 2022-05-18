/*
        Copyright 2011 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SHADOWMAPFBO_H
#define	SHADOWMAPFBO_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class PickingTexture
{
public:
    PickingTexture();

    ~PickingTexture();

    bool Init(unsigned int WindowWidth, unsigned int WindowHeight);

    void EnableWriting();
    
    void DisableWriting();
    
    struct PixelInfo {
        //LUCIANO
        // float ObjectID;
        // float DrawID;
        // float PrimID;

        unsigned char ObjectID;
        unsigned char DrawID;
        unsigned char PrimID;
        
        PixelInfo() //NOT USED
        {
            ObjectID = 0.0f;
            DrawID = 0.0f;
            PrimID = 0.0f;
        }
    };
    
    int m_pickedID;
    PixelInfo ReadPixel(unsigned int x, unsigned int y);
    int ReadPixelToInt(unsigned int x, unsigned int y);
    
private:
    GLuint m_fbo;
    GLuint m_pickingTexture;
    GLuint m_depthTexture;
    

};

#endif	/* SHADOWMAPFBO_H */

