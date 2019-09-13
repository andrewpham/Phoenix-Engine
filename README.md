# Phoenix Engine
Real-time physically based renderin'.

### Features
Variance shadow mapping with Gaussian filtering using a ping-pong FBO or compute shaders, percentage-closer soft shadows and reflective shadow maps using Poisson disk sampling, cascaded shadow mapping, screen space reflections using deferred shading and ray marching, physically based rendering with image-based lighting and a spherical harmonics probe, tiled deferred shading using compute shaders, and indirect illumination using voxel cone tracing.

Mesh loading using Assimp.  
Texture loading using nothings' stb library.

### Dependencies
Glad, GLFW, OpenGL Mathematics (GLM), and Assimp.

### Screenshots
![](images/ibl.png "Image-Based Lighting")
![](images/ibl2.png "Image-Based Lighting")
![](images/vct.png "Indirect Illumination Using Voxel Cone Tracing")
![](images/vct2.png "Indirect Illumination Using Voxel Cone Tracing")
![](images/vct3.png "Indirect Illumination Using Voxel Cone Tracing")
![](images/vct4.png "3D Texture (Voxel) Representation of the Scene")
![](images/pcss.png "Percentage-Closer Soft Shadows")
![](images/pcss_sm.png "Percentage-Closer Soft Shadows Shadow Map")
![](images/vsm.png "Variance Shadow Mapping")
![](images/vsm_sm.png "Variance Shadow Mapping Shadow Map")
![](images/csm.png "Cascaded Shadow Mapping")
![](images/ssr.png "Screen Space Reflections")
![](images/tds.png "Tiled Deferred Shading")
![](images/hair.png "Hair Rendering")

### Resources
Brian Karis' [Real Shading in Unreal Engine 4](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf)  
Real-Time Rendering  
Andrew Lauritzen on Variance Shadow Maps in [GPU Gems 3](https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch08.html)  
[Variance Shadow Mapping](http://developer.download.nvidia.com/SDK/10/direct3d/Source/VarianceShadowMapping/Doc/VarianceShadowMapping.pdf) by Kevin Myers  
Jan Kautz' [Exponential Shadow Maps](http://jankautz.com/publications/esm_gi08.pdf)  
Cyril Crassin's [Interactive Indirect Illumination Using Voxel Cone Tracing](https://research.nvidia.com/sites/default/files/pubs/2011-09_Interactive-Indirect-Illumination/GIVoxels-pg2011-authors.pdf)  
[Graphics Deep Dive: Cascaded voxel cone tracing in The Tomorrow Children](https://www.gamasutra.com/view/news/286023/Graphics_Deep_Dive_Cascaded_voxel_cone_tracing_in_The_Tomorrow_Children.php)  
Bartlomiej Wronski on [The future of screenspace reflections](https://www.gamasutra.com/blogs/BartlomiejWronski/20140129/209609/The_future_of_screenspace_reflections.php)  
[Assassin's Creed IV: Black Flag - Road to Next-Gen Graphics](https://www.gdcvault.com/play/1020397/Assassin-s-Creed-IV-Black)  
[GDC follow-up: Screenspace reflections filtering and up-sampling](https://bartwronski.com/2014/03/23/gdc-follow-up-screenspace-reflections-filtering-and-up-sampling/)  
Randima Fernando's [Percentage Closer Soft Shadows](http://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf) Paper  
Andrew Lauritzen's [Deferred Rendering for Current and Future Rendering Pipelines](https://software.intel.com/sites/default/files/m/d/4/1/d/8/lauritzen_deferred_shading_siggraph_2010.pdf)  
Johan Andersson on [DirectX 11 Rendering in Battlefield 3](http://www.dice.se/wp-content/uploads/2014/12/GDC11_DX11inBF3_Public.pdf)  
[NVIDIA Turing GPU Architecture Whitepaper](https://www.nvidia.com/content/dam/en-zz/Solutions/design-visualization/technologies/turing-architecture/NVIDIA-Turing-Architecture-Whitepaper.pdf)  
Robin Green's [Spherical Harmonic Lighting: The Gritty Details](http://silviojemma.com/public/papers/lighting/spherical-harmonic-lighting.pdf) Document  
Ravi Ramamoorthi's [An Efficient Representation for Irradiance Environment Maps](http://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf) Paper  
[Stupid Spherical Harmonics (SH) Tricks](https://www.ppsloan.org/publications/StupidSH36.pdf)  
Carsten Dachsbacher's [Reflective Shadow Maps](http://www.klayge.org/material/3_12/GI/rsm.pdf) Paper  
Thorsten Scheuermann’s SIGGRAPH Talk on [Practical Real-Time Hair Rendering and Shading](https://pdfs.semanticscholar.org/aa4c/9b498a48ec9eff1d93eaca646b3c9d2490b1.pdf)  
Thorsten Scheuermann’s GDC Talk on [Hair Rendering and Shading](http://web.engr.oregonstate.edu/~mjb/cs519/Projects/Papers/HairRendering.pdf)  
[Practical Real-Time Hair Rendering and Shading](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.607.1272&rep=rep1&type=pdf) Paper