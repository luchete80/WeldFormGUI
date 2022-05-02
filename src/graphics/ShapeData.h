//FROM BULLET3 / EXMAPLES /OPENGLWINDOW/ SHAPEDATA.h

#ifndef _SHAPE_DATA_H
#define _SHAPE_DATA_H

///position xyz, unused w, normal, uv
static const float cube_vertices[] =
	{
		-1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		0,
		0,  //0
		1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		1,
		0,  //1
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		1,
		1,  //2
		-1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		0,
		1,  //3

		-1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		0,
		0,  //4
		1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		1,
		0,  //5
		1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		1,
		1,  //6
		-1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		0,
		1,  //7

		-1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		-1,
		0,
		0,
		0,
		0,
		-1.0f,
		1.0f,
		-1.0f,
		1.0f,
		-1,
		0,
		0,
		1,
		0,
		-1.0f,
		1.0f,
		1.0f,
		1.0f,
		-1,
		0,
		0,
		1,
		1,
		-1.0f,
		-1.0f,
		1.0f,
		1.0f,
		-1,
		0,
		0,
		0,
		1,

		1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		1,
		0,
		0,
		0,
		0,
		1.0f,
		1.0f,
		-1.0f,
		1.0f,
		1,
		0,
		0,
		1,
		0,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1,
		0,
		0,
		1,
		1,
		1.0f,
		-1.0f,
		1.0f,
		1.0f,
		1,
		0,
		0,
		0,
		1,

		-1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		-1,
		0,
		0,
		0,
		-1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		-1,
		0,
		1,
		0,
		1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		-1,
		0,
		1,
		1,
		1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		-1,
		0,
		0,
		1,

		-1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		1,
		0,
		0,
		0,
		-1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		1,
		0,
		1,
		0,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		1,
		0,
		1,
		1,
		1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		1,
		0,
		0,
		1,
};

///position xyz, unused w, normal, uv
static const float cube_vertices_textured[] =
	{
		-1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		0.75,
		0.25,  //0//back
		1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		1,
		0.25,  //1
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		1,
		0,  //2
		-1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		1,
		0.75,
		0,  //3

		-1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		0.5,
		0.25,  //4//front
		1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		0.25,
		0.25,  //5
		1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		0.25,
		0,  //6
		-1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		0,
		-1,
		0.5,
		0,  //7

		-1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		-1,
		0,
		0,
		0.5,
		0,  //Right
		-1.0f,
		1.0f,
		-1.0f,
		1.0f,
		-1,
		0,
		0,
		0.75,
		0,
		-1.0f,
		1.0f,
		1.0f,
		1.0f,
		-1,
		0,
		0,
		0.75,
		0.25,
		-1.0f,
		-1.0f,
		1.0f,
		1.0f,
		-1,
		0,
		0,
		0.5,
		0.25,

		1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		1,
		0,
		0,
		0.25,
		0.5,  //Left
		1.0f,
		1.0f,
		-1.0f,
		1.0f,
		1,
		0,
		0,
		0.25,
		0.25,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1,
		0,
		0,
		0.,
		.25,
		1.0f,
		-1.0f,
		1.0f,
		1.0f,
		1,
		0,
		0,
		0,
		.5,

		-1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		-1,
		0,
		0.25,
		0.5,  //bottom
		-1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		-1,
		0,
		0.25,
		0.25,
		1.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		-1,
		0,
		0.5,
		0.25,
		1.0f,
		-1.0f,
		-1.0f,
		1.0f,
		0,
		-1,
		0,
		0.5,
		0.5,

		-1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		1,
		0,
		0,
		0,  //top
		-1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		1,
		0,
		0,
		0.25,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		1,
		0,
		0.25,
		0.25,
		1.0f,
		1.0f,
		-1.0f,
		1.0f,
		0,
		1,
		0,
		0.25,
		0,
};

///position xyz, unused w, normal, uv
static const float cube_vertices2[] =
	{
		-1.5f,
		-0.5f,
		0.5f,
		0.0f,
		0,
		0,
		1,
		0,
		0,  //0
		1.5f,
		-0.5f,
		0.5f,
		0.0f,
		0,
		0,
		1,
		1,
		0,  //1
		1.5f,
		0.5f,
		0.5f,
		0.0f,
		0,
		0,
		1,
		1,
		1,  //2
		-1.5f,
		0.5f,
		0.5f,
		0.0f,
		0,
		0,
		1,
		0,
		1,  //3

		-1.5f,
		-0.5f,
		-0.5f,
		0.5f,
		0,
		0,
		-1,
		0,
		0,  //4
		1.5f,
		-0.5f,
		-0.5f,
		0.5f,
		0,
		0,
		-1,
		1,
		0,  //5
		1.5f,
		0.5f,
		-0.5f,
		0.5f,
		0,
		0,
		-1,
		1,
		1,  //6
		-1.5f,
		0.5f,
		-0.5f,
		0.5f,
		0,
		0,
		-1,
		0,
		1,  //7

		-1.5f,
		-0.5f,
		-0.5f,
		0.5f,
		-1,
		0,
		0,
		0,
		0,
		-1.5f,
		0.5f,
		-0.5f,
		0.5f,
		-1,
		0,
		0,
		1,
		0,
		-1.5f,
		0.5f,
		0.5f,
		0.5f,
		-1,
		0,
		0,
		1,
		1,
		-1.5f,
		-0.5f,
		0.5f,
		0.5f,
		-1,
		0,
		0,
		0,
		1,

		1.5f,
		-0.5f,
		-0.5f,
		0.5f,
		1,
		0,
		0,
		0,
		0,
		1.5f,
		0.5f,
		-0.5f,
		0.5f,
		1,
		0,
		0,
		1,
		0,
		1.5f,
		0.5f,
		0.5f,
		0.5f,
		1,
		0,
		0,
		1,
		1,
		1.5f,
		-0.5f,
		0.5f,
		0.5f,
		1,
		0,
		0,
		0,
		1,

		-1.5f,
		-0.5f,
		-0.5f,
		0.5f,
		0,
		-1,
		0,
		0,
		0,
		-1.5f,
		-0.5f,
		0.5f,
		0.5f,
		0,
		-1,
		0,
		1,
		0,
		1.5f,
		-0.5f,
		0.5f,
		0.5f,
		0,
		-1,
		0,
		1,
		1,
		1.5f,
		-0.5f,
		-0.5f,
		0.5f,
		0,
		-1,
		0,
		0,
		1,

		-1.5f,
		0.5f,
		-0.5f,
		0.5f,
		0,
		1,
		0,
		0,
		0,
		-1.5f,
		0.5f,
		0.5f,
		0.5f,
		0,
		1,
		0,
		1,
		0,
		1.5f,
		0.5f,
		0.5f,
		0.5f,
		0,
		1,
		0,
		1,
		1,
		1.5f,
		0.5f,
		-0.5f,
		0.5f,
		0,
		1,
		0,
		0,
		1,
};

static const int cube_indices[] =
	{
		0, 1, 2, 0, 2, 3,  //ground face
		6, 5, 4, 7, 6, 4,  //top face
		10, 9, 8, 11, 10, 8,
		12, 13, 14, 12, 14, 15,
		18, 17, 16, 19, 18, 16,
		20, 21, 22, 20, 22, 23};
    

const float low_sphere_vertices[] =
	{
		0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.999999f, 0.020053f,
		0.425323f, -0.850654f, 0.309011f, 0.000000f, 0.425323f, -0.850654f, 0.309011f, 0.336927f, 0.341759f,
		-0.162456f, -0.850654f, 0.499995f, 0.000000f, -0.162456f, -0.850654f, 0.499995f, 0.391200f, 0.484445f,
		0.723607f, -0.447220f, 0.525725f, 0.000000f, 0.723607f, -0.447220f, 0.525725f, 0.757596f, 0.176140f,
		0.425323f, -0.850654f, 0.309011f, 0.000000f, 0.425323f, -0.850654f, 0.309011f, 0.336927f, 0.341759f,
		0.850648f, -0.525736f, 0.000000f, 0.000000f, 0.850648f, -0.525736f, 0.000000f, 0.235205f, 0.325266f,
		0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.999999f, 0.020053f,
		-0.162456f, -0.850654f, 0.499995f, 0.000000f, -0.162456f, -0.850654f, 0.499995f, 0.391200f, 0.484445f,
		-0.525730f, -0.850652f, 0.000000f, 0.000000f, -0.525730f, -0.850652f, 0.000000f, 0.943007f, 0.371159f,
		0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.999999f, 0.020053f,
		-0.525730f, -0.850652f, 0.000000f, 0.000000f, -0.525730f, -0.850652f, 0.000000f, 0.943007f, 0.371159f,
		-0.162456f, -0.850654f, -0.499995f, 0.000000f, -0.162456f, -0.850654f, -0.499995f, 0.991338f, 0.520023f,
		0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.999999f, 0.020053f,
		-0.162456f, -0.850654f, -0.499995f, 0.000000f, -0.162456f, -0.850654f, -0.499995f, 0.991338f, 0.520023f,
		0.425323f, -0.850654f, -0.309011f, 0.000000f, 0.425323f, -0.850654f, -0.309011f, 0.137692f, 0.365246f,
		0.723607f, -0.447220f, 0.525725f, 0.000000f, 0.723607f, -0.447220f, 0.525725f, 0.757596f, 0.176140f,
		0.850648f, -0.525736f, 0.000000f, 0.000000f, 0.850648f, -0.525736f, 0.000000f, 0.235205f, 0.325266f,
		0.951058f, 0.000000f, 0.309013f, 0.000000f, 0.951058f, 0.000000f, 0.309013f, 0.291278f, 0.494851f,
		-0.276388f, -0.447220f, 0.850649f, 0.000000f, -0.276388f, -0.447220f, 0.850649f, 0.546072f, 0.156902f,
		0.262869f, -0.525738f, 0.809012f, 0.000000f, 0.262869f, -0.525738f, 0.809012f, 0.438926f, 0.305137f,
		0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.891200f, 0.515555f,
		-0.894426f, -0.447216f, 0.000000f, 0.000000f, -0.894426f, -0.447216f, 0.000000f, 0.746383f, 0.351642f,
		-0.688189f, -0.525736f, 0.499997f, 0.000000f, -0.688189f, -0.525736f, 0.499997f, 0.191461f, 0.507219f,
		-0.951058f, 0.000000f, 0.309013f, 0.000000f, -0.951058f, 0.000000f, 0.309013f, 0.735205f, 0.674734f,
		-0.276388f, -0.447220f, -0.850649f, 0.000000f, -0.276388f, -0.447220f, -0.850649f, 0.846308f, 0.334943f,
		-0.688189f, -0.525736f, -0.499997f, 0.000000f, -0.688189f, -0.525736f, -0.499997f, 1.039894f, 0.343193f,
		-0.587786f, 0.000000f, -0.809017f, 0.000000f, -0.587786f, 0.000000f, -0.809017f, 0.443007f, 0.628841f,
		0.723607f, -0.447220f, -0.525725f, 0.000000f, 0.723607f, -0.447220f, -0.525725f, 1.326823f, 0.166086f,
		0.262869f, -0.525738f, -0.809012f, 0.000000f, 0.262869f, -0.525738f, -0.809012f, 0.691461f, 0.492781f,
		0.587786f, 0.000000f, -0.809017f, 0.000000f, 0.587786f, 0.000000f, -0.809017f, 0.446597f, 0.804967f,
		0.723607f, -0.447220f, 0.525725f, 0.000000f, 0.723607f, -0.447220f, 0.525725f, 0.757596f, 0.176140f,
		0.951058f, 0.000000f, 0.309013f, 0.000000f, 0.951058f, 0.000000f, 0.309013f, 0.291278f, 0.494851f,
		0.587786f, 0.000000f, 0.809017f, 0.000000f, 0.587786f, 0.000000f, 0.809017f, 0.091499f, 0.516836f,
		-0.276388f, -0.447220f, 0.850649f, 0.000000f, -0.276388f, -0.447220f, 0.850649f, 0.546072f, 0.156902f,
		0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.891200f, 0.515555f,
		-0.587786f, 0.000000f, 0.809017f, 0.000000f, -0.587786f, 0.000000f, 0.809017f, 0.637692f, 0.634754f,
		-0.894426f, -0.447216f, 0.000000f, 0.000000f, -0.894426f, -0.447216f, 0.000000f, 0.746383f, 0.351642f,
		-0.951058f, 0.000000f, 0.309013f, 0.000000f, -0.951058f, 0.000000f, 0.309013f, 0.735205f, 0.674734f,
		-0.951058f, 0.000000f, -0.309013f, 0.000000f, -0.951058f, 0.000000f, -0.309013f, 0.629692f, 0.810571f,
		-0.276388f, -0.447220f, -0.850649f, 0.000000f, -0.276388f, -0.447220f, -0.850649f, 0.846308f, 0.334943f,
		-0.587786f, 0.000000f, -0.809017f, 0.000000f, -0.587786f, 0.000000f, -0.809017f, 0.443007f, 0.628841f,
		0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.539894f, 0.656807f,
		0.723607f, -0.447220f, -0.525725f, 0.000000f, 0.723607f, -0.447220f, -0.525725f, 1.326823f, 0.166086f,
		0.587786f, 0.000000f, -0.809017f, 0.000000f, 0.587786f, 0.000000f, -0.809017f, 0.446597f, 0.804967f,
		0.951058f, 0.000000f, -0.309013f, 0.000000f, 0.951058f, 0.000000f, -0.309013f, 0.491338f, 0.479977f,
		0.276388f, 0.447220f, 0.850649f, 0.000000f, 0.276388f, 0.447220f, 0.850649f, 1.129692f, 0.189429f,
		0.688189f, 0.525736f, 0.499997f, 0.000000f, 0.688189f, 0.525736f, 0.499997f, 0.246383f, 0.648358f,
		0.162456f, 0.850654f, 0.499995f, 0.000000f, 0.162456f, 0.850654f, 0.499995f, 0.836927f, 0.658241f,
		-0.723607f, 0.447220f, 0.525725f, 0.000000f, -0.723607f, 0.447220f, 0.525725f, 0.946597f, 0.195033f,
		-0.262869f, 0.525738f, 0.809012f, 0.000000f, -0.262869f, 0.525738f, 0.809012f, 0.346308f, 0.665057f,
		-0.425323f, 0.850654f, 0.309011f, 0.000000f, -0.425323f, 0.850654f, 0.309011f, 0.938926f, 0.694863f,
		-0.723607f, 0.447220f, -0.525725f, 0.000000f, -0.723607f, 0.447220f, -0.525725f, 0.791278f, 0.505149f,
		-0.850648f, 0.525736f, 0.000000f, 0.000000f, -0.850648f, 0.525736f, 0.000000f, 0.257596f, 0.823860f,
		-0.425323f, 0.850654f, -0.309011f, 0.000000f, -0.425323f, 0.850654f, -0.309011f, 0.826823f, 0.833914f,
		0.276388f, 0.447220f, -0.850649f, 0.000000f, 0.276388f, 0.447220f, -0.850649f, 0.542767f, 0.333057f,
		-0.262869f, 0.525738f, -0.809012f, 0.000000f, -0.262869f, 0.525738f, -0.809012f, 0.042767f, 0.666943f,
		0.162456f, 0.850654f, -0.499995f, 0.000000f, 0.162456f, 0.850654f, -0.499995f, 0.326823f, 0.166086f,
		0.894426f, 0.447216f, 0.000000f, 0.000000f, 0.894426f, 0.447216f, 0.000000f, 0.646443f, 0.311386f,
		0.688189f, 0.525736f, -0.499997f, 0.000000f, 0.688189f, 0.525736f, -0.499997f, 0.146443f, 0.688614f,
		0.525730f, 0.850652f, 0.000000f, 0.000000f, 0.525730f, 0.850652f, 0.000000f, 0.046072f, 0.843098f,
		-0.162456f, -0.850654f, 0.499995f, 0.000000f, -0.162456f, -0.850654f, 0.499995f, 0.391200f, 0.484445f,
		0.262869f, -0.525738f, 0.809012f, 0.000000f, 0.262869f, -0.525738f, 0.809012f, 0.438926f, 0.305137f,
		-0.276388f, -0.447220f, 0.850649f, 0.000000f, -0.276388f, -0.447220f, 0.850649f, 0.546072f, 0.156902f,
		-0.162456f, -0.850654f, 0.499995f, 0.000000f, -0.162456f, -0.850654f, 0.499995f, 0.391200f, 0.484445f,
		0.425323f, -0.850654f, 0.309011f, 0.000000f, 0.425323f, -0.850654f, 0.309011f, 0.336927f, 0.341759f,
		0.262869f, -0.525738f, 0.809012f, 0.000000f, 0.262869f, -0.525738f, 0.809012f, 0.438926f, 0.305137f,
		0.425323f, -0.850654f, 0.309011f, 0.000000f, 0.425323f, -0.850654f, 0.309011f, 0.336927f, 0.341759f,
		0.723607f, -0.447220f, 0.525725f, 0.000000f, 0.723607f, -0.447220f, 0.525725f, 0.757596f, 0.176140f,
		0.262869f, -0.525738f, 0.809012f, 0.000000f, 0.262869f, -0.525738f, 0.809012f, 0.438926f, 0.305137f,
		0.850648f, -0.525736f, 0.000000f, 0.000000f, 0.850648f, -0.525736f, 0.000000f, 0.235205f, 0.325266f,
		0.425323f, -0.850654f, -0.309011f, 0.000000f, 0.425323f, -0.850654f, -0.309011f, 0.137692f, 0.365246f,
		0.723607f, -0.447220f, -0.525725f, 0.000000f, 0.723607f, -0.447220f, -0.525725f, 1.326823f, 0.166086f,
		0.850648f, -0.525736f, 0.000000f, 0.000000f, 0.850648f, -0.525736f, 0.000000f, 0.235205f, 0.325266f,
		0.425323f, -0.850654f, 0.309011f, 0.000000f, 0.425323f, -0.850654f, 0.309011f, 0.336927f, 0.341759f,
		0.425323f, -0.850654f, -0.309011f, 0.000000f, 0.425323f, -0.850654f, -0.309011f, 0.137692f, 0.365246f,
		0.425323f, -0.850654f, 0.309011f, 0.000000f, 0.425323f, -0.850654f, 0.309011f, 0.336927f, 0.341759f,
		0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.999999f, 0.020053f,
		0.425323f, -0.850654f, -0.309011f, 0.000000f, 0.425323f, -0.850654f, -0.309011f, 0.137692f, 0.365246f,
		-0.525730f, -0.850652f, 0.000000f, 0.000000f, -0.525730f, -0.850652f, 0.000000f, 0.943007f, 0.371159f,
		-0.688189f, -0.525736f, 0.499997f, 0.000000f, -0.688189f, -0.525736f, 0.499997f, 0.191461f, 0.507219f,
		-0.894426f, -0.447216f, 0.000000f, 0.000000f, -0.894426f, -0.447216f, 0.000000f, 0.746383f, 0.351642f,
		-0.525730f, -0.850652f, 0.000000f, 0.000000f, -0.525730f, -0.850652f, 0.000000f, 0.943007f, 0.371159f,
		-0.162456f, -0.850654f, 0.499995f, 0.000000f, -0.162456f, -0.850654f, 0.499995f, 0.391200f, 0.484445f,
		-0.688189f, -0.525736f, 0.499997f, 0.000000f, -0.688189f, -0.525736f, 0.499997f, 0.191461f, 0.507219f,
		-0.162456f, -0.850654f, 0.499995f, 0.000000f, -0.162456f, -0.850654f, 0.499995f, 0.391200f, 0.484445f,
		-0.276388f, -0.447220f, 0.850649f, 0.000000f, -0.276388f, -0.447220f, 0.850649f, 0.546072f, 0.156902f,
		-0.688189f, -0.525736f, 0.499997f, 0.000000f, -0.688189f, -0.525736f, 0.499997f, 0.191461f, 0.507219f,
		-0.162456f, -0.850654f, -0.499995f, 0.000000f, -0.162456f, -0.850654f, -0.499995f, 0.991338f, 0.520023f,
		-0.688189f, -0.525736f, -0.499997f, 0.000000f, -0.688189f, -0.525736f, -0.499997f, 1.039894f, 0.343193f,
		-0.276388f, -0.447220f, -0.850649f, 0.000000f, -0.276388f, -0.447220f, -0.850649f, 0.846308f, 0.334943f,
		-0.162456f, -0.850654f, -0.499995f, 0.000000f, -0.162456f, -0.850654f, -0.499995f, 0.991338f, 0.520023f,
		-0.525730f, -0.850652f, 0.000000f, 0.000000f, -0.525730f, -0.850652f, 0.000000f, 0.943007f, 0.371159f,
		-0.688189f, -0.525736f, -0.499997f, 0.000000f, -0.688189f, -0.525736f, -0.499997f, 1.039894f, 0.343193f,
		-0.525730f, -0.850652f, 0.000000f, 0.000000f, -0.525730f, -0.850652f, 0.000000f, 0.943007f, 0.371159f,
		-0.894426f, -0.447216f, 0.000000f, 0.000000f, -0.894426f, -0.447216f, 0.000000f, 0.746383f, 0.351642f,
		-0.688189f, -0.525736f, -0.499997f, 0.000000f, -0.688189f, -0.525736f, -0.499997f, 1.039894f, 0.343193f,
		0.425323f, -0.850654f, -0.309011f, 0.000000f, 0.425323f, -0.850654f, -0.309011f, 0.137692f, 0.365246f,
		0.262869f, -0.525738f, -0.809012f, 0.000000f, 0.262869f, -0.525738f, -0.809012f, 0.691461f, 0.492781f,
		0.723607f, -0.447220f, -0.525725f, 0.000000f, 0.723607f, -0.447220f, -0.525725f, 1.326823f, 0.166086f,
		0.425323f, -0.850654f, -0.309011f, 0.000000f, 0.425323f, -0.850654f, -0.309011f, 0.137692f, 0.365246f,
		-0.162456f, -0.850654f, -0.499995f, 0.000000f, -0.162456f, -0.850654f, -0.499995f, 0.991338f, 0.520023f,
		0.262869f, -0.525738f, -0.809012f, 0.000000f, 0.262869f, -0.525738f, -0.809012f, 0.691461f, 0.492781f,
		-0.162456f, -0.850654f, -0.499995f, 0.000000f, -0.162456f, -0.850654f, -0.499995f, 0.991338f, 0.520023f,
		-0.276388f, -0.447220f, -0.850649f, 0.000000f, -0.276388f, -0.447220f, -0.850649f, 0.846308f, 0.334943f,
		0.262869f, -0.525738f, -0.809012f, 0.000000f, 0.262869f, -0.525738f, -0.809012f, 0.691461f, 0.492781f,
		0.951058f, 0.000000f, 0.309013f, 0.000000f, 0.951058f, 0.000000f, 0.309013f, 0.291278f, 0.494851f,
		0.951058f, 0.000000f, -0.309013f, 0.000000f, 0.951058f, 0.000000f, -0.309013f, 0.491338f, 0.479977f,
		0.894426f, 0.447216f, 0.000000f, 0.000000f, 0.894426f, 0.447216f, 0.000000f, 0.646443f, 0.311386f,
		0.951058f, 0.000000f, 0.309013f, 0.000000f, 0.951058f, 0.000000f, 0.309013f, 0.291278f, 0.494851f,
		0.850648f, -0.525736f, 0.000000f, 0.000000f, 0.850648f, -0.525736f, 0.000000f, 0.235205f, 0.325266f,
		0.951058f, 0.000000f, -0.309013f, 0.000000f, 0.951058f, 0.000000f, -0.309013f, 0.491338f, 0.479977f,
		0.850648f, -0.525736f, 0.000000f, 0.000000f, 0.850648f, -0.525736f, 0.000000f, 0.235205f, 0.325266f,
		0.723607f, -0.447220f, -0.525725f, 0.000000f, 0.723607f, -0.447220f, -0.525725f, 1.326823f, 0.166086f,
		0.951058f, 0.000000f, -0.309013f, 0.000000f, 0.951058f, 0.000000f, -0.309013f, 0.491338f, 0.479977f,
		0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.891200f, 0.515555f,
		0.587786f, 0.000000f, 0.809017f, 0.000000f, 0.587786f, 0.000000f, 0.809017f, 0.091499f, 0.516836f,
		0.276388f, 0.447220f, 0.850649f, 0.000000f, 0.276388f, 0.447220f, 0.850649f, 1.129692f, 0.189429f,
		0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.891200f, 0.515555f,
		0.262869f, -0.525738f, 0.809012f, 0.000000f, 0.262869f, -0.525738f, 0.809012f, 0.438926f, 0.305137f,
		0.587786f, 0.000000f, 0.809017f, 0.000000f, 0.587786f, 0.000000f, 0.809017f, 0.091499f, 0.516836f,
		0.262869f, -0.525738f, 0.809012f, 0.000000f, 0.262869f, -0.525738f, 0.809012f, 0.438926f, 0.305137f,
		0.723607f, -0.447220f, 0.525725f, 0.000000f, 0.723607f, -0.447220f, 0.525725f, 0.757596f, 0.176140f,
		0.587786f, 0.000000f, 0.809017f, 0.000000f, 0.587786f, 0.000000f, 0.809017f, 0.091499f, 0.516836f,
		-0.951058f, 0.000000f, 0.309013f, 0.000000f, -0.951058f, 0.000000f, 0.309013f, 0.735205f, 0.674734f,
		-0.587786f, 0.000000f, 0.809017f, 0.000000f, -0.587786f, 0.000000f, 0.809017f, 0.637692f, 0.634754f,
		-0.723607f, 0.447220f, 0.525725f, 0.000000f, -0.723607f, 0.447220f, 0.525725f, 0.946597f, 0.195033f,
		-0.951058f, 0.000000f, 0.309013f, 0.000000f, -0.951058f, 0.000000f, 0.309013f, 0.735205f, 0.674734f,
		-0.688189f, -0.525736f, 0.499997f, 0.000000f, -0.688189f, -0.525736f, 0.499997f, 0.191461f, 0.507219f,
		-0.587786f, 0.000000f, 0.809017f, 0.000000f, -0.587786f, 0.000000f, 0.809017f, 0.637692f, 0.634754f,
		-0.688189f, -0.525736f, 0.499997f, 0.000000f, -0.688189f, -0.525736f, 0.499997f, 0.191461f, 0.507219f,
		-0.276388f, -0.447220f, 0.850649f, 0.000000f, -0.276388f, -0.447220f, 0.850649f, 0.546072f, 0.156902f,
		-0.587786f, 0.000000f, 0.809017f, 0.000000f, -0.587786f, 0.000000f, 0.809017f, 0.637692f, 0.634754f,
		-0.587786f, 0.000000f, -0.809017f, 0.000000f, -0.587786f, 0.000000f, -0.809017f, 0.443007f, 0.628841f,
		-0.951058f, 0.000000f, -0.309013f, 0.000000f, -0.951058f, 0.000000f, -0.309013f, 0.629692f, 0.810571f,
		-0.723607f, 0.447220f, -0.525725f, 0.000000f, -0.723607f, 0.447220f, -0.525725f, 0.791278f, 0.505149f,
		-0.587786f, 0.000000f, -0.809017f, 0.000000f, -0.587786f, 0.000000f, -0.809017f, 0.443007f, 0.628841f,
		-0.688189f, -0.525736f, -0.499997f, 0.000000f, -0.688189f, -0.525736f, -0.499997f, 1.039894f, 0.343193f,
		-0.951058f, 0.000000f, -0.309013f, 0.000000f, -0.951058f, 0.000000f, -0.309013f, 0.629692f, 0.810571f,
		-0.688189f, -0.525736f, -0.499997f, 0.000000f, -0.688189f, -0.525736f, -0.499997f, 1.039894f, 0.343193f,
		-0.894426f, -0.447216f, 0.000000f, 0.000000f, -0.894426f, -0.447216f, 0.000000f, 0.746383f, 0.351642f,
		-0.951058f, 0.000000f, -0.309013f, 0.000000f, -0.951058f, 0.000000f, -0.309013f, 0.629692f, 0.810571f,
		0.587786f, 0.000000f, -0.809017f, 0.000000f, 0.587786f, 0.000000f, -0.809017f, 0.446597f, 0.804967f,
		0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.539894f, 0.656807f,
		0.276388f, 0.447220f, -0.850649f, 0.000000f, 0.276388f, 0.447220f, -0.850649f, 0.542767f, 0.333057f,
		0.587786f, 0.000000f, -0.809017f, 0.000000f, 0.587786f, 0.000000f, -0.809017f, 0.446597f, 0.804967f,
		0.262869f, -0.525738f, -0.809012f, 0.000000f, 0.262869f, -0.525738f, -0.809012f, 0.691461f, 0.492781f,
		0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.539894f, 0.656807f,
		0.262869f, -0.525738f, -0.809012f, 0.000000f, 0.262869f, -0.525738f, -0.809012f, 0.691461f, 0.492781f,
		-0.276388f, -0.447220f, -0.850649f, 0.000000f, -0.276388f, -0.447220f, -0.850649f, 0.846308f, 0.334943f,
		0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.539894f, 0.656807f,
		0.587786f, 0.000000f, 0.809017f, 0.000000f, 0.587786f, 0.000000f, 0.809017f, 0.091499f, 0.516836f,
		0.688189f, 0.525736f, 0.499997f, 0.000000f, 0.688189f, 0.525736f, 0.499997f, 0.246383f, 0.648358f,
		0.276388f, 0.447220f, 0.850649f, 0.000000f, 0.276388f, 0.447220f, 0.850649f, 1.129692f, 0.189429f,
		0.587786f, 0.000000f, 0.809017f, 0.000000f, 0.587786f, 0.000000f, 0.809017f, 0.091499f, 0.516836f,
		0.951058f, 0.000000f, 0.309013f, 0.000000f, 0.951058f, 0.000000f, 0.309013f, 0.291278f, 0.494851f,
		0.688189f, 0.525736f, 0.499997f, 0.000000f, 0.688189f, 0.525736f, 0.499997f, 0.246383f, 0.648358f,
		0.951058f, 0.000000f, 0.309013f, 0.000000f, 0.951058f, 0.000000f, 0.309013f, 0.291278f, 0.494851f,
		0.894426f, 0.447216f, 0.000000f, 0.000000f, 0.894426f, 0.447216f, 0.000000f, 0.646443f, 0.311386f,
		0.688189f, 0.525736f, 0.499997f, 0.000000f, 0.688189f, 0.525736f, 0.499997f, 0.246383f, 0.648358f,
		-0.587786f, 0.000000f, 0.809017f, 0.000000f, -0.587786f, 0.000000f, 0.809017f, 0.637692f, 0.634754f,
		-0.262869f, 0.525738f, 0.809012f, 0.000000f, -0.262869f, 0.525738f, 0.809012f, 0.346308f, 0.665057f,
		-0.723607f, 0.447220f, 0.525725f, 0.000000f, -0.723607f, 0.447220f, 0.525725f, 0.946597f, 0.195033f,
		-0.587786f, 0.000000f, 0.809017f, 0.000000f, -0.587786f, 0.000000f, 0.809017f, 0.637692f, 0.634754f,
		0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.891200f, 0.515555f,
		-0.262869f, 0.525738f, 0.809012f, 0.000000f, -0.262869f, 0.525738f, 0.809012f, 0.346308f, 0.665057f,
		0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.891200f, 0.515555f,
		0.276388f, 0.447220f, 0.850649f, 0.000000f, 0.276388f, 0.447220f, 0.850649f, 1.129692f, 0.189429f,
		-0.262869f, 0.525738f, 0.809012f, 0.000000f, -0.262869f, 0.525738f, 0.809012f, 0.346308f, 0.665057f,
		-0.951058f, 0.000000f, -0.309013f, 0.000000f, -0.951058f, 0.000000f, -0.309013f, 0.629692f, 0.810571f,
		-0.850648f, 0.525736f, 0.000000f, 0.000000f, -0.850648f, 0.525736f, 0.000000f, 0.257596f, 0.823860f,
		-0.723607f, 0.447220f, -0.525725f, 0.000000f, -0.723607f, 0.447220f, -0.525725f, 0.791278f, 0.505149f,
		-0.951058f, 0.000000f, -0.309013f, 0.000000f, -0.951058f, 0.000000f, -0.309013f, 0.629692f, 0.810571f,
		-0.951058f, 0.000000f, 0.309013f, 0.000000f, -0.951058f, 0.000000f, 0.309013f, 0.735205f, 0.674734f,
		-0.850648f, 0.525736f, 0.000000f, 0.000000f, -0.850648f, 0.525736f, 0.000000f, 0.257596f, 0.823860f,
		-0.951058f, 0.000000f, 0.309013f, 0.000000f, -0.951058f, 0.000000f, 0.309013f, 0.735205f, 0.674734f,
		-0.723607f, 0.447220f, 0.525725f, 0.000000f, -0.723607f, 0.447220f, 0.525725f, 0.946597f, 0.195033f,
		-0.850648f, 0.525736f, 0.000000f, 0.000000f, -0.850648f, 0.525736f, 0.000000f, 0.257596f, 0.823860f,
		0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.539894f, 0.656807f,
		-0.262869f, 0.525738f, -0.809012f, 0.000000f, -0.262869f, 0.525738f, -0.809012f, 0.042767f, 0.666943f,
		0.276388f, 0.447220f, -0.850649f, 0.000000f, 0.276388f, 0.447220f, -0.850649f, 0.542767f, 0.333057f,
		0.000000f, 0.000000f, -1.000000f, 0.000000f, 0.000000f, 0.000000f, -1.000000f, 0.539894f, 0.656807f,
		-0.587786f, 0.000000f, -0.809017f, 0.000000f, -0.587786f, 0.000000f, -0.809017f, 0.443007f, 0.628841f,
		-0.262869f, 0.525738f, -0.809012f, 0.000000f, -0.262869f, 0.525738f, -0.809012f, 0.042767f, 0.666943f,
		-0.587786f, 0.000000f, -0.809017f, 0.000000f, -0.587786f, 0.000000f, -0.809017f, 0.443007f, 0.628841f,
		-0.723607f, 0.447220f, -0.525725f, 0.000000f, -0.723607f, 0.447220f, -0.525725f, 0.791278f, 0.505149f,
		-0.262869f, 0.525738f, -0.809012f, 0.000000f, -0.262869f, 0.525738f, -0.809012f, 0.042767f, 0.666943f,
		0.951058f, 0.000000f, -0.309013f, 0.000000f, 0.951058f, 0.000000f, -0.309013f, 0.491338f, 0.479977f,
		0.688189f, 0.525736f, -0.499997f, 0.000000f, 0.688189f, 0.525736f, -0.499997f, 0.146443f, 0.688614f,
		0.894426f, 0.447216f, 0.000000f, 0.000000f, 0.894426f, 0.447216f, 0.000000f, 0.646443f, 0.311386f,
		0.951058f, 0.000000f, -0.309013f, 0.000000f, 0.951058f, 0.000000f, -0.309013f, 0.491338f, 0.479977f,
		0.587786f, 0.000000f, -0.809017f, 0.000000f, 0.587786f, 0.000000f, -0.809017f, 0.446597f, 0.804967f,
		0.688189f, 0.525736f, -0.499997f, 0.000000f, 0.688189f, 0.525736f, -0.499997f, 0.146443f, 0.688614f,
		0.587786f, 0.000000f, -0.809017f, 0.000000f, 0.587786f, 0.000000f, -0.809017f, 0.446597f, 0.804967f,
		0.276388f, 0.447220f, -0.850649f, 0.000000f, 0.276388f, 0.447220f, -0.850649f, 0.542767f, 0.333057f,
		0.688189f, 0.525736f, -0.499997f, 0.000000f, 0.688189f, 0.525736f, -0.499997f, 0.146443f, 0.688614f,
		0.162456f, 0.850654f, 0.499995f, 0.000000f, 0.162456f, 0.850654f, 0.499995f, 0.836927f, 0.658241f,
		0.525730f, 0.850652f, 0.000000f, 0.000000f, 0.525730f, 0.850652f, 0.000000f, 0.046072f, 0.843098f,
		0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.591499f, 0.483164f,
		0.162456f, 0.850654f, 0.499995f, 0.000000f, 0.162456f, 0.850654f, 0.499995f, 0.836927f, 0.658241f,
		0.688189f, 0.525736f, 0.499997f, 0.000000f, 0.688189f, 0.525736f, 0.499997f, 0.246383f, 0.648358f,
		0.525730f, 0.850652f, 0.000000f, 0.000000f, 0.525730f, 0.850652f, 0.000000f, 0.046072f, 0.843098f,
		0.688189f, 0.525736f, 0.499997f, 0.000000f, 0.688189f, 0.525736f, 0.499997f, 0.246383f, 0.648358f,
		0.894426f, 0.447216f, 0.000000f, 0.000000f, 0.894426f, 0.447216f, 0.000000f, 0.646443f, 0.311386f,
		0.525730f, 0.850652f, 0.000000f, 0.000000f, 0.525730f, 0.850652f, 0.000000f, 0.046072f, 0.843098f,
		-0.425323f, 0.850654f, 0.309011f, 0.000000f, -0.425323f, 0.850654f, 0.309011f, 0.938926f, 0.694863f,
		0.162456f, 0.850654f, 0.499995f, 0.000000f, 0.162456f, 0.850654f, 0.499995f, 0.836927f, 0.658241f,
		0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.591499f, 0.483164f,
		-0.425323f, 0.850654f, 0.309011f, 0.000000f, -0.425323f, 0.850654f, 0.309011f, 0.938926f, 0.694863f,
		-0.262869f, 0.525738f, 0.809012f, 0.000000f, -0.262869f, 0.525738f, 0.809012f, 0.346308f, 0.665057f,
		0.162456f, 0.850654f, 0.499995f, 0.000000f, 0.162456f, 0.850654f, 0.499995f, 0.836927f, 0.658241f,
		-0.262869f, 0.525738f, 0.809012f, 0.000000f, -0.262869f, 0.525738f, 0.809012f, 0.346308f, 0.665057f,
		0.276388f, 0.447220f, 0.850649f, 0.000000f, 0.276388f, 0.447220f, 0.850649f, 1.129692f, 0.189429f,
		0.162456f, 0.850654f, 0.499995f, 0.000000f, 0.162456f, 0.850654f, 0.499995f, 0.836927f, 0.658241f,
		-0.425323f, 0.850654f, -0.309011f, 0.000000f, -0.425323f, 0.850654f, -0.309011f, 0.826823f, 0.833914f,
		-0.425323f, 0.850654f, 0.309011f, 0.000000f, -0.425323f, 0.850654f, 0.309011f, 0.938926f, 0.694863f,
		0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.591499f, 0.483164f,
		-0.425323f, 0.850654f, -0.309011f, 0.000000f, -0.425323f, 0.850654f, -0.309011f, 0.826823f, 0.833914f,
		-0.850648f, 0.525736f, 0.000000f, 0.000000f, -0.850648f, 0.525736f, 0.000000f, 0.257596f, 0.823860f,
		-0.425323f, 0.850654f, 0.309011f, 0.000000f, -0.425323f, 0.850654f, 0.309011f, 0.938926f, 0.694863f,
		-0.850648f, 0.525736f, 0.000000f, 0.000000f, -0.850648f, 0.525736f, 0.000000f, 0.257596f, 0.823860f,
		-0.723607f, 0.447220f, 0.525725f, 0.000000f, -0.723607f, 0.447220f, 0.525725f, 0.946597f, 0.195033f,
		-0.425323f, 0.850654f, 0.309011f, 0.000000f, -0.425323f, 0.850654f, 0.309011f, 0.938926f, 0.694863f,
		0.162456f, 0.850654f, -0.499995f, 0.000000f, 0.162456f, 0.850654f, -0.499995f, 0.326823f, 0.166086f,
		-0.425323f, 0.850654f, -0.309011f, 0.000000f, -0.425323f, 0.850654f, -0.309011f, 0.826823f, 0.833914f,
		0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.591499f, 0.483164f,
		0.162456f, 0.850654f, -0.499995f, 0.000000f, 0.162456f, 0.850654f, -0.499995f, 0.326823f, 0.166086f,
		-0.262869f, 0.525738f, -0.809012f, 0.000000f, -0.262869f, 0.525738f, -0.809012f, 0.042767f, 0.666943f,
		-0.425323f, 0.850654f, -0.309011f, 0.000000f, -0.425323f, 0.850654f, -0.309011f, 0.826823f, 0.833914f,
		-0.262869f, 0.525738f, -0.809012f, 0.000000f, -0.262869f, 0.525738f, -0.809012f, 0.042767f, 0.666943f,
		-0.723607f, 0.447220f, -0.525725f, 0.000000f, -0.723607f, 0.447220f, -0.525725f, 0.791278f, 0.505149f,
		-0.425323f, 0.850654f, -0.309011f, 0.000000f, -0.425323f, 0.850654f, -0.309011f, 0.826823f, 0.833914f,
		0.525730f, 0.850652f, 0.000000f, 0.000000f, 0.525730f, 0.850652f, 0.000000f, 0.046072f, 0.843098f,
		0.162456f, 0.850654f, -0.499995f, 0.000000f, 0.162456f, 0.850654f, -0.499995f, 0.326823f, 0.166086f,
		0.000000f, 1.000000f, 0.000000f, 0.000000f, 0.000000f, 1.000000f, 0.000000f, 0.591499f, 0.483164f,
		0.525730f, 0.850652f, 0.000000f, 0.000000f, 0.525730f, 0.850652f, 0.000000f, 0.046072f, 0.843098f,
		0.688189f, 0.525736f, -0.499997f, 0.000000f, 0.688189f, 0.525736f, -0.499997f, 0.146443f, 0.688614f,
		0.162456f, 0.850654f, -0.499995f, 0.000000f, 0.162456f, 0.850654f, -0.499995f, 0.326823f, 0.166086f,
		0.688189f, 0.525736f, -0.499997f, 0.000000f, 0.688189f, 0.525736f, -0.499997f, 0.146443f, 0.688614f,
		0.276388f, 0.447220f, -0.850649f, 0.000000f, 0.276388f, 0.447220f, -0.850649f, 0.542767f, 0.333057f,
		0.162456f, 0.850654f, -0.499995f, 0.000000f, 0.162456f, 0.850654f, -0.499995f, 0.326823f, 0.166086f};

const int low_sphere_indices[] =
	{
		0, 1, 2,
		3, 4, 5,
		6, 7, 8,
		9, 10, 11,
		12, 13, 14,
		15, 16, 17,
		18, 19, 20,
		21, 22, 23,
		24, 25, 26,
		27, 28, 29,
		30, 31, 32,
		33, 34, 35,
		36, 37, 38,
		39, 40, 41,
		42, 43, 44,
		45, 46, 47,
		48, 49, 50,
		51, 52, 53,
		54, 55, 56,
		57, 58, 59,
		60, 61, 62,
		63, 64, 65,
		66, 67, 68,
		69, 70, 71,
		72, 73, 74,
		75, 76, 77,
		78, 79, 80,
		81, 82, 83,
		84, 85, 86,
		87, 88, 89,
		90, 91, 92,
		93, 94, 95,
		96, 97, 98,
		99, 100, 101,
		102, 103, 104,
		105, 106, 107,
		108, 109, 110,
		111, 112, 113,
		114, 115, 116,
		117, 118, 119,
		120, 121, 122,
		123, 124, 125,
		126, 127, 128,
		129, 130, 131,
		132, 133, 134,
		135, 136, 137,
		138, 139, 140,
		141, 142, 143,
		144, 145, 146,
		147, 148, 149,
		150, 151, 152,
		153, 154, 155,
		156, 157, 158,
		159, 160, 161,
		162, 163, 164,
		165, 166, 167,
		168, 169, 170,
		171, 172, 173,
		174, 175, 176,
		177, 178, 179,
		180, 181, 182,
		183, 184, 185,
		186, 187, 188,
		189, 190, 191,
		192, 193, 194,
		195, 196, 197,
		198, 199, 200,
		201, 202, 203,
		204, 205, 206,
		207, 208, 209,
		210, 211, 212,
		213, 214, 215,
		216, 217, 218,
		219, 220, 221,
		222, 223, 224,
		225, 226, 227,
		228, 229, 230,
		231, 232, 233,
		234, 235, 236,
		237, 238, 239};



//////////////////////////

// Our vertices. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
// ONLY VERTEX DATA
// FROM http://www.opengl-tutorial.org/beginners-tutorials/tutorial-4-a-colored-cube/
// CAN BE DRAWN 
static const GLfloat g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f, // triangle 1 : begin
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, // triangle 1 : end
    1.0f, 1.0f,-1.0f, // triangle 2 : begin
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f, // triangle 2 : end
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f
};

#endif