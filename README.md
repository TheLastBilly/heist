# Heist
A command line utility to generate scriptable scenes for 3D objects

## Usage
```bash
heist [scene file]
```

`heist` uses scripts written on heist (.hst files) to render scenes on a 3D space using primitives like rectangles and spheres, or more complex 3D objects via wavefront files. The following features are currently supported by heist (the language).

### `name`: Sets the name of an scene, and must be called before rendering

*Example*
```
name my_scene
```

### `create`: Used to create variables, primitives and/or lights on a scene. Can be used alongside properties of the objects to be created. 

*Examples:*
```
# Creates a sphere of radius 2
create sphere 2

# Creates a rectangle with 2x2 dimensions
create rectangle (2 2)

# Creates a point light with 100% brightness on all colors
create light (1 1 1)

# Creates a variable called var
create variable var
```

### `load`: Loads a 3D model from a wavefront file.

*Example:*
```
# Loads ./teapot.obj onto mesh teapot
load ./teapot.obj teapot
```

### `translate`: Applies a translation to an object on the scene

*Example:*
```
# Translates camera to coordinates (x: 1, y: 1, z: 10)
translate camera (1 1 10)
```

### `rotate`: Applies a rotation (around the origin) to an object on the scene

*Example:*
```
# Translates rotates camera by 90 degrees around y axis
rotate camera (0 1 0 90)
```

### `assign`: Assigns a numeric value to a variable

*Example:*
```
# Assign variable var a value of 10
assign var 10
```

### `add`: Adds a value to a variable

*Example:*
```
# Adds 10 to variable var
add var 10
```

### `substract`: Substract a value to a variable

*Example:*
```
# Substracts 10 from variable var
substract var 10
```

### `equals`: Compares a variable with a number, and should the number match the variable, it will skip the next instruction and land on the one after

*Example:*
```
# Will skip the creation of sphere a, and only create b
assign var 1
equals var 1
create sphere a 2
create sphere b 2
```

### `tag`: Tags a specific point on the program for later use
### `goto`: Will move the program counter to the specified tab

*Example*
```
# Program will loop indefinetly
tag test
...
goto test
```

### `color`: Applies a color to a specific object

*Example*
```
# Make sphere_a red
color sphere_a (1 0 0)
```

### `render`: Renders the current scene and saves the frame on a file called [scene name]_[frame number].ppm