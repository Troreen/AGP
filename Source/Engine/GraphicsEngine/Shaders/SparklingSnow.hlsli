// SPARKLING SNOW WORKSHEET
// Start with Docs/MaterialShaderGuide.md. Helper details are in Docs/MaterialShaderHelpers.md.
// This file is deliberately only comments. It will not compile as a material until you fill it in.
// Keep it unassigned in the scene until it is ready to compile.
// For an exported scene, the master material name must match this filename: SparklingSnow.

// 1. BRING IN THE TOOLS
// Include MaterialParameters.hlsli for the material inputs and outputs.
// Include Normal.hlsli for working with normal maps.
// Include MaterialUtilities.hlsli for the sun, camera, Fresnel, and screen UV helpers.

// 2. GIVE THE THREE TEXTURES NAMES
// Use one material with three DDS files:
// - The packed 1024 snow texture.
// - The 512 detail normal texture.
// - The 512 black-and-white sparkle texture.
// Give each texture its own name and slot. Extra texture slots start at 4 and must stay below 16.
// Choose the actual DDS files in the material's scene parameters, using those same names.
// Check what each channel of the packed snow texture contains before reading it.

// 3. KEEP SETTINGS EASY TO CHANGE
// Decide which values you want to adjust, such as texture repeat size and sparkle brightness.
// Look at an existing material's MaterialBuffer to see where adjustable values belong.

// 4. ADD THE MATERIAL FUNCTIONS
// Look at an existing material for the Material_Vertex and Material_Pixel function signatures.
// Keep both functions. Material_Vertex can have an empty body for now.
// Do the surface work below inside Material_Pixel.

// 5. START WITH PLAIN SNOW
// Choose a base color and roughness.
// Check this simple version in the game before adding the other parts.

// 6. COMBINE THE NORMAL MAPS
// Read the packed snow texture's normal information and the detail normal texture.
// Give the detail texture its own repeat size.
// Follow the video's manual unpacking: take red and green, multiply by 2, then subtract 1.
// Pass those signed XY values to Normal_DeriveZ to recover the missing upward part.
// Keep the resulting direction one unit long before blending.
// Normal_DecodeRG is also available as a shortcut for all of those steps.
// Normal_BlendAngleCorrected combines the two decoded normal directions.
// Normal_TangentToWorld converts the result into the world direction our Normal output expects.

// 7. READ THE SPARKLE TEXTURE TWICE
// First, read it using the object's UVs, with the repeat amount shown in the reference.
// Then read the SAME texture using screen UVs from UV_ScreenTexture.

// 8. WORK OUT THE SUN MASK
// Sun_DirectionToLight gives the world direction toward the sun, or zero if it is absent or off.
// Use the vertex normal and the reference's dot, saturate, and power steps.
// Think about which parts of the surface this should allow to sparkle.

// 9. TRY THE FRESNEL PART
// View_DirectionToCamera gives the direction from the surface toward the camera.
// Fresnel_Schlick makes a rim effect. Keep the normal and camera direction in world space.
// Follow the reference to decide how this changes the snow's appearance.

// 10. JOIN THE PARTS
// Follow the reference's wires to combine the patterns and masks.
// Its sparkle result goes into Emission, adding brightness beyond ordinary surface lighting.
// Keep each part easy to inspect before combining everything.

// 11. CHECK THE RESULT
// Build the game and restart it after shader changes.
// Check close up, far away, in shadow, and while moving the camera.
// If a pattern is confusing, show it alone as the base color and turn emission off.
