CSCI 420
Dr. Jernej

Assignment 1 - Height Fields
Deadline: Wednesday, February 20

Programmed by Yuka Murata
USC ID: 6434262018

Basic Components
1) Use OpenGL core profile.
2) Handle a 256x256 image and window size of 1280x720.
3) Be able to render height field as point, lines, and solid triangles.
4) Render as a perspective view, utilizing GL's depth buffer for hidden surface removal.
5) Be able to rotate, move and scale the height field.
6) Color vertices using smooth gradient.
7) Take up to 300 screenshots.

Usage
1) Use trackpad to rotate. Hold ALT to rotate around z axis.
2) Hold SHIFT and use trackpad to scale. Hold SHIFT and ALT to scale in the z direction.
3) Hold t key and use trackpad to translate. Hold t key and ALT to translate in the z direction.
4) Press p for points mode.
5) Press w for wireframe mode.
6) Press s for solid mode.
7) Press c to display points on top of solid triangles (combo).

Extra Features
1) Work with any image sizes if it is explicitly written in a file name. So, this works for the sample height field images.
2) Display points on top of solid triangles by using glPolygonOffset.
3) I tried to use glDrawElement but somewhat it did not work. I left the code needed, so please check it for a partial credit.
