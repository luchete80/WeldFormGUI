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

#include "picking_technique.h"
#include "ogldev_util.h"




PickingTechnique::PickingTechnique()
{   
}

bool PickingTechnique::Init()
{
    if (!Technique::Init()) {
        return false;
    }

    if (!AddShader(GL_VERTEX_SHADER, "picking.vs")) {
        return false;
    }

    if (!AddShader(GL_FRAGMENT_SHADER, "picking.fs")) {
        return false;
    }
    
    if (!Finalize()) {
        return false;
    }

    m_WVPLocation = GetUniformLocation("gWVP");
    m_objectIndexLocation = GetUniformLocation("gPickingColor");
    //m_drawIndexLocation = GetUniformLocation("gDrawIndex");

    if (m_WVPLocation == INVALID_UNIFORM_LOCATION ||
        m_objectIndexLocation == INVALID_UNIFORM_LOCATION ||
        m_drawIndexLocation == INVALID_UNIFORM_LOCATION) {
        return false;
    }

    return true;
}


void PickingTechnique::SetWVP(const Matrix4f& WVP)
{
    glUniformMatrix4fv(m_WVPLocation, 1, GL_TRUE, (const GLfloat*)WVP.m);    
}

void PickingTechnique::SetWVP_glm(glm::mat4 WVP) /// DO NOT TRANSPOSE 
{
    glUniformMatrix4fv(m_WVPLocation, 1, GL_FALSE, &WVP[0][0]);    
}

void PickingTechnique::DrawStartCB(uint DrawIndex)
{
    glUniform1ui(m_drawIndexLocation, DrawIndex);
}


void PickingTechnique::SetObjectIndex(uint ObjectIndex)
{
    //GLExitIfError;
    //glUniform1ui(m_objectIndexLocation, ObjectIndex);
    
      // Convert "i", the integer mesh ID, into an RGB color
  int r = (ObjectIndex & 0x000000FF) >>  0;
  int g = (ObjectIndex & 0x0000FF00) >>  8;
  int b = (ObjectIndex & 0x00FF0000) >> 16;

  // OpenGL expects colors to be in [0,1], so divide by 255.
  glUniform4f(m_objectIndexLocation, r/255.0f, g/255.0f, b/255.0f, 1.0f);
//    GLExitIfError;
}