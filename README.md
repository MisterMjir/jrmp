# (M)jir Map Platformer

Map format for a platformer I am making

# Features
Somewhat extensible, but no out-of-box support for it.
Parallax layers

# Design
I want this to be somewhat efficient, there are two types of files, the .jrmp file that
holds compact data, and the other files (like header, tiles) that hold data that's easier
to edit and use in the game

This is tile base, but layer 0 is a dedicated collision layer so worlds don't have to be 100% blocky

Only loading the tiles that will be on screen + a few more will save a bunch of memory

There are also zones, these can be for music changes, or change in properties like gravity or something

There is also scripting (TBD), which will be an easier way to edit some runtime properties of the map

NPCS and items haven't been planned at all yet, most likely they will both be treated the same way,
most likely will just have a string id and you can write things to happen on events like interactions
with the script

Enemies also have not been planned...
