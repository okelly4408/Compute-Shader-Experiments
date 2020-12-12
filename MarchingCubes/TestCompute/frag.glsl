#version 450 core
uniform sampler2D rock_texture;
uniform sampler2D grass_texture;
in GEOM_OUT
{
	vec3 position;
	vec3 normal;
} geom_out;
out vec4 color;
vec4 triplanar(sampler2D tex, vec3 normal, vec3 pos, out vec3 blend)
{
	blend = abs(normal);
	blend = normalize(max(blend, vec3(0.00001)));
	float b = blend.x + blend.y + blend.z;
	blend /= b;
	vec4 x_tex = texture(tex, pos.yz);
	vec4 y_tex = texture(tex, pos.xz);
	vec4 z_tex = texture(tex, pos.xy);
	return x_tex * blend.x + y_tex * blend.y + z_tex * blend.z;
}
void main(void)
{
	vec3 normal = geom_out.normal;
	float diffuse = max(dot(normal, -normalize(vec3(-sqrt(2.0)/2.0, sqrt(2.0)/2.0, -sqrt(2.0)/2.0))), 0.0) + 0.25;
	float blend = smoothstep(0.1, 0.6, 1.0 - length(normal.xy));
	vec3 blend2;
	vec4 rock_color = triplanar(rock_texture, normal, geom_out.position, blend2);
	vec3 blend3;
	vec4 grass_color = triplanar(grass_texture, normal, geom_out.position, blend3);
	float blend4 = smoothstep(0.07, 0.4, (1.0 - length(normal.xz)) * float(normal.y < 0));
	vec3 tex_color = mix(rock_color.rgb, grass_color.rgb, blend4);
	//vec3 c = mix(vec3(0.6, 0.3, 0.3), vec3(0.3, 0.6, 0.6), blend);
	color = vec4((normal + 1.0) / 2.0, 1.0f);
	//color = vec4(tex_color.xyz * diffuse, 1.0);
	//color = vec4(tex_color.xyz, 1.0);
}