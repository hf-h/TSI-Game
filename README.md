# TSI

A small terminal based shmup game. Very much a work in progress.

## Platforms

- Windows

## Build

To build the project you will need the msvc compiler "cl" and the c environment.
The easiest way of doing this is to install the compiler via Visual Studio.

The environment can be loaded up using the x64 "Native Tools" command prompt provided with Visual Studio or manually load up the vcvarsall.bat in a cmd instance

Once the environment is loaded run the build.bat file and it will produce the tsi.exe binary

## Running

Run the program from the command line.
Tested on cmd and the PowerShell version you can get in the ms store.

### Controls

W, A, S, D for movement

SPACE for shooting

### Gameplay

- You have one hp so any damage taken is game over
- There are pickups marked with a blue P that will spawn, these give you additional weapons
