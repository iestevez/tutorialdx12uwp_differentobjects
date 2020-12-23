#DirectX12 rendering of different objects with multiple-instances

Compiler: Visual Studio 2019 

Windows SDK 19041

For Windows SDK18362, comment define _SDK19041 at the beginning of pch.h

The code shows how to render different objects with multi-instances with DirectX12
Includes also a HUD programmed with Direct2D and DirectWrite.

The developed controller allows for  first person movement (AWSD-QE) and change
the point of view with mouse and  pressing left button at the same time.

Issues: 

1 Some problems with windows resizing probably at the calculation of the
projection matrix.

2 Example meshes with uncorrect normals at vertex.

