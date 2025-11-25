
#include "SDL3/SDL.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_sdlrenderer3.h"
#include "PageRegistry.hpp"

#include "StackAllocator.hpp"

#include <cstdio>
#include <iostream>

struct TestStruct {
    int a = 1;
    int b = 2;
    bool c = false;
    std::string d = "Foobar!";
};

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window;
    SDL_Renderer* renderer;
    if (!SDL_CreateWindowAndRenderer("Memory management assignment", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags, &window, &renderer))
    {
        printf("Error: SDL_CreateWindowAndRenderer(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    int foo = 3;
    int bar = 5;
    bool baz = true;
    std::string testStr = "Hiya!";

    StackAllocator stackAllocator;
    size_t fooPtr = stackAllocator.Push(&foo, sizeof(foo));
    size_t barPtr = stackAllocator.Push(&bar, sizeof(bar));
    size_t bazPtr = stackAllocator.Push(&baz, sizeof(baz));
    size_t strPtr = stackAllocator.Push(&testStr, sizeof(testStr));

    std::cout << "The value of foo is " << foo << ", the stack start position of it is " << fooPtr << std::endl;
    std::cout << "The value of bar is " << bar << ", the stack start position of it is " << barPtr << std::endl;
    std::cout << "The value of baz is " << baz << ", the stack start position of it is " << bazPtr << std::endl;
    std::cout << "The value of testStr is " << testStr << ", the stack start position of it is " << strPtr << std::endl;

    int* x = (int*)stackAllocator.At(fooPtr);
    int* y = (int*)stackAllocator.At(barPtr);
    bool* z = (bool*)stackAllocator.At(bazPtr);
    std::string* w = (std::string*)stackAllocator.At(strPtr);

    std::cout << "The value of foo after getting it back is " << *x << std::endl;
    std::cout << "The value of bar after getting it back is " << *y << std::endl;
    std::cout << "The value of baz after getting it back is " << *z << std::endl;
    std::cout << "The value of testStr after getting it back is " << *w << std::endl;

    int intA = 42;
    int intB = -234;
    std::string stringA = "Testing number one";
    bool boolA = false;
    unsigned int uintA = 234332;
    TestStruct structA;
    std::array<int, 5> arrayA = { 43, 44, 45, 46, 47 };
    std::string stringB = "well here I am now";

    size_t intAPtr = stackAllocator.Push(&intA, sizeof(intA));
    size_t intBPtr = stackAllocator.Push(&intB, sizeof(intB));
    size_t stringAPtr = stackAllocator.Push(&stringA, sizeof(stringA));
    size_t boolAPtr = stackAllocator.Push(&boolA, sizeof(boolA));
    size_t uintAPtr = stackAllocator.Push(&uintA, sizeof(uintA));
    size_t structAPtr = stackAllocator.Push(&structA, sizeof(structA));
    size_t arrayAPtr = stackAllocator.Push(&arrayA, sizeof(arrayA));
    size_t stringBPtr = stackAllocator.Push(&stringB, sizeof(stringB));

    std::cout << "The value of intA after getting it back is " << *(int*)stackAllocator.At(intAPtr) << std::endl;
    std::cout << "The value of intB after getting it back is " << *(int*)stackAllocator.At(intBPtr) << std::endl;
    std::cout << "The value of stringA after getting it back is " << *(std::string*)stackAllocator.At(stringAPtr) << std::endl;
    std::cout << "The value of boolA after getting it back is " << *(bool*)stackAllocator.At(boolAPtr) << std::endl;
    std::cout << "The value of uintA after getting it back is " << *(unsigned int*)stackAllocator.At(uintAPtr) << std::endl;
    TestStruct ts = *(TestStruct*)stackAllocator.At(structAPtr);
    std::cout << "The value of structA after getting it back is {" << ts.a << ", " << ts.b << ", " << ts.c << ", " << ts.d << "}" << std::endl;
    std::array<int, 5> arr = *(std::array<int, 5>*)stackAllocator.At(arrayAPtr);
    std::cout << "The value of arrayA after getting it back is {";
    for (size_t i = 0; i < arr.size(); i++)
    {
        std::cout << arr[i];
        if (i != arr.size() - 1)
            std::cout << ", ";
    }
    std::cout << "}" << std::endl;
    std::cout << "The value of stringB after getting it back is " << *(std::string*)stackAllocator.At(stringBPtr) << std::endl;

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }


        intA = 31;
        intB = -3214;
        stringA = "Testing number two";
        boolA = true;
        uintA = 9879834;
        structA = {};
        arrayA = { 43, 4214, 5125, 46, 47 };
        stringB = "well here I am now again";

        intAPtr = stackAllocator.Push(&intA, sizeof(intA));
        intBPtr = stackAllocator.Push(&intB, sizeof(intB));
        stringAPtr = stackAllocator.Push(&stringA, sizeof(stringA));
        boolAPtr = stackAllocator.Push(&boolA, sizeof(boolA));
        uintAPtr = stackAllocator.Push(&uintA, sizeof(uintA));
        structAPtr = stackAllocator.Push(&structA, sizeof(structA));
        arrayAPtr = stackAllocator.Push(&arrayA, sizeof(arrayA));
        stringBPtr = stackAllocator.Push(&stringB, sizeof(stringB));

        std::cout << "The value of intA after getting it back is " << *(int*)stackAllocator.At(intAPtr) << std::endl;
        std::cout << "The value of intB after getting it back is " << *(int*)stackAllocator.At(intBPtr) << std::endl;
        std::cout << "The value of stringA after getting it back is " << *(std::string*)stackAllocator.At(stringAPtr) << std::endl;
        std::cout << "The value of boolA after getting it back is " << *(bool*)stackAllocator.At(boolAPtr) << std::endl;
        std::cout << "The value of uintA after getting it back is " << *(unsigned int*)stackAllocator.At(uintAPtr) << std::endl;
        ts = *(TestStruct*)stackAllocator.At(structAPtr);
        std::cout << "The value of structA after getting it back is {" << ts.a << ", " << ts.b << ", " << ts.c << ", " << ts.d << "}" << std::endl;
        arr = *(std::array<int, 5>*)stackAllocator.At(arrayAPtr);
        std::cout << "The value of arrayA after getting it back is {";
        for (size_t i = 0; i < arr.size(); i++)
        {
            std::cout << arr[i];
            if (i != arr.size() - 1)
                std::cout << ", ";
        }
        std::cout << "}" << std::endl;
        std::cout << "The value of stringB after getting it back is " << *(std::string*)stackAllocator.At(stringBPtr) << std::endl;


        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        stackAllocator.Reset();
    }


    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}