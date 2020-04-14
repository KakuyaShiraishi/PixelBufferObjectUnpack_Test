#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include "glExtension.h"                     
#include "Timer.h"


void displayCB();
void reshapeCB(int w, int h);
void timerCB(int millisec);
void idleCB();
void keyboardCB(unsigned char key, int x, int y);
void mouseCB(int button, int stat, int x, int y);
void mouseMotionCB(int x, int y);

void exitCB();

void initGL();
int  initGLUT(int argc, char **argv);
bool initSharedMem();
void clearSharedMem();
void initLights();
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ);
void updatePixels(GLubyte* dst, int size);
void drawString(const char *str, int x, int y, float color[4], void *font);
void drawString3D(const char *str, float pos[3], float color[4], void *font);
void showInfo();
void showTransferRate();
void printTransferRate();


const int    SCREEN_WIDTH    = 800;
const int    SCREEN_HEIGHT   = 600;
const float  CAMERA_DISTANCE = 2.0f;
const int    TEXT_WIDTH      = 8;
const int    TEXT_HEIGHT     = 13;
const int    IMAGE_WIDTH     = 2048;
const int    IMAGE_HEIGHT    = 2048;
const int    CHANNEL_COUNT   = 4;
const int    DATA_SIZE       = IMAGE_WIDTH * IMAGE_HEIGHT * CHANNEL_COUNT;
const GLenum PIXEL_FORMAT    = GL_BGRA;

void *font = GLUT_BITMAP_8_BY_13;
GLuint pboIds[2];                  
GLuint textureId;                  
GLubyte* imageData = 0;             
int screenWidth;
int screenHeight;
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance;
bool pboSupported;
int pboMode;
int drawMode = 0;
Timer timer, t1, t2;
float copyTime, updateTime;



int main(int argc, char **argv)
{
    initSharedMem();

    atexit(exitCB);

    initGLUT(argc, argv);
    initGL();

    glExtension& ext = glExtension::getInstance();
    pboSupported = ext.isSupported("GL_ARB_pixel_buffer_object");
    if(pboSupported)
    {
        std::cout << "Video card supports GL_ARB_pixel_buffer_object." << std::endl;
    }
    else
    {
        std::cout << "[ERROR] Video card does not supports GL_ARB_pixel_buffer_object." << std::endl;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMAGE_WIDTH, IMAGE_HEIGHT, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
    glBindTexture(GL_TEXTURE_2D, 0);

#ifdef _WIN32
    if(ext.isSupported("WGL_EXT_swap_control"))
    {
        wglSwapIntervalEXT(0);
        std::cout << "Video card supports WGL_EXT_swap_control." << std::endl;
    }
#endif

    if(pboSupported)
    {
       
        glGenBuffers(2, pboIds);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

   
    timer.start();

    glutMainLoop(); 

    return 0;
}

int initGLUT(int argc, char **argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_ALPHA); 

    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);    
    glutInitWindowPosition(100, 100);                  

    int handle = glutCreateWindow(argv[0]);     

    glutDisplayFunc(displayCB);
    //glutTimerFunc(33, timerCB, 33);             
    glutIdleFunc(idleCB);                
    glutReshapeFunc(reshapeCB);
    glutKeyboardFunc(keyboardCB);
    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);

    return handle;
}


void initGL()
{
    //@glShadeModel(GL_SMOOTH);                  
    glShadeModel(GL_FLAT);                  
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);   

    //@glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    //@glEnable(GL_LIGHTING);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glClearColor(0, 0, 0, 0);                   
    glClearStencil(0);                         
    glClearDepth(1.0f);                       
    glDepthFunc(GL_LEQUAL);

    //@initLights();
}

void drawString(const char *str, int x, int y, float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); 
    glDisable(GL_LIGHTING);   
    glDisable(GL_TEXTURE_2D);

    glColor4fv(color);         
    glRasterPos2i(x, y);       

    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopAttrib();
}



void drawString3D(const char *str, float pos[3], float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); 
    glDisable(GL_LIGHTING);    
    glDisable(GL_TEXTURE_2D);

    glColor4fv(color);         
    glRasterPos3fv(pos);       

    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopAttrib();
}


bool initSharedMem()
{
    screenWidth = SCREEN_WIDTH;
    screenHeight = SCREEN_HEIGHT;

    mouseLeftDown = mouseRightDown = false;
    mouseX = mouseY = 0;

    cameraAngleX = cameraAngleY = 0;
    cameraDistance = CAMERA_DISTANCE;

    drawMode = 0; // 0:fill, 1: wireframe, 2:points

    imageData = new GLubyte[DATA_SIZE];
    memset(imageData, 0, DATA_SIZE);

    return true;
}



void clearSharedMem()
{
    delete [] imageData;
    imageData = 0;

    glDeleteTextures(1, &textureId);

    if(pboSupported)
    {
        glDeleteBuffers(2, pboIds);
    }
}



void initLights()
{
    GLfloat lightKa[] = {.2f, .2f, .2f, 1.0f};  // ambient light
    GLfloat lightKd[] = {.7f, .7f, .7f, 1.0f};  // diffuse light
    GLfloat lightKs[] = {1, 1, 1, 1};           // specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

    float lightPos[4] = {0, 0, 20, 1}; 
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);                      
}


void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posX, posY, posZ, targetX, targetY, targetZ, 0, 1, 0); // eye(x,y,z), focal(x,y,z), up(x,y,z)
}


void updatePixels(GLubyte* dst, int size)
{
    static int color = 0;

    if(!dst)
        return;

    int* ptr = (int*)dst;

    for(int i = 0; i < IMAGE_HEIGHT; ++i)
    {
        for(int j = 0; j < IMAGE_WIDTH; ++j)
        {
            *ptr = color;
            ++ptr;
        }
        color += 257;
    }
    ++color;           
}


void showInfo()
{
   
    glPushMatrix();                   
    glLoadIdentity();                  

    glMatrixMode(GL_PROJECTION);    
    glPushMatrix();                 
    glLoadIdentity();               
    gluOrtho2D(0, screenWidth, 0, screenHeight); 

    float color[4] = {1, 1, 1, 1};

    std::stringstream ss;
    ss << "PBO: ";
    if(pboMode == 0)
        ss << "off" << std::ends;
    else if(pboMode == 1)
        ss << "1 PBO" << std::ends;
    else if(pboMode == 2)
        ss << "2 PBOs" << std::ends;

    drawString(ss.str().c_str(), 1, screenHeight-TEXT_HEIGHT, color, font);
    ss.str(""); // clear buffer

    ss << std::fixed << std::setprecision(3);
    ss << "Updating Time: " << updateTime << " ms" << std::ends;
    drawString(ss.str().c_str(), 1, screenHeight-(2*TEXT_HEIGHT), color, font);
    ss.str("");

    ss << "Copying Time: " << copyTime << " ms" << std::ends;
    drawString(ss.str().c_str(), 1, screenHeight-(3*TEXT_HEIGHT), color, font);
    ss.str("");

    ss << "Press SPACE key to toggle PBO on/off." << std::ends;
    drawString(ss.str().c_str(), 1, 1, color, font);

    ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);

    glPopMatrix();               

    glMatrixMode(GL_MODELVIEW);    
    glPopMatrix();                   
}



void showTransferRate()
{
    static Timer timer;
    static int count = 0;
    static std::stringstream ss;

    ++count;
    double elapsedTime = timer.getElapsedTime();
    if(elapsedTime > 1.0)
    {
        ss.str("");
        ss << std::fixed << std::setprecision(1);
        ss << "Transfer Rate: " << (count / elapsedTime) * DATA_SIZE / (1024 * 1024) << " MB" << std::ends; // update fps string
        ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
        count = 0;                      
        timer.start();                 
    }

    glPushMatrix();                    
    glLoadIdentity();                

    glMatrixMode(GL_PROJECTION);       
    glPushMatrix();                    
    glLoadIdentity();                  
    //gluOrtho2D(0, IMAGE_WIDTH, 0, IMAGE_HEIGHT); 
    gluOrtho2D(0, screenWidth, 0, screenHeight);

    float color[4] = {1, 1, 0, 1};
    drawString(ss.str().c_str(), 200, 286, color, font);

    glPopMatrix();                    

    glMatrixMode(GL_MODELVIEW);        
    glPopMatrix();                     
}


void printTransferRate()
{
    const double INV_MEGA = 1.0 / (1024 * 1024);
    static Timer timer;
    static int count = 0;
    static std::stringstream ss;
    double elapsedTime;

    ++count;
    elapsedTime = timer.getElapsedTime();
    if(elapsedTime > 1.0)
    {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "Transfer Rate: " << (count / elapsedTime) * DATA_SIZE * INV_MEGA << " MB/s. (" << count / elapsedTime << " FPS)\n";
        std::cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
        count = 0;                   
        timer.start();                
    }
}



void toOrtho()
{
    glViewport(0, 0, (GLsizei)screenWidth, (GLsizei)screenHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



void toPerspective()
{
    glViewport(0, 0, (GLsizei)screenWidth, (GLsizei)screenHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(screenWidth)/screenHeight, 1.0f, 1000.0f); 

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}






void displayCB()
{
    static int index = 0;
    int nextIndex = 0;                  

    if(pboMode > 0)
    {
        if(pboMode == 1)
        {
            index = nextIndex = 0;
        }
        else if(pboMode == 2)
        {
            index = (index + 1) % 2;
            nextIndex = (index + 1) % 2;
        }

        t1.start();

        glBindTexture(GL_TEXTURE_2D, textureId);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[index]);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);

        t1.stop();
        copyTime = t1.getElapsedTimeInMilliSec();
        t1.start();

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nextIndex]);

        glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
        GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if(ptr)
        {
            updatePixels(ptr, DATA_SIZE);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);  
        }

        t1.stop();
        updateTime = t1.getElapsedTimeInMilliSec();
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    else
    {
        t1.start();

        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid*)imageData);

        t1.stop();
        copyTime = t1.getElapsedTimeInMilliSec();
        t1.start();
        updatePixels(imageData, DATA_SIZE);
        t1.stop();
        updateTime = t1.getElapsedTimeInMilliSec();
    }


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glPushMatrix();

    glTranslatef(0, 0, -cameraDistance);
    glRotatef(cameraAngleX, 1, 0, 0);   
    glRotatef(cameraAngleY, 0, 1, 0);   

    glBindTexture(GL_TEXTURE_2D, textureId);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(-1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);   glVertex3f( 1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);   glVertex3f( 1.0f,  1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);   glVertex3f(-1.0f,  1.0f, 0.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);


    showInfo();

    printTransferRate();

    glPopMatrix();

    glutSwapBuffers();
}


void reshapeCB(int width, int height)
{
    screenWidth = width;
    screenHeight = height;
    toPerspective();
}


void timerCB(int millisec)
{
    glutTimerFunc(millisec, timerCB, millisec);
    glutPostRedisplay();
}


void idleCB()
{
    glutPostRedisplay();
}


void keyboardCB(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 27:
        exit(0);
        break;

    case ' ':
        if(pboSupported)
        {
            ++pboMode;
            pboMode %= 3;
        }
        std::cout << "PBO mode: " << pboMode << std::endl;
        break;

    case 'd':
    case 'D':
        ++drawMode;
        drawMode %= 3;
        if(drawMode == 0)        // fill mode
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
        }
        else if(drawMode == 1)  // wireframe mode
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
        }
        else                    // point mode
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
        }
        break;

    default:
        ;
    }
}


void mouseCB(int button, int state, int x, int y)
{
    mouseX = x;
    mouseY = y;

    if(button == GLUT_LEFT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseLeftDown = true;
        }
        else if(state == GLUT_UP)
            mouseLeftDown = false;
    }

    else if(button == GLUT_RIGHT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseRightDown = true;
        }
        else if(state == GLUT_UP)
            mouseRightDown = false;
    }
}


void mouseMotionCB(int x, int y)
{
    if(mouseLeftDown)
    {
        cameraAngleY += (x - mouseX);
        cameraAngleX += (y - mouseY);
        mouseX = x;
        mouseY = y;
    }
    if(mouseRightDown)
    {
        cameraDistance -= (y - mouseY) * 0.2f;
        if(cameraDistance < 2.0f)
            cameraDistance = 2.0f;

        mouseY = y;
    }
}



void exitCB()
{
    clearSharedMem();
}
