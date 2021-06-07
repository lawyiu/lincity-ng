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

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <new>
#include <physfs.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <unistd.h>

#include "gui/FontManager.hpp"
#include "gui/TextureManager.hpp"
#include "gui/PainterSDL/TextureManagerSDL.hpp"
#include "gui/PainterSDL/PainterSDL.hpp"
#include "gui/PainterGL/TextureManagerGL.hpp"
#include "gui/PainterGL/PainterGL.hpp"

#include "main.hpp"
#include "MainLincity.hpp"
#include "MainMenu.hpp"
#include "Game.hpp"
#include "Sound.hpp"
#include "Config.hpp"
#include "PBar.hpp"
#include "lincity/loadsave.h"
#include "lincity/engglobs.h"
#include "lincity/lin-city.h"
#include "lincity/init_game.h"


#ifdef ENABLE_BINRELOC
#include "binreloc.h"
#endif

SDL_Window* window = NULL;
SDL_GLContext window_context = NULL;
SDL_Renderer* window_renderer = NULL;
Painter* painter = 0;
tinygettext::DictionaryManager* dictionaryManager = 0;
bool restart = false;

#ifdef __APPLE__
     extern char *getBundleSharePath(char *packageName);
#endif

void initPhysfs(const char* argv0)
{
    if(!PHYSFS_init(argv0)) {
        std::stringstream msg;
        msg << "Couldn't initialize physfs: " << PHYSFS_getLastError();
        throw std::runtime_error(msg.str());
    }

    // Initialize physfs (this is a slightly modified version of
    // PHYSFS_setSaneConfig
    const char* application = LC_SAVE_DIR;
    const char* userdir = PHYSFS_getUserDir();
    const char* dirsep = PHYSFS_getDirSeparator();
    char* writedir = new char[strlen(userdir) + strlen(application) + 2];

    // Set configuration directory
    //sprintf(writedir, "%s.%s", userdir, application);
    sprintf(writedir, "%s%s", userdir, application);
    if(!PHYSFS_setWriteDir(writedir)) {
        // try to create the directory
        char* mkdir = new char[strlen(application) + 2];
        sprintf(mkdir, "%s", application);
        if(!PHYSFS_setWriteDir(userdir) || !PHYSFS_mkdir(mkdir)) {
            std::ostringstream msg;
            msg << "Failed creating configuration directory '" <<
                writedir << "': " << PHYSFS_getLastError();
            delete[] writedir;
            delete[] mkdir;
            throw std::runtime_error(msg.str());
        }
        delete[] mkdir;

        if(!PHYSFS_setWriteDir(writedir)) {
            std::ostringstream msg;
            msg << "Failed to use configuration directory '" <<
                writedir << "': " << PHYSFS_getLastError();
            delete[] writedir;
            throw std::runtime_error(msg.str());
        }
    }
    PHYSFS_addToSearchPath(writedir, 0);

    // add ~/.lincity for old savegames
    sprintf(writedir, "%s.lincity", userdir);
    PHYSFS_addToSearchPath(writedir, 1);
    delete[] writedir;

  //TODO: add zips later
    // Search for archives and add them to the search path
    const char* archiveExt = "zip";
    char** rc = PHYSFS_enumerateFiles("/");
    size_t extlen = strlen(archiveExt);
//TODO sort .zip files! so we are sure which patch is first.
//and change all file access to physfs. what does PHYSFS_getRealDir
//do when file in in archive?
    for(char** i = rc; *i != 0; ++i) {
        size_t l = strlen(*i);
        if((l > extlen) && ((*i)[l - extlen - 1] == '.')) {
            const char* ext = (*i) + (l - extlen);
            if(strcasecmp(ext, archiveExt) == 0) {
                const char* d = PHYSFS_getRealDir(*i);
                char* str = new char[strlen(d) + strlen(dirsep) + l + 1];
                sprintf(str, "%s%s%s", d, dirsep, *i);
                PHYSFS_addToSearchPath(str, 1);
                delete[] str;
            }
        }
    }

    PHYSFS_freeList(rc);

    // when started from source dir...
    std::string dir = PHYSFS_getBaseDir();
    dir += "data";
    std::ostringstream testfname;
    //TODO: Windows/Mingw does not like this test on other machine?
    testfname << dir << dirsep << "images" << dirsep << "tiles" << dirsep << "images.xml";
    FILE* f = fopen(testfname.str().c_str(), "r");
    if(f) {
        fclose(f);
        if(!PHYSFS_addToSearchPath(dir.c_str(), 1)) {
#ifdef DEBUG
            std::cout << "Warning: Couldn't add '" << dir <<
                "' to physfs searchpath: " << PHYSFS_getLastError() << "\n";
#endif
        }
    }

#ifndef __APPLE__	
	#if defined(APPDATADIR) || defined(ENABLE_BINRELOC)
	    std::string datadir;
	#ifdef ENABLE_BINRELOC
	    BrInitError error;
	    if (br_init (&error) == 0 && error != BR_INIT_ERROR_DISABLED) {
	        printf ("Warning: BinReloc failed to initialize (error code %d)\n",
	                error);
	        printf ("Will fallback to hardcoded default path.\n");
	    }
	
	    char* brdatadir = br_find_data_dir("/usr/local/share");
	    datadir = brdatadir;
	    datadir += "/" PACKAGE_NAME;
	    free(brdatadir);
	#else
	    datadir = APPDATADIR;
	#endif
	
	    if(!PHYSFS_addToSearchPath(datadir.c_str(), 1)) {
	        std::cout << "Couldn't add '" << datadir
	            << "' to physfs searchpath: " << PHYSFS_getLastError() << "\n";
	    }
	#endif
#else
	#if defined(APPDATADIR) || defined(ENABLE_BINRELOC) && __APPLE__ 
	    std::string datadir;
	#ifdef ENABLE_BINRELOC
	    BrInitError error;
	    if (br_init (&error) == 0 && error != BR_INIT_ERROR_DISABLED) {
	        printf ("Warning: BinReloc failed to initialize (error code %d)\n",
	                error);
	        printf ("Will fallback to hardcoded default path.\n");
	    }
	
	    char* brdatadir = br_find_data_dir("/usr/local/share");
	    datadir = brdatadir;
	    datadir += "/" PACKAGE_NAME;
	    free(brdatadir);
	  #else
	      datadir = APPDATADIR;
	  #endif
	     datadir = getBundleSharePath(PACKAGE_NAME);
	      if(!PHYSFS_addToSearchPath(datadir.c_str(), 1)) {
	          std::cout << "Couldn't add '" << datadir
	              << "' to physfs searchpath: " << PHYSFS_getLastError() << "\n";
	    }
	#endif
#endif

    // allow symbolic links
    PHYSFS_permitSymbolicLinks(1);

    //show search Path
    for(char** i = PHYSFS_getSearchPath(); *i != NULL; i++)
    {   printf("[%s] is in the search path.\n", *i);}
    //show write directory
    printf("[%s] is the write directory.\n", PHYSFS_getWriteDir());
}

void musicHalted() {
    getSound()->changeTrack(NEXT_OR_FIRST_TRACK);
    //FIXME: options menu song entry doesn't update while song changes.
}

void videoSizeChanged(int width, int height) {
    if (getConfig()->useOpenGL) {
        /* Reset OpenGL state */
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glClearColor(0, 0, 0, 0);
        glViewport(0, 0, width, height);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);

        glClear(GL_COLOR_BUFFER_BIT);
    }
}
void resizeVideo(int width, int height, bool fullscreen)
{
    SDL_SetWindowSize(window, width, height);
    // Set fullscreen (video mode change)
    if (fullscreen) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window, 0);
    }
}

void initVideo(int width, int height)
{
    Uint32 flags = 0;

    flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    if( getConfig()->useOpenGL ){
        flags |= SDL_WINDOW_OPENGL;
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
        //SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
        //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    }
    if(getConfig()->useFullScreen)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    window = SDL_CreateWindow(PACKAGE_NAME " " PACKAGE_VERSION,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, width, height,
                              flags);
    if( getConfig()->useOpenGL ){
        window_context = SDL_GL_CreateContext(window);
        SDL_GL_SetSwapInterval(1);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glClearColor(0, 0, 0, 0);
        glViewport(0, 0, width, height);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);

        glClear(GL_COLOR_BUFFER_BIT);

        painter = new PainterGL(window);
        std::cout << "\nOpenGL Mode " << width;
        std::cout << "x" << height << "\n";

        texture_manager = new TextureManagerGL();
    } else {
        window_renderer = SDL_CreateRenderer(window, -1, 0);

        painter = new PainterSDL(window_renderer);
        std::cout << "\nSDL Mode " << width;
        std::cout << "x"<< height <<"\n";

        texture_manager = new TextureManagerSDL();
    }

    fontManager = new FontManager();
}

void mainLoop()
{
    std::unique_ptr<MainMenu> menu;
    std::unique_ptr<Game> game;
    MainState state = MAINMENU;
    MainState nextstate;

    while(state != QUIT)
    {
        switch(state)
        {
            case MAINMENU:
                {
                    if(menu.get() == 0)
                    {   menu.reset(new MainMenu(window));}
                    nextstate = menu->run();
                }
                break;
            case INGAME:
                {
                    if(game.get() == 0)
                    {
                        game.reset(new Game(window));

                        while(!LCPBarPage1 || !LCPBarPage2)
                        {//wait until PBars exist so they can be initalized
                            printf(".");
                            SDL_Delay(100);
                        }
                    }
                    nextstate = game->run();
                    if(menu.get() == 0)
                    {    menu.reset(new MainMenu(window));}
                    menu->gotoMainMenu();

                }
                break;
            case RESTART:
                restart = true;
                nextstate = QUIT;
                break;
            default:
                assert(false);
        }
        state = nextstate;
    }
}

void parseCommandLine(int argc, char** argv)
{
    for(int currentArgument = 1; currentArgument < argc; ++currentArgument) {
        std::string argStr = argv[currentArgument];

        if(argStr == "-v" || argStr == "--version") {
            std::cout << PACKAGE_NAME << " version " << PACKAGE_VERSION << "\n";
            exit(0);
        } else  if(argStr == "-h" || argStr == "--help") {
            std::cout << PACKAGE_NAME << " version " << PACKAGE_VERSION << "\n";
            std::cout << "Command line overrides configfiles.\n";
            std::cout << "Known arguments are:\n";
            std::cout << "-v           --version         show version and exit\n";
            std::cout << "-h           --help            show this text and exit\n";
            std::cout << "-g           --gl              use OpenGL\n";
            std::cout << "-s           --sdl             use SDL\n";
            std::cout << "-S [size]    --size [size]     specify screensize (eg. -S 1024x768)\n";
            std::cout << "-w           --window          run in window\n";
            std::cout << "-f           --fullscreen      run fullscreen\n";
            std::cout << "-m           --mute            mute audio\n";
            std::cout << "-q [delay]   --quick [delay]   Setting for fast speed (current " \
                                                                << getConfig()->quickness \
                                                                << ")\n";
            std::cout << "                               -q 9 skips animation steps for speed.\n";
            std::cout << "                               -q 8 is the slowest speed with full animation.\n";
            std::cout << "                               -q 1 is fastest. It may heat your hardware!\n";
            exit(0);
        } else if(argStr == "-g" || argStr == "--gl") {
            getConfig()->useOpenGL = true;
        } else if(argStr == "-s" || argStr == "--sdl") {
            getConfig()->useOpenGL = false;
        } else if(argStr == "-S" || argStr == "--size") {
            currentArgument++;
            if(currentArgument >= argc) {
                std::cerr << "Error: --size needs a parameter.\n";
                exit(1);
            }
            argStr = argv[currentArgument];
            int newX, newY, count;
            count = sscanf( argStr.c_str(), "%ix%i", &newX, &newY );
            if( count != 2  ) {
                std::cerr << "Error: Can not parse --size parameter.\n";
                exit( 1 );
            }
            if(newX <= 0 || newY <= 0) {
                std::cerr << "Error: Size parameter out of range.\n";
                exit(1);
            }
            getConfig()->videoX = newX;
            getConfig()->videoY = newY;
        } else if(argStr == "-f" || argStr == "--fullscreen") {
            getConfig()->useFullScreen = true;
        } else if(argStr == "-w" || argStr == "--window") {
            getConfig()->useFullScreen = false;
        } else if(argStr == "-m" || argStr == "--mute") {
            getConfig()->soundEnabled = false;
            getConfig()->musicEnabled = false;
        } else if (argStr == "-q" || argStr == "--quick") {
            currentArgument++;
            if(currentArgument >= argc) {
                std::cerr << "Error: --quick needs a parameter.\n";
                exit(1);
            }
            //fast_time_for_year
            argStr = argv[currentArgument];
            int newSpeed;
            sscanf( argStr.c_str(), "%i", &newSpeed );
            if ( newSpeed < 1 || newSpeed > 9 ) {
                fprintf(stderr, " --quick = %i out of range (1..9). Will use default value %i\n", \
                                newSpeed, FAST_TIME_FOR_YEAR);
                newSpeed = FAST_TIME_FOR_YEAR;
            }
            getConfig()->quickness = newSpeed;

        } else {
            std::cerr << "Unknown command line argument: " << argStr << "\n";
            exit(1);
        }
    }
}

int main(int argc, char** argv)
{
    int result = 0;

#ifndef DEBUG //in debug mode we wanna have a backtrace
    try {
        std::cout << "Starting " << PACKAGE_NAME << " (version " << PACKAGE_VERSION << ")...\n";
#else
        std::cout << "Starting " << PACKAGE_NAME << " (version " << PACKAGE_VERSION << ") in Debug Mode...\n";
#endif
        initPhysfs(argv[0]);

        if( getConfig()->language != "autodetect" ){
            setenv("LINCITY_LANG", getConfig()->language.c_str(), false);
        }
        dictionaryManager = new tinygettext::DictionaryManager();
        dictionaryManager->set_charset("UTF-8");
        dictionaryManager->add_directory("locale");
        std::cout << "Language is \"" << dictionaryManager->get_language() << "\".\n";

#ifndef DEBUG
    } catch(std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        return 1;
    } catch(...) {
        std::cerr << "Unexpected exception.\n";
        return 1;
    }
#endif
    parseCommandLine(argc, argv); // Do not use getConfig() before parseCommandLine for anything command line might change.

    fast_time_for_year = getConfig()->quickness;
    fprintf(stderr," fast = %i\n", fast_time_for_year);

// in debug mode we want a backtrace of the exceptions so we don't catch them
#ifndef DEBUG
    try {
#endif
        xmlInitParser ();
#ifdef EMSCRIPTEN
        if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
#else
        if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
#endif
            std::stringstream msg;
            msg << "Couldn't initialize SDL: " << SDL_GetError();
            throw std::runtime_error(msg.str());
        }
        if(TTF_Init() < 0) {
            std::stringstream msg;
            msg << "Couldn't initialize SDL_ttf: " << SDL_GetError();
            throw std::runtime_error(msg.str());
        }
        initVideo(getConfig()->videoX, getConfig()->videoY);
        initLincity();
        std::unique_ptr<Sound> sound;
        sound.reset(new Sound());
        //set a function to call when music stops
        Mix_HookMusicFinished(musicHalted);
        mainLoop();
        getConfig()->save();
        destroy_game();
#ifndef DEBUG
    } catch(std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        result = 1;
    } catch(...) {
        std::cerr << "Unexpected exception.\n";
        result = 1;
    }
#endif
    delete painter;
    delete fontManager;
    delete texture_manager;
    if(TTF_WasInit())
        TTF_Quit();
    if(SDL_WasInit(0))
        SDL_Quit();
    xmlCleanupParser();
    delete dictionaryManager;
    dictionaryManager = 0;
    PHYSFS_deinit();
    if( restart ){
#ifdef WIN32
        //Windows has a Problem with Whitespaces.
        std::string fixWhiteSpaceInPathnameProblem;
        fixWhiteSpaceInPathnameProblem="\"";
        fixWhiteSpaceInPathnameProblem+=argv[0];
        fixWhiteSpaceInPathnameProblem+="\"";
        execlp( argv[0], fixWhiteSpaceInPathnameProblem.c_str(), (char *) NULL );
#else
        execlp( argv[0], argv[0], (char *) NULL );
#endif
    }
    return result;
}


/** @file lincity-ng/main.cpp */

