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
#include <config.h>

#include "TextureSDL.hpp"

TextureSDL::~TextureSDL()
{
    destroyTexturesMap(textures);
    SDL_FreeSurface(surface);

    if(zoomSurface)
    {
        destroyTexturesMap(zoomTextures);
        SDL_FreeSurface(zoomSurface);
    }
}

SDL_Texture* TextureSDL::getTexture(SDL_Renderer* renderer)
{
    SDL_Texture* tex;
    auto texIter = textures.find(renderer);

    if(texIter == textures.end())
    {
        tex = SDL_CreateTextureFromSurface(renderer, surface);
        textures[renderer] = tex;
    }
    else
    {
        tex = (*texIter).second;
    }

    return tex;
}

SDL_Texture* TextureSDL::getZoomTexture(SDL_Renderer* renderer, double zx, double zy)
{
    SDL_Texture* zoomTex;
    auto texIter = zoomTextures.find(renderer);

    if(texIter == zoomTextures.end())
    {
        zoomTex = SDL_CreateTextureFromSurface(renderer, zoomSurface);
        zoomTextures[renderer] = zoomTex;
    }
    else
    {
        zoomTex = (*texIter).second;
    }

    return zoomTex;
}

/** @file gui/PainterSDL/TextureSDL.cpp */

