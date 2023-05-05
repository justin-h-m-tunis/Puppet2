#include <Eigen/Dense>
#define _USE_MATH_DEFINES

#include <cmath>
#include <chrono>

#include "Default3d.h"
#include "Dynamic3d.hpp"
#include "level.h"
#include "player_camera.h"
#include "debug_camera.h"
#include "DebugGraphics.h"
#include "UI.h"
#include "Default2d.hpp"
#include "debug_menu.h"
#include "text_graphics.hpp"
#include "Debugger.h"
#include "ZMapper.h"
#include "sound.hpp"

#include <GLFW/glfw3.h>



void framebuffer_size_callback(GLFWwindow* window, int width, int height);


int main(void)
{
    

    GLFWwindow* window;

   
    // Initialize the library
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    
    //Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(1000, 1000, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //HboxGraphics hboxGraphics;

    std::vector<InternalObject> bases;
    std::vector<GameObject*> layout;//this being a vector which rearranges is horrible lol
    DebugCamera center("debug_cam");
    layout.push_back(&center);

    /*int n_debuggers = 10;
    for (int i = 0; i < n_debuggers; i++) {
        bases.emplace_back(Eigen::Matrix4f::Identity(), i + 1);
        bases[i].rotateY(2 * M_PI / n_debuggers * i);
        bases[i].update(window);
    }
    for (int i = 0; i < n_debuggers; i++) {
        debugs.emplace_back(Eigen::Matrix4f::Identity(), i + n_debuggers + 1, bases[i]);
        debugs[i].translate(Eigen::Vector3f(0, 0, 5));
    }
    for (auto& dbg : debugs) {
        layout.push_back(&dbg);
    }*/

    //Debugger right((Eigen::Matrix4f()<<1, 0, 0, .5, 0, 1, 0, 0, 0, 0, 1, .5, 0, 0, 0, 1).finished(), 2, default3d);
    //ZMapper zmapper;
    Texture rocky_texture("soil.jpg");
    Level cult_spiral_stairs(layout,window,new Model("spiral_staircase_cult_exit.obj"),&rocky_texture ,"spiral_staircase");
    Level cult_landing(layout, window, new Model("cult_exit_landing.obj"), &rocky_texture, "cult_exit_landing");
    Level cult_hallway1(layout, window, new Model("cult_exit_hallway.obj"), &rocky_texture, "cult_exit_hallway");
    Level cult_stairs1(layout, window, new Model("cult_ascencion_stairs.obj"), &rocky_texture, "cult_ascension_stairs");
    Level cult_impluvium(layout, window, new Model("cult_impluvium.obj"), &rocky_texture, "cult_impluvium");
    Level cult_ritual(layout, window, new Model("cult_ritual_room.obj"), &rocky_texture, "cult_ritual_room");
    Level path_to_town(layout, window, new Model("path_to_town.obj"), &rocky_texture, "path_to_town");


    cult_spiral_stairs.addNeighbor(&cult_landing);
    cult_spiral_stairs.addNeighbor(&cult_hallway1);
    cult_landing.addNeighbor(&path_to_town);
    cult_landing.addNeighbor(&cult_spiral_stairs);
    cult_hallway1.addNeighbor(&cult_spiral_stairs);
    cult_hallway1.addNeighbor(&cult_stairs1);
    cult_stairs1.addNeighbor(&cult_impluvium);
    cult_stairs1.addNeighbor(&cult_hallway1);
    cult_impluvium.addNeighbor(&cult_stairs1);
    cult_impluvium.addNeighbor(&cult_ritual);
    cult_ritual.addNeighbor(&cult_impluvium);
    path_to_town.addNeighbor(&cult_landing);
    


    ZMapper zmapper;
    cult_spiral_stairs.createZmapCollisionSurface(6,&zmapper);
    cult_hallway1.createZmapCollisionSurface(4,&zmapper);
    cult_landing.createZmapCollisionSurface(4, &zmapper);
    cult_stairs1.createZmapCollisionSurface(15, &zmapper);
    cult_impluvium.createZmapCollisionSurface(4, &zmapper);
    cult_ritual.createZmapCollisionSurface(4, &zmapper);
    path_to_town.createZmapCollisionSurface(6, &zmapper);

    Sound bkg_music("bkg_music", "EldenRingOSTGodskinApostles.wav");
    cult_impluvium.setTheme(bkg_music);
    Level::goToLevel(&cult_impluvium);

    PlayerCamera<DebugCamera> camera(.1, 5000, 90, 1.0,window,"player1cam");
    //Camera camera((Eigen::Matrix4f() << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1).finished(), -1);
    camera.activateKeyInput(window);
    Default3d default3d(camera);
    Dynamic3d dynamic3d(camera);
    Default2d default2d;
    HboxGraphics hbox_graphics(camera, .1, 100, 90);
    Font test_glyph("test_glyph.png");
    TextGraphics text_graphics(test_glyph);
   


    //room1.add(camera); //shouldnt be part of the layout
    default3d.add(cult_spiral_stairs);
    default3d.add(cult_landing);
    default3d.add(cult_hallway1);
    default3d.add(cult_stairs1);
    default3d.add(cult_impluvium);
    default3d.add(cult_ritual);
    default3d.add(path_to_town);

    //Button test_button(.5, .5, "test_button");
    DebugMenu debugMenu(window,default2d,text_graphics,center);
    debugMenu.activateKeyInput(window);
    //debugMenu.setDebugTarget(&center);

    Debugger dbg1(Eigen::Matrix4f::Identity(), "tester1", cult_spiral_stairs);
    cult_impluvium.add(dbg1);
    default3d.add(dbg1);

    dynamic3d.add(center);
    hbox_graphics.add(center);

    glfwSetWindowSize(window, 1600, 1200);

    camera.setPlayer(&center);

    //need to add objects to the shaders manually

    std::vector<uint8_t> screenshot_buffer(1200*1600*3);
    std::fill(screenshot_buffer.begin(), screenshot_buffer.end(),255);
    int screenshot_width, screenshot_height;
    int screenshot_buffers = 3;
    std::string screenshot_fname;

    glfwGetWindowSize(window, &screenshot_width, &screenshot_height);


    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        Level::UpdateCurrentLevel(window);
        camera.update(window);
        debugMenu.update(window);

        // Render here
        
        if (camera.getScreenshotFlag()) {
            default3d.startScreenshot(screenshot_width, screenshot_height);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

        default3d.drawAll();
        dynamic3d.drawAll();
        hbox_graphics.drawAll();
        default2d.drawAll();
        text_graphics.drawAll();

        if (camera.getScreenshotFlag()) {
            default3d.finishScreenshot<uint8_t, GL_UNSIGNED_BYTE>(&screenshot_buffer,"screenshot.png");
            camera.clearScreenshotFlag();
            //stbi_write_png("screenshot.png", screenshot_width, screenshot_height, screenshot_buffers, screenshot_buffer.data(), screenshot_width * screenshot_buffers);
        } 

        //hboxGraphics.drawAll();

        glfwSwapBuffers(window);

        // Poll for and process events
    }

    glfwTerminate();
    return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

