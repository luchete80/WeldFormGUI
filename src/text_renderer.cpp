//
//  ProtoBall - a fun Soccer Game
//  Copyright (C) 2020-2021 Luciano Buglioni <physerdev@gmail.com>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "text_renderer.h"
#include <iostream>

#include <glad/glad.h>

#include "texture.h"
#include "shader.h"

// #include <GLFW/glfw3.h>
//#include <GL/GL.h>

using namespace std;


TextRenderer::TextRenderer(unsigned int width, unsigned int height)
{
	/// THIS IS THE ORIGINAL
    // // load and configure shader
    // this->TextShader = ResourceManager::LoadShader("text_2d.vs", "text_2d.fs", nullptr, "text");
    // this->TextShader.SetMatrix4("projection", glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f), true);
    // this->TextShader.SetInteger("text", 0);
    // // configure VAO/VBO for texture quads
    // glGenVertexArrays(1, &this->VAO);
    // glGenBuffers(1, &this->VBO);
    // glBindVertexArray(this->VAO);
    // glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);
	
	
	
	
		    // // compile and setup the shader
    // ----------------------------
    //Shader shader("text.vs", "text.fs");
	cout << "Initializing shaders"<<endl;
	shader=new Shader("text.vs", "text.fs");
	cout << "shader created"<<endl;
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
    shader->use();
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	
	cout << "Initializing freetype"<<endl;
    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        //return -1;
    }

	// find path to font
    //std::string font_name = FileSystem::getPath("resources/fonts/Antonio-Bold.ttf");
	std::string font_name = "Antonio-Bold.ttf";
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        //return -1;
    }
	
	// load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font xx" << std::endl;
        //return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    
    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void TextRenderer::Load(std::string font, unsigned int fontSize)
{
    // // first clear the previously loaded Characters
    // this->Characters.clear();
    // // then initialize and load the FreeType library
    // FT_Library ft;    
    // if (FT_Init_FreeType(&ft)) // all functions return a value different than 0 whenever an error occurred
        // std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    // // load font as face
    // FT_Face face;
    // if (FT_New_Face(ft, font.c_str(), 0, &face))
        // std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    // // set size to load glyphs as
    // FT_Set_Pixel_Sizes(face, 0, fontSize);
    // // disable byte-alignment restriction
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
    // // then for the first 128 ASCII characters, pre-load/compile their characters and store them
    // for (GLubyte c = 0; c < 128; c++) // lol see what I did there 
    // {
        // // load character glyph 
        // if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        // {
            // std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            // continue;
        // }
        // // generate texture
        // unsigned int texture;
        // glGenTextures(1, &texture);
        // glBindTexture(GL_TEXTURE_2D, texture);
        // glTexImage2D(
            // GL_TEXTURE_2D,
            // 0,
            // GL_RED,
            // face->glyph->bitmap.width,
            // face->glyph->bitmap.rows,
            // 0,
            // GL_RED,
            // GL_UNSIGNED_BYTE,
            // face->glyph->bitmap.buffer
            // );
        // // set texture options
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       
        // // now store character for later use
        // Character character = {
            // texture,
            // glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            // glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            // face->glyph->advance.x
        // };
        // Characters.insert(std::pair<char, Character>(c, character));
    // }
    // glBindTexture(GL_TEXTURE_2D, 0);
    // // destroy FreeType once we're finished
    // FT_Done_Face(face);
    // FT_Done_FreeType(ft);
}

void TextRenderer::RenderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
	    // // activate corresponding render state	
    shader->use();
    glUniform3f(glGetUniformLocation(shader->ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character, NOW ARE ORIENTED CLOCKWISE
				float vertices[6][4] = {
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f },      

            { xpos + w, ypos + h,   1.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f }
          
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}