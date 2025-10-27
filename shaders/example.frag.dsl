shader fragment {
  input vec3 fragColor;

  t = 10.0 - 5.0 + 2.0 * 3.0;   // → 10.0 - 5.0 + 6.0 → 11.0 (fold), then removed (DCE)


  gl_FragColor = vec4(fragColor * 3.0 * 2.0 * 0.5, 1.0) + vec4(0.0, 0.0, 0.0, 0.0);
}
