# WeldFormGUI users guide

## Introduction

This Graphical User Interface (GUI) is intended to build, run and view the results of a FEM model.

## Model 
A model consists of parts, either deformable(probes/pieces) or rigid (dies), bounday conditions (in case of no dies), meshes, material(s), etc.

### Setting up a Model
You can right click on the model tree and there you can right over "3D" and click edit, and change to 2D (2D is Work In Progress).

### Adding a Part
Up until now, you can draw only simple shapes and load external STEP geometry files. 
So, you can right click on a part and select "New Geometry from file" or, in case you want to generate a simple geometry, you can go to 

### New Geometry
`New Geometry` changes depending on the current analysis type.

For 3D analysis:
- `Box`
- `Cylinder`
- `Plane`

For 2D analysis:
- `Line`
- `Rectangle` or `Profile`
- `Closed Profile`

Use `Closed Profile` when you want to create a 2D surface from a closed contour and mesh it later.

Current `Closed Profile` workflow:
- switch the model to a 2D analysis type first
- open `New Geometry`
- choose `Closed Profile` in the `Geometry` combo
- use `Seed rectangle` to create an initial contour or `Add point` to build one manually
- edit the `x` and `y` coordinates of each point
- click `Create GEO`

Current limitations:
- the contour must contain at least 3 points
- the contour area must be non-zero
- consecutive duplicate points are rejected
- self-intersecting contours are rejected
- this is intended for simple polygonal closed contours

### Meshing a Part
By right clicking a part, you select mesh and

## Model Input
Model input is a file in which you write the model input file which is taken by the solver in order to run the model itself. 

## Jobs
A job is a task to run a model. The idea is, for every model you create, you run a job, selecting the run options like number of cores. 

### Create a Job
By right cliking "Jobs" you select "Create new job".

There are two supported flows:
- `From Model`
  Exports a new `.wfinput` from the active model and creates a job for it.
- `Choose File`
  Uses an existing `.wfinput` or `.json` file and creates a job for that input after pressing `Create`.

### 3D implicit restart and checkpoint

The Job dialog exposes restart/checkpoint options only for implicit 3D jobs using `weldform_imp_3d`.

Supported fields:
- `checkpointEnabled`
- `checkpointInterval`
- `checkpointDir`
- `checkpointPrefix`
- `restartFile`

`checkpointDir` is still supported by the GUI and exporter.

If `checkpointDir` is set to `"."`, checkpoint/restart files are written in the run directory itself. This matches the validation setup used by the engine-side restart tests.

The restart file chooser in the GUI uses `.wfrst` files.

### Runing and monitoring a job


## Results
