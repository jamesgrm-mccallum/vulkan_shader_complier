shader vertex {
    input vec3 inPos;
    input vec3 inColor;
    uniform mat4 uMVP;

    
    tmp = 1.0 * 2.0 * 3.0;
    
    gl_Position = uMVP * vec4(inPos * 1.0, 1.0) + vec4(0.0, 0.0, 0.0, 0.0);
    
}
