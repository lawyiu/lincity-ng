/*
Copyright (C) 2005 Matthias Braun <matze@braunis.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef __TEXTURESDL_HPP__
#define __TEXTURESDL_HPP__

#include "gui/Texture.hpp"
#include <SDL.h>
#include <map>

/**
 * Wrapper around a pixmap. Texture have to be created by the TextureManager
 * class
 */
class TextureSDL : public Texture
{
public:
    virtual ~TextureSDL();

    float getWidth() const
    {
        return surface->w;
    }
    float getHeight() const
    {
        return surface->h;
    }

    void setZoomSurface(SDL_Surface* zs, double zx, double zy)
    {
        if(zoomSurface != NULL)
        {
            destroyTexturesMap(zoomTextures);
            SDL_FreeSurface(zoomSurface);
            zoomSurface = NULL;
        }

        zoomSurface = zs;
        zoomx = zx;
        zoomy = zy;
    }

    SDL_Texture* getTexture(SDL_Renderer* renderer);
    SDL_Texture* getZoomTexture(SDL_Renderer* renderer, double zx, double zy);

private:
    friend class PainterSDL;
    friend class TextureManagerSDL;
    TextureSDL(SDL_Surface* _surface)
        : surface(_surface)
    {
        zoomSurface = NULL;
    }

    void destroyTexturesMap(std::map<SDL_Renderer*, SDL_Texture*>& texMap)
    {
        for (auto elm : texMap)
        {
            SDL_DestroyTexture(elm.second);
        }

        texMap.clear();
    }

    SDL_Surface* surface;
    std::map<SDL_Renderer*, SDL_Texture*> textures;
    SDL_Surface* zoomSurface;
    std::map<SDL_Renderer*, SDL_Texture*> zoomTextures;
    double zoomx,zoomy;
};

#endif

/** @file gui/PainterSDL/TextureSDL.hpp */

