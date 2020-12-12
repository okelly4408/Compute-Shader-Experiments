 #version 450 core

layout(location = 0) in vec4 position;
uniform mat4 screen_to_camera;
uniform mat4 world_to_screen;
uniform mat4 camera_to_world;
uniform vec3 ocean_camera_pos;
uniform sampler2DArray wave_sampler;
uniform vec2 gridSize;
uniform vec4 GRID_SIZES;
out vec2 uv;
out vec3 color;
out vec3 P;
out vec2 u;
out vec3 camera_dir;
const vec4 choppy_factor = vec4(2.3, 2.1, 1.3, 0.9);
vec2 ocean_pos(vec4 vertex, out vec3 world_dir)
{
	camera_dir = normalize((screen_to_camera * vertex).xyz);
	world_dir = (camera_to_world * vec4(camera_dir, 0.0)).xyz;
	//was t = -world_camera.z / world_dir.z;
	float t = -(ocean_camera_pos.y) / (min(world_dir.y, -0.000001));
//	float t = -ocean_camera_pos.y / world_dir.y;
	return ocean_camera_pos.xz + (t) * world_dir.xz;
}
void main()
{
	//uv = position.zw;
	vec3 wd;
	u = ocean_pos(position, wd);
	vec2 ux = ocean_pos(position + vec4(gridSize.x, 0.0, 0.0, 0.0), wd);
    vec2 uy = ocean_pos(position + vec4(0.0, gridSize.y, 0.0, 0.0), wd);
    vec2 dux = abs(ux - u) * 2.0;
    vec2 duy = abs(uy - u) * 2.0;

    vec3 dP = vec3(0.0);
    dP.y += textureGrad(wave_sampler, vec3(u / GRID_SIZES.x, 0.0), dux / GRID_SIZES.x, duy / GRID_SIZES.x).x;
    dP.y += textureGrad(wave_sampler, vec3(u / GRID_SIZES.y, 0.0), dux / GRID_SIZES.y, duy / GRID_SIZES.y).y;
    dP.y += textureGrad(wave_sampler, vec3(u / GRID_SIZES.z, 0.0), dux / GRID_SIZES.z, duy / GRID_SIZES.z).z;
    dP.y += textureGrad(wave_sampler, vec3(u / GRID_SIZES.w, 0.0), dux / GRID_SIZES.w, duy / GRID_SIZES.w).w;
    dP.xz += choppy_factor.x * textureGrad(wave_sampler, vec3(u / GRID_SIZES.x, 3.0), dux / GRID_SIZES.x, duy / GRID_SIZES.x).xy;
    dP.xz += choppy_factor.y * textureGrad(wave_sampler, vec3(u / GRID_SIZES.y, 3.0), dux / GRID_SIZES.y, duy / GRID_SIZES.y).zw;
    dP.xz += choppy_factor.z * textureGrad(wave_sampler, vec3(u / GRID_SIZES.z, 4.0), dux / GRID_SIZES.z, duy / GRID_SIZES.z).xy;
    dP.xz += choppy_factor.w * textureGrad(wave_sampler, vec3(u / GRID_SIZES.w, 4.0), dux / GRID_SIZES.w, duy / GRID_SIZES.w).zw;
	color = dP;
	P = vec3(u.x + dP.x, dP.y, u.y + dP.z);
	gl_Position = world_to_screen * vec4(P, 1.0);
	//gl_Position = vec4(position.xy, 0.0, 1.0);
}