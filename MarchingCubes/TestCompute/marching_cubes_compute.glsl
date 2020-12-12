#version 450 core
//size of local work group. the compute shader is constructed with parameters NUM WORKGROUP xyz so 4, 4, 4, would lead to (4 * 4 * 4) 'cubes of compute shaders' each of those cube sizes specified in 
//local_size. When local_size = 8,8,8 each of those (4*4*4) cubes would have a compute shader load of 8*8*8 == 32768
//gl_GlobalInvocationID = gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID ie which work group in in NUM WORKGROUP which is a (x,y,z) and which chunk this one shader is in the workgroup
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout(location = 3) uniform sampler3D noise_map;
layout(r16f, binding = 1) readonly uniform image2D tri_table;
layout(binding = 2) writeonly buffer Vertices
{
	vec4 elements[];
} vertices;
layout(binding = 3) buffer Triangles_Drawn
{
	uint elements[];
} triangles_drawn;
layout(binding = 4) buffer Normals
{
	vec4 elements[];
} normals;
//#define BLOCKY
const uint edge_table[256]={
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };
const vec3 offsets[8] =
{
	vec3(0.0, 0.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(1.0, 0.0, 1.0),
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 1.0, 1.0),
	vec3(1.0, 1.0, 1.0),
	vec3(1.0, 1.0, 0.0)
};
vec3 vert_interp(float iso_level, vec3 v1, vec3 v2, float iso_v1, float iso_v2)
{
	return mix(v1, v2, (iso_level - iso_v1) / (iso_v2 - iso_v1));
}
float mapTerrain(in vec3 uvw)
{
	return texture(noise_map, uvw).r;
}
vec3 calcNormal( in vec3 pos)
{
    vec3 eps = vec3( 0.01,0.0,0.0);
	return normalize( vec3(
           mapTerrain(pos+eps.xyy) - mapTerrain(pos-eps.xyy),
           mapTerrain(pos+eps.yxy) - mapTerrain(pos-eps.yxy),
           mapTerrain(pos+eps.yyx) - mapTerrain(pos-eps.yyx) ) );
}
vec3 vert_list[12];
vec3 norm_list[12];
void main(void)
{
	float scale = 1.0/16.0;
	vec3 base_offset = vec3(gl_GlobalInvocationID.xyz) * scale;
	float size = float(gl_NumWorkGroups.x * gl_WorkGroupSize.x) * scale;
	vec3 c1 = (offsets[0] * scale) + base_offset;
	vec3 c2 = (offsets[1] * scale) + base_offset;
	vec3 c3 = (offsets[2] * scale) + base_offset;
	vec3 c4 = (offsets[3] * scale) + base_offset;
	vec3 c5 = (offsets[4] * scale) + base_offset;
	vec3 c6 = (offsets[5] * scale) + base_offset;
	vec3 c7 = (offsets[6] * scale) + base_offset;
	vec3 c8 = (offsets[7] * scale) + base_offset;
	float iso_c1 = texture(noise_map, (c1 / size)).r;
	float iso_c2 = texture(noise_map, (c2 / size)).r;
	float iso_c3 = texture(noise_map, (c3 / size)).r;
	float iso_c4 = texture(noise_map, (c4 / size)).r;
	float iso_c5 = texture(noise_map, (c5 / size)).r;
	float iso_c6 = texture(noise_map, (c6 / size)).r;
	float iso_c7 = texture(noise_map, (c7 / size)).r;
	float iso_c8 = texture(noise_map, (c8 / size)).r;
	uint cube_index = 0;
	float surf_thresh = 1.25;
	cube_index = uint(iso_c1 < surf_thresh);
	cube_index += uint(iso_c2 < surf_thresh)*2;
	cube_index += uint(iso_c3 < surf_thresh)*4;
	cube_index += uint(iso_c4 < surf_thresh)*8;
	cube_index += uint(iso_c5 < surf_thresh)*16;
	cube_index += uint(iso_c6 < surf_thresh)*32;
	cube_index += uint(iso_c7 < surf_thresh)*64;
	cube_index += uint(iso_c8 < surf_thresh)*128;
	uint num_triangles = 0;
	uint i;
	if(cube_index != 0 && cube_index != 255)
	{
		#ifdef BLOCKY
		vec3 down = normalize(vec3(0.2, -1.0, 0.0));
		vec3 up = normalize(vec3(0.0, 1.0, 0.2));
		vec3 right = normalize(vec3(1.0, 0.2, 0.0));
		vec3 left = normalize(vec3(-1.0, 0.0, 0.2));
		vec3 forward =normalize(vec3(0.2, 0.0, 1.0));
		vec3 back = normalize(vec3(0.0, 0.2, -1.0));
		uint index_offset = atomicAdd(triangles_drawn.elements[0], 1);
		

		//bottom
		vertices.elements[index_offset * 3] = vec4(c1, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c2, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c3, 1.0);

		normals.elements[index_offset * 3] = vec4(down, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(down, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(down, 1.0);

		index_offset = atomicAdd(triangles_drawn.elements[0], 1);
		vertices.elements[index_offset * 3] = vec4(c4, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c3, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c1, 1.0);

		normals.elements[index_offset * 3] = vec4(down, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(down, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(down, 1.0);

		//top
		index_offset = atomicAdd(triangles_drawn.elements[0], 1);

		vertices.elements[index_offset * 3] = vec4(c5, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c6, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c7, 1.0);

		normals.elements[index_offset * 3] = vec4(up, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(up, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(up, 1.0);

		index_offset = atomicAdd(triangles_drawn.elements[0], 1);
		vertices.elements[index_offset * 3] = vec4(c8, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c7, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c5, 1.0);

		normals.elements[index_offset * 3] = vec4(up, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(up, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(up, 1.0);

		//right
		index_offset = atomicAdd(triangles_drawn.elements[0], 1);

		vertices.elements[index_offset * 3] = vec4(c8, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c4, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c3, 1.0);

		normals.elements[index_offset * 3] = vec4(right, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(right, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(right, 1.0);

		index_offset = atomicAdd(triangles_drawn.elements[0], 1);
		vertices.elements[index_offset * 3] = vec4(c3, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c7, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c8, 1.0);

		normals.elements[index_offset * 3] = vec4(right, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(right, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(right, 1.0);

		//left
		index_offset = atomicAdd(triangles_drawn.elements[0], 1);

		vertices.elements[index_offset * 3] = vec4(c5, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c1, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c2, 1.0);

		normals.elements[index_offset * 3] = vec4(left, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(left, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(left, 1.0);

		index_offset = atomicAdd(triangles_drawn.elements[0], 1);
		vertices.elements[index_offset * 3] = vec4(c2, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c6, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c5, 1.0);

		normals.elements[index_offset * 3] = vec4(left, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(left, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(left, 1.0);

		//forward
		index_offset = atomicAdd(triangles_drawn.elements[0], 1);

		vertices.elements[index_offset * 3] = vec4(c6, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c2, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c3, 1.0);

		normals.elements[index_offset * 3] = vec4(forward, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(forward, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(forward, 1.0);

		index_offset = atomicAdd(triangles_drawn.elements[0], 1);
		vertices.elements[index_offset * 3] = vec4(c7, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c3, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c6, 1.0);

		normals.elements[index_offset * 3] = vec4(forward, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(forward, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(forward, 1.0);

		//back
		index_offset = atomicAdd(triangles_drawn.elements[0], 1);

		vertices.elements[index_offset * 3] = vec4(c5, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c1, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c4, 1.0);

		normals.elements[index_offset * 3] = vec4(back, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(back, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(back, 1.0);

		index_offset = atomicAdd(triangles_drawn.elements[0], 1);
		vertices.elements[index_offset * 3] = vec4(c8, 1.0);
		vertices.elements[(index_offset * 3) + 1] = vec4(c4, 1.0);
		vertices.elements[(index_offset * 3) + 2] = vec4(c5, 1.0);

		normals.elements[index_offset * 3] = vec4(back, 1.0);
		normals.elements[(index_offset * 3) + 1] = vec4(back, 1.0);
		normals.elements[(index_offset * 3) + 2] = vec4(back, 1.0);

		#else

			vec3 n1 = calcNormal(c1 / size);
			vec3 n2 = calcNormal(c2 / size);
			vec3 n3 = calcNormal(c3 / size);
			vec3 n4 = calcNormal(c4 / size);
			vec3 n5 = calcNormal(c5 / size);
			vec3 n6 = calcNormal(c6 / size);
			vec3 n7 = calcNormal(c7 / size);
			vec3 n8 = calcNormal(c8 / size);
			vert_list[0] = vert_interp(surf_thresh, c1, c2, iso_c1, iso_c2);
			norm_list[0] = vert_interp(surf_thresh, n1, n2, iso_c1, iso_c2);

			vert_list[1] = vert_interp(surf_thresh, c2, c3, iso_c2, iso_c3);
			norm_list[1] = vert_interp(surf_thresh, n2, n3, iso_c2, iso_c3);

			vert_list[2] = vert_interp(surf_thresh, c3, c4, iso_c3, iso_c4);
			norm_list[2] = vert_interp(surf_thresh, n3, n4, iso_c3, iso_c4);

			vert_list[3] = vert_interp(surf_thresh, c4, c1, iso_c4, iso_c1);
			norm_list[3] = vert_interp(surf_thresh, n4, n1, iso_c4, iso_c1);

			vert_list[4] = vert_interp(surf_thresh, c5, c6, iso_c5, iso_c6);
			norm_list[4] = vert_interp(surf_thresh, n5, n6, iso_c5, iso_c6);

			vert_list[5] = vert_interp(surf_thresh, c6, c7, iso_c6, iso_c7);
			norm_list[5] = vert_interp(surf_thresh, n6, n7, iso_c6, iso_c7);

			vert_list[6] = vert_interp(surf_thresh, c7, c8, iso_c7, iso_c8);
			norm_list[6] = vert_interp(surf_thresh, n7, n8, iso_c7, iso_c8);

			vert_list[7] = vert_interp(surf_thresh, c8, c5, iso_c8, iso_c5);
			norm_list[7] = vert_interp(surf_thresh, n8, n5, iso_c8, iso_c5);

			vert_list[8] = vert_interp(surf_thresh, c1, c5, iso_c1, iso_c5);
			norm_list[8] = vert_interp(surf_thresh, n1, n5, iso_c1, iso_c5);

			vert_list[9] = vert_interp(surf_thresh, c2, c6, iso_c2, iso_c6);
			norm_list[9] = vert_interp(surf_thresh, n2, n6, iso_c2, iso_c6);

			vert_list[10] = vert_interp(surf_thresh, c3, c7, iso_c3, iso_c7);
			norm_list[10] = vert_interp(surf_thresh, n3, n7, iso_c3, iso_c7);

			vert_list[11] = vert_interp(surf_thresh, c4, c8, iso_c4, iso_c8);
			norm_list[11] = vert_interp(surf_thresh, n4, n8, iso_c4, iso_c8);



	
			for (i = 0; imageLoad(tri_table, ivec2(i, cube_index)).r != -1.0; i += 3)
			{
				uint index_offset = atomicAdd(triangles_drawn.elements[0], 1);
				uint vert_index1 = uint(imageLoad(tri_table, ivec2(i, cube_index)).r);
				uint vert_index2 = uint(imageLoad(tri_table, ivec2(i + 1, cube_index)).r);
				uint vert_index3 = uint(imageLoad(tri_table, ivec2(i + 2, cube_index)).r);

				vertices.elements[index_offset * 3] = vec4(vert_list[vert_index1], 1.0);
				vertices.elements[(index_offset * 3) + 1] = vec4(vert_list[vert_index2], 1.0);
				vertices.elements[(index_offset * 3) + 2] = vec4(vert_list[vert_index3], 1.0);

				normals.elements[index_offset * 3] = vec4(norm_list[vert_index1], 1.0);
				normals.elements[(index_offset * 3) + 1] = vec4(norm_list[vert_index2], 1.0);
				normals.elements[(index_offset * 3) + 2] = vec4(norm_list[vert_index3], 1.0);

			}
	#endif
	}
}