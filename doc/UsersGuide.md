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

### Meshing a Part
By right clicking a part, you select mesh and

## Model Input
Model input is a file in which you write the model input file which is taken by the solver in order to run the model itself. 

## Jobs
A job is a task to run a model. The idea is, for every model you create, you run a job, selecting the run options like number of cores. 

### Create a Job
By right cliking "Jobs" you select "Create new job" and then you select input file.

### Runing and monitoring a job


## Results