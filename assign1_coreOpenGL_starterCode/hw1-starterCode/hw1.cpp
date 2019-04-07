/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: ymurata
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

GLuint program;

int imageSize = 256;  // size of width and height
int numVertecisP = 0; // # of vertices in point mode
int numVertecisW = 0; // # of vertices in wireframe mode
int numVertecisS = 0; // # of vertices in solid mode

// container for colors and positions
vector<float> colors, positions;
vector<unsigned int> indices;

typedef enum { POINT, WIREFRAME, SOLID, COMBO } DISPLAY_MODE;
DISPLAY_MODE displayMode = SOLID;

GLuint vboP, vboW, vboS, vboPointYellow, vboElement;
GLuint vaoP, vaoW, vaoS, vaoPointYellow, vaoElement;

OpenGLMatrix openGLMatrix; // open GL helper
BasicPipelineProgram pipelineProgram;
// handles or modelview and rojectionmatrices
GLint h_modelViewMatrix, h_projectionMatrix;

int frame = 0; // frame # for screenshots

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{
  // clear the color and depth buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // ModelView Matrix
  // prepare the modelview matrix
  openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
  openGLMatrix.LoadIdentity();
  openGLMatrix.LookAt(-128, 200, 300, 128, 0, -128, 0, 1, 0);

  // Transformation
  openGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  openGLMatrix.Rotate(landRotate[0], 1.0, 0.0, 0.0);
  openGLMatrix.Rotate(landRotate[1], 0.0, 1.0, 0.0);
  openGLMatrix.Rotate(landRotate[2], 0.0, 0.0, 1.0);
  openGLMatrix.Scale(landScale[0], landScale[1], landScale[2]); // scale the onject

  float m[16]; // column-major
  openGLMatrix.GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);

  // Projection Matrix
  openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
  float p[16]; // column-major
  openGLMatrix.GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);

  GLint first = 0;
  
  switch(displayMode)
  {
    // display points
  	case POINT:
      glBindVertexArray(vaoP); // bind the VAO
  	  glDrawArrays(GL_POINTS, first, numVertecisP);
  	break;

    // display lines
  	case WIREFRAME:
      glBindVertexArray(vaoW); // bind the VAO
  	  glDrawArrays(GL_LINES, first, numVertecisW);
  	break;

    //display triangles
  	case SOLID:
      glBindVertexArray(vaoS); // bind the VAO
  	  glDrawArrays(GL_TRIANGLES, first, numVertecisS);

      /* Use glDrawElements
      count = indices.size();
      glBindVertexArray(vaoElement);
      glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices.data());
      */
  	break;

    // display both points and triangles
    case COMBO:
      glBindVertexArray(vaoPointYellow);
      glPolygonOffset(1, 1);
      glDrawArrays(GL_POINTS, first, numVertecisP);
      glBindVertexArray(vaoS); // bind the VAO
      glDrawArrays(GL_TRIANGLES, first, numVertecisS);
    break;
  }

  glBindVertexArray(0); // unbind the VAO
  glutSwapBuffers(); // swap the buffers:
}

void idleFunc()
{
  // take 300 screenshots
  if (frame < 300)
  {
    saveScreenshot(("screenshots/screenshot" + string(3 - to_string(frame).length(), '0') + to_string(frame) + ".jpg").c_str());
    frame++;
  }

  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // setup perspective matrix
  openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
  openGLMatrix.LoadIdentity();
  openGLMatrix.Perspective(45.0, 1.0 * w / h, 0.01, 5000.0);
  openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case 'x':
      // take a screenshot
      saveScreenshot(("screenshots/screenshot" + string(3 - to_string(frame).length(), '0') + to_string(frame) + ".jpg").c_str());
      frame++;
    break;

    // use t key to translate object
    case 't':
      controlState = TRANSLATE;
    break;

    // use the following keys to switch display mode

    case 'p':
      displayMode = POINT;
    break;

    case 'w':
      displayMode = WIREFRAME;
    break;

    case 's':
      displayMode = SOLID;
    break;

    case 'c':
      displayMode = COMBO;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

// initialize VBO
void initVBO()
{
  // init VBO’s size, but don’t assign any data to it
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size() + sizeof(float) * colors.size(), nullptr, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * positions.size(), static_cast<void*>(positions.data()));
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), sizeof(float) * colors.size(), static_cast<void*>(colors.data()));
}

// read height map and fill positions and colors
void getData(string mode)
{
  positions.clear();
  colors.clear();
  float scale = 0.1f;
  float height, color;

  if (mode == "point")
  {
    for (int i = 0; i < imageSize; i++) {
      for (int j = 0; j < imageSize; j++) {
    	  height = heightmapImage->getPixel(i, j, 0);
      	color = 1.0f * height / 255;

      	positions.push_back((float) i);
        positions.push_back(scale * height);
        positions.push_back((float) -j);

        colors.push_back(color); // R
        colors.push_back(color); // G
        colors.push_back(color); // B
        colors.push_back(1.0f); // A
      }
    }
    numVertecisP = positions.size() / 3;
  }
  else if (mode == "wireframe")
  {
    for (int i = 0; i < imageSize; i++) {
      for (int j = 0; j < imageSize; j++) {

        float height;
        if (j != imageSize -1)
        {
          height = heightmapImage->getPixel(i, j, 0);
          color = 1.0f * height / 255;

          positions.push_back((float) i);
          positions.push_back(scale * height);
          positions.push_back((float) -j);
 
          colors.push_back(color); // R
          colors.push_back(color); // G
          colors.push_back(color); // B
          colors.push_back(1.0f); // A

          height = heightmapImage->getPixel(i, j+1, 0);
          color = 1.0f * height / 255;

          positions.push_back((float) i);
          positions.push_back(scale * height);
          positions.push_back((float) -(j+1));
 
          colors.push_back(color); // R
          colors.push_back(color); // G
          colors.push_back(color); // B
          colors.push_back(1.0f); // A
        }

        if (i != imageSize -1)
        {
          height = heightmapImage->getPixel(i, j, 0);
          color = 1.0f * height / 255;

          positions.push_back((float) i);
          positions.push_back(scale * height);
          positions.push_back((float) -j);
 
          colors.push_back(color); // R
          colors.push_back(color); // G
          colors.push_back(color); // B
          colors.push_back(1.0f); // A

          height = heightmapImage->getPixel(i+1, j, 0);
          color = 1.0f * height / 255;

          positions.push_back((float) (i+1));
          positions.push_back(scale * height);
          positions.push_back((float) -j);
 
          colors.push_back(color); // R
          colors.push_back(color); // G
          colors.push_back(color); // B
          colors.push_back(1.0f); // A
        }
      }
    }
    numVertecisW = positions.size() / 3;
  }
  else if (mode == "solid")
  {
    for (int i = 0; i < imageSize-1; i++) {
      for (int j = 0; j < imageSize-1; j++) {

        // upper right triangle
        height = heightmapImage->getPixel(i, j, 0);
        color = 1.0f * height / 255;

        positions.push_back((float) i);
        positions.push_back(scale * height);
        positions.push_back((float) -j);

        colors.push_back(color); // R
        colors.push_back(color); // G
        colors.push_back(color); // B
        colors.push_back(1.0f); // A

        height = heightmapImage->getPixel(i, j+1, 0);
        color = 1.0f * height / 255;

        positions.push_back((float) i);
        positions.push_back(scale * height);
        positions.push_back((float) -(j+1));

        colors.push_back(color); // R
        colors.push_back(color); // G
        colors.push_back(color); // B
        colors.push_back(1.0f); // A

        height = heightmapImage->getPixel(i+1, j+1, 0);
        color = 1.0f * height / 255;

        positions.push_back((float) (i+1));
        positions.push_back(scale * height);
        positions.push_back((float) -(j+1));

        colors.push_back(color); // R
        colors.push_back(color); // G
        colors.push_back(color); // B
        colors.push_back(1.0f); // A

        // bottom left triangle

        height = heightmapImage->getPixel(i, j, 0);
        color = 1.0f * height / 255;

        positions.push_back((float) i);
        positions.push_back(scale * height);
        positions.push_back((float) -j);

        colors.push_back(color); // R
        colors.push_back(color); // G
        colors.push_back(color); // B
        colors.push_back(1.0f); // A

        height = heightmapImage->getPixel(i+1, j, 0);
        color = 1.0f * height / 255;

        positions.push_back((float) (i+1));
        positions.push_back(scale * height);
        positions.push_back((float) -j);

        colors.push_back(color); // R
        colors.push_back(color); // G
        colors.push_back(color); // B
        colors.push_back(1.0f); // A

        height = heightmapImage->getPixel(i+1, j+1, 0);
        color = 1.0f * height / 255;

        positions.push_back((float) (i+1));
        positions.push_back(scale * height);
        positions.push_back((float) -(j+1));

        colors.push_back(color); // R
        colors.push_back(color); // G
        colors.push_back(color); // B
        colors.push_back(1.0f); // A
      }
    }
    numVertecisS = positions.size() / 3;
  }
  else if (mode == "combo")
  {
    // store only yellow points to make it look nicer
    for (int i = 0; i < imageSize; i++) {
      for (int j = 0; j < imageSize; j++) {
        height = heightmapImage->getPixel(i, j, 0);
        color = 1.0f * height / 255;

        positions.push_back((float) i);
        positions.push_back(scale * height);
        positions.push_back((float) -j);

        colors.push_back(1.0f); // R
        colors.push_back(1.0f); // G
        colors.push_back(0.0f); // B
        colors.push_back(1.0f); // A
      }
    }
  }

  // store indices for glDrawElement
  for (int i = 0; i < imageSize-1; i++)
  {
    for (int j = 0; j < imageSize-1; j++)
    {
      indices.push_back(i*imageSize+j);
      indices.push_back((i+1)*imageSize+j);
      indices.push_back(i*imageSize+j+1);
      indices.push_back(i*imageSize+j+1);
      indices.push_back((i+1)*imageSize+j);
      indices.push_back((i+1)*imageSize+j+1);
    }
  }
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // get the size of image
  string numbers = "0123456789";
  string num = "";
  size_t idx = string(argv[1]).find_first_of(numbers.c_str());

  while (idx != string::npos) {
    num = num + string(argv[1]).at(idx);
    idx = string(argv[1]).find_first_of(numbers.c_str(), idx+1);
  }
  // set image size
  if (num != "") imageSize = stoi(num);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // enable hidden surface removal:
  glEnable(GL_DEPTH_TEST);

  pipelineProgram.Init("../openGLHelper-starterCode");
  // bind the pipeline program (run this before glUniformMatrix4fv)
  pipelineProgram.Bind();

  // Create handles for modelview and projection matrices
  program = pipelineProgram.GetProgramHandle();
  h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
  h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");

  // set up vbo and vao for point mode below

  // create VBO
  glGenBuffers(1, &vboP);
  glBindBuffer(GL_ARRAY_BUFFER, vboP);

  // create VAO
  glGenVertexArrays(1, &vaoP);
  glBindVertexArray(vaoP); // bind the VAO

  // read file and fill positions and colors
  getData("point");
  initVBO(); // create VBO
  
  GLboolean normalized = GL_FALSE;
  GLsizei stride = 0;

  // bind the VBO "buffer" (must be previously created)
  glBindBuffer(GL_ARRAY_BUFFER, vboP);
  // get location index of the "position" shader variable
  GLuint loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  const void * offset = (const void*) 0;
  // set the layout of the "position" attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  // get the location index of the "color" shader variable
  loc = glGetAttribLocation(program, "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float) * positions.size());
  // set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glBindVertexArray(0); // unbind the VAO

  // set up vbo and vao for wireframe mode below

  // create VBO
  glGenBuffers(1, &vboW);
  glBindBuffer(GL_ARRAY_BUFFER, vboW);

  // create VAO
  glGenVertexArrays(1, &vaoW);
  glBindVertexArray(vaoW); // bind the VAO

  // read file and fill positions and colors
  getData("wireframe");
  initVBO(); // create VBO

  // bind the VBO "buffer" (must be previously created)
  glBindBuffer(GL_ARRAY_BUFFER, vboW);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  // set the layout of the "position" attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  // get the location index of the "color" shader variable
  loc = glGetAttribLocation(program, "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float) * positions.size());
  // set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glBindVertexArray(0); // unbind the VAO

  // set up vbo and vao for solid mode below

  // create VBO
  glGenBuffers(1, &vboS);
  glBindBuffer(GL_ARRAY_BUFFER, vboS);

  // create VAO
  glGenVertexArrays(1, &vaoS);
  glBindVertexArray(vaoS); // bind the VAO

  // read file and fill positions and colors
  getData("solid");
  initVBO(); // create VBO

  // bind the VBO "buffer" (must be previously created)
  glBindBuffer(GL_ARRAY_BUFFER, vboS);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  // set the layout of the "position" attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  // get the location index of the "color" shader variable
  loc = glGetAttribLocation(program, "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float) * positions.size());
  // set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glBindVertexArray(0); // unbind the VAO

  // set up vbo and vao for combo mode

  // create VBO
  glGenBuffers(1, &vboPointYellow);
  glBindBuffer(GL_ARRAY_BUFFER, vboPointYellow);

  // create VAO
  glGenVertexArrays(1, &vaoPointYellow);
  glBindVertexArray(vaoPointYellow); // bind the VAO

  // read file and fill positions and colors
  getData("combo");
  initVBO(); // create VBO

  // bind the VBO "buffer" (must be previously created)
  glBindBuffer(GL_ARRAY_BUFFER, vboPointYellow);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  // set the layout of the "position" attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  // get the location index of the "color" shader variable
  loc = glGetAttribLocation(program, "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float) * positions.size());
  // set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glBindVertexArray(0); // unbind the VAO

  // set up vbo and vao for glDrawElement
  /*
  // create VBO
  glGenBuffers(1, &vboElement);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboElement);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

  // create VAO
  glGenVertexArrays(1, &vaoElement);
  glBindVertexArray(vaoElement); // bind the VAO

  // bind the VBO "buffer" (must be previously created)
  glBindBuffer(GL_ARRAY_BUFFER, vboS);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboElement);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  // set the layout of the "position" attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  // get the location index of the "color" shader variable
  loc = glGetAttribLocation(program, "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float) * positions.size());
  // set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glBindVertexArray(0); // unbind the VAO
  */
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


