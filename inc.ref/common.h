#pragma once

// this is the socket to which our nex will communicate with. 
// those %s, %i, etc formatters will be formated later in the src. 
// for those of you who know preprocessor, it is just a text replacment.
#define SOCKET_PATH_TPL  "/tmp/nex%s_%i_%i-socket"

// this is our SOCKET env variable, NEX_SOCKET will be set, so that our nexl can write to it. 
#define SOCKET_ENV_VAR   "NEX_SOCKET"

// this hex \x07 is BELL character.
// our failure message is a bell. 
#define FAILURE_MESSAGE  "\x07"

/*
#pragma once : this is the modern equivalent of three lines.

#ifndef SOMETHING
#define SOMETHING 

../ preprocessor code, goes here.

#endif
 */
