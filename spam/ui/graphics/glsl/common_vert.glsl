#version 330
layout(location = 0) in vec3 aPos;

uniform mat4 modelview;
uniform mat4 projection;

void main() {
  gl_Position = projection * modelview * vec4(aPos, 1.0);
}
