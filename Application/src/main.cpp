#include "TracyWrapper.hpp"

#include "SDL3/SDL.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_sdlrenderer3.h"

#include "PoolAllocator.hpp"
#include "StackAllocator.hpp"
#include "BuddyAllocator.hpp"
#include "MemPerfTests.hpp"

#include <cstdio>
#include <iostream>
#include <functional>

struct TestStruct {
    int a = 1;
    int b = 2;
    bool c = false;
    std::string d = "Foobar!";
};

int main()
{
#ifdef TRACY_ENABLE
    SDL_Delay(500);
#endif

    ZoneScopedC(tracy::Color::Coral3);

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

    BuddyAllocator buddyAllocator;
    void* foo = buddyAllocator.Alloc(75 * 1000);
    void* bar = buddyAllocator.Alloc(36 * 1000);
    void* baz = buddyAllocator.Alloc(36 * 1000);
    void *p1 = buddyAllocator.Alloc(128 * 1000);
    void *p2 = buddyAllocator.Alloc(200 * 1000);
    void *p3 = buddyAllocator.Alloc(30 * 1000);
    void *p4 = buddyAllocator.Alloc(100 * 1000);
    void *p5 = buddyAllocator.Alloc(128 * 1000);

    std::cout << "The first alloc was placed at " << foo << "\n";
    std::cout << "The second alloc was placed at " << bar << "\n";
    buddyAllocator.PrintAllocatedIndices();
    buddyAllocator.Free(bar);
    buddyAllocator.Free(baz);
    buddyAllocator.PrintAllocatedIndices();

    void *p6 = buddyAllocator.Alloc(128 * 10000);
    buddyAllocator.PrintAllocatedIndices();

    void *p7 = buddyAllocator.Alloc(36 * 1000);
    buddyAllocator.PrintAllocatedIndices();

    void *p8 = buddyAllocator.Alloc(500 * 1000);
    buddyAllocator.PrintAllocatedIndices();

	buddyAllocator.Free(foo);
	buddyAllocator.Free(p1);
	buddyAllocator.Free(p2);
	buddyAllocator.Free(p3);
	buddyAllocator.Free(p4);
	buddyAllocator.Free(p5);
	buddyAllocator.Free(p6);
	buddyAllocator.Free(p7);
	buddyAllocator.Free(p8);


    StackAllocator stackAllocator;

	void* dataPtr1 = stackAllocator.Alloc(1);
	void* dataPtr2 = stackAllocator.Alloc(2);
	void* dataPtr3 = stackAllocator.Alloc(3);

	std::cout << "Stack Allocator allocated 1 bytes at address: " << dataPtr1 << "\n";
	std::cout << "Stack Allocator allocated 2 bytes at address: " << dataPtr2 << "\n";
	std::cout << "Stack Allocator allocated 3 bytes at address: " << dataPtr3 << "\n";
    
    FrameMark;
    
    // Main loop
    bool done = false;
    while (!done)
    {
        ZoneNamedNC(mainLoopZone, "Main Loop", tracy::Color::Honeydew3, true);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ZoneNamedNC(sdlPollZone, "SDL Poll Event", tracy::Color::VioletRed, true);

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

		ImGuiID mainDockID = ImGui::DockSpaceOverViewport();

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            ZoneNamedNC(imguiWindowZone, "ImGui Window", tracy::Color::Firebrick4, true);

            static float f = 0.0f;
            static int counter = 0;

			// Dock to the main dockspace
			ImGui::SetNextWindowDockID(mainDockID, ImGuiCond_Always);

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

            if (ImGui::Button("Run Pool Performance Tests"))
            {
                PerfTests::RunPoolPerfTests();
			}

			if (ImGui::TreeNode("Buddy Allocator Visualizer"))
			{
                std::array<char, 4096 * 1024> *memory = buddyAllocator.DBG_GetMemory();
                std::vector<BuddyAllocator::Block> *blocks = buddyAllocator.DBG_GetBlocks();

				static std::vector<BuddyAllocator::Block *> allocations;

                static bool initAllocs = true;
                if (initAllocs)
                {
                    for (size_t i = 0; i < blocks->size(); i++)
                    {
                        if (!blocks->at(i).isFree)
                        {
                            allocations.push_back(&blocks->at(i));
						}
                    }

                    initAllocs = false;
                }

                // Draw blocks
                {
                    static float width = 600.0f;
                    static float height = 40.0f;
					ImGui::SliderFloat("##BlockWidth", &width, 100.0f, 1200.0f, "%.1f");
					ImGui::SliderFloat("##BlockHeight", &height, 10.0f, 100.0f, "%.1f");

                    size_t totalMemory = memory->size();

                    BuddyAllocator::Block *rootBlock = &blocks->at(0);

					std::function<void(BuddyAllocator::Block *)> traverseBlocks;

                    traverseBlocks = [&](BuddyAllocator::Block *currentBlock)
                    {
						bool drawBlock = true;

                        if (currentBlock->left)
                        {
                            traverseBlocks(currentBlock->left);
							drawBlock = false;
                        }

                        if (currentBlock->right)
                        {
                            traverseBlocks(currentBlock->right);
							drawBlock = false;
                        }

                        bool isAllocated = !currentBlock->isFree;

                        if (drawBlock)
                        {
							// Draw block with ImGui
							ImDrawList *drawList = ImGui::GetWindowDrawList();
                            ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

							float blockOffset = width * (static_cast<float>(currentBlock->offset) / static_cast<float>(totalMemory));
                            float blockWidth = width * (static_cast<float>(currentBlock->size) / static_cast<float>(totalMemory));

                            ImVec2 blockPos = ImVec2(cursorScreenPos.x + blockOffset, cursorScreenPos.y);
                            ImU32 blockColor = isAllocated ? IM_COL32(200, 50, 50, 255) : IM_COL32(50, 200, 50, 100);

                            drawList->AddRectFilled(blockPos, ImVec2(blockPos.x + blockWidth, blockPos.y + height), blockColor);
                            drawList->AddRect(blockPos, ImVec2(blockPos.x + blockWidth, blockPos.y + height), IM_COL32(0, 0, 0, 255));
						}
                    };

                    traverseBlocks(rootBlock);

					ImGui::Dummy(ImVec2(width, height));
                }

                if (ImGui::Button("Reset Allocator"))
                {
					buddyAllocator = BuddyAllocator();
					allocations.clear();
				}
                else
                {
                    static int allocSize = 64 * 1024;
                    ImGui::DragInt("##AllocationSize", &allocSize, 64.0f, 1, 4096 * 1024);
                    ImGui::SameLine();
                    if (ImGui::Button("Make Allocation"))
                    {
                        void *ptr = buddyAllocator.Alloc(static_cast<size_t>(allocSize));
                        if (ptr != nullptr)
                        {
                            BuddyAllocator::Block *block = buddyAllocator.DBG_GetAllocationBlock(ptr);
                            allocations.push_back(block);
                        }
                    }

                    if (ImGui::TreeNode("Current Allocations"))
                    {
                        for (size_t i = 0; i < allocations.size(); ++i)
                        {
                            BuddyAllocator::Block *block = allocations[i];
                            void *ptr = memory->data() + block->offset;
                            if (!block)
                                continue;

                            ImGui::Text("Allocation %d: %p (%d, %d)", static_cast<int>(i), ptr, block->offset, block->size);
                            ImGui::SameLine();
                            if (ImGui::Button(std::format("Free##{}", i).c_str()))
                            {
                                buddyAllocator.Free(ptr);
                                allocations.erase(allocations.begin() + i);
                                --i;
                            }
                        }

                        ImGui::TreePop();
                    }

                    // Display blocks as a binary tree
                    std::function<void(BuddyAllocator::Block *, int, int)> displayBlock = [&](BuddyAllocator::Block *block, int index, int depth)
                    {
                        if (block == nullptr)
                            return;

                        ImGui::Text("Size: %d", block->size);
                        ImGui::Text("offset: %d", block->offset);

                        if (block->left == nullptr && block->right == nullptr)
                        {
                            // Display block data

                            if (block->isFree)
                            {
                                ImGui::Text("Block is free");
                            }
                            else if (ImGui::TreeNode("Data"))
                            {
                                char *buf = (char *)(memory->data() + block->offset);
                                ImGui::InputTextMultiline("##data", buf, block->size);

                                ImGui::TreePop();
                            }
                        }

                        int child1Index = (2 * index) + 1;
                        int child2Index = 2 * (index + 1);

                        if (block->left != nullptr)
                            if (ImGui::TreeNode(std::format("Block {}", child1Index).c_str()))
                            {
                                displayBlock(block->left, child1Index, depth + 1);
                                ImGui::TreePop();
                            }

                        if (block->right != nullptr)
                            if (ImGui::TreeNode(std::format("Block {}", child2Index).c_str()))
                            {
                                displayBlock(block->right, child2Index, depth + 1);
                                ImGui::TreePop();
                            }
                    };

                    BuddyAllocator::Block *root = &blocks->at(0);

                    if (ImGui::TreeNode(std::format("Block {}", 0).c_str()))
                    {
                        displayBlock(root, 0, 0);
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
			}

            if (ImGui::TreeNode("Pool Allocator Visualizer"))
            {
                using namespace MemoryInternal;

				PoolAllocator<char>::Initialize(4096);

				auto &pageStorage = PoolAllocator<char>::DBG_GetPageStorage();
				auto &allocMap = PoolAllocator<char>::DBG_GetAllocMap();
                auto &freeRegions = PoolAllocator<char>::DBG_GetFreeRegions();
                auto freeRegionRoot = PoolAllocator<char>::DBG_GetFreeRegionRoot();

				static std::vector<char *> allocations;

				// Draw blocks
                {
                    static float width = 600.0f;
					static float height = 40.0f;
                    ImGui::SliderFloat("##BlockWidth", &width, 100.0f, 1200.0f, "%.1f");
                    ImGui::SliderFloat("##BlockHeight", &height, 10.0f, 100.0f, "%.1f");

					size_t totalMemory = pageStorage.size();

                    // Draw allocated blocks
                    ImDrawList *drawList = ImGui::GetWindowDrawList();
                    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

					// Draw background block for total memory
					ImVec2 bgPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y);
					ImU32 bgColor = IM_COL32(200, 50, 50, 128);
					drawList->AddRectFilled(bgPos, ImVec2(bgPos.x + width, bgPos.y + height), bgColor);
					drawList->AddRect(bgPos, ImVec2(bgPos.x + width, bgPos.y + height), bgColor);

                    IndexType freeRegionIndex = freeRegionRoot;

					// Draw free regions over the background
                    while (freeRegionIndex != NULL_INDEX)
                    {
						auto &freeRegion = freeRegions[freeRegionIndex];

                        if (freeRegion.size == 0)
                            break;

						float regionOffset = width * (static_cast<float>(freeRegion.offset) / static_cast<float>(totalMemory));
						float regionWidth = width * (static_cast<float>(freeRegion.size) / static_cast<float>(totalMemory));

                        ImVec2 regionPos = ImVec2(cursorScreenPos.x + regionOffset, cursorScreenPos.y);
                        ImU32 regionColor = IM_COL32(50, 200, 50, 128);

                        drawList->AddRectFilled(regionPos, ImVec2(regionPos.x + regionWidth, regionPos.y + height), regionColor);
                        drawList->AddRect(regionPos, ImVec2(regionPos.x + regionWidth, regionPos.y + height), regionColor);
						freeRegionIndex = freeRegion.next;
                    }

					ImGui::Dummy(ImVec2(width, height));
                }

                if (ImGui::Button("Reset Allocator"))
				{
					PoolAllocator<char>::Reset();
					allocations.clear();
				}
                else
                {
                    static int allocSize = 256;
                    ImGui::DragInt("##AllocationSize", &allocSize, 1.0f, 1, (int)pageStorage.size());
                    ImGui::SameLine();
                    if (ImGui::Button("Make Allocation"))
                    {
                        char *ptr = PoolAllocator<char>::Alloc(static_cast<IndexType>(allocSize));
                        if (ptr != nullptr)
                            allocations.push_back(ptr);
                    }

                    if (ImGui::TreeNode("Current Allocations"))
                    {
                        for (size_t i = 0; i < allocations.size(); ++i)
                        {
							ImGui::PushID(static_cast<int>(i));

							char *ptr = allocations[i];

							IndexType offset = static_cast<IndexType>(ptr - pageStorage.data());
                            IndexType blockSize = allocMap[offset];

                            ImGui::Text("Allocation %d: %p (%d, %d)", static_cast<int>(i), ptr, offset, blockSize);
                            ImGui::SameLine();
                            if (ImGui::Button(std::format("Free##{}", i).c_str()))
                            {
								PoolAllocator<char>::Free(ptr);
								allocations.erase(allocations.begin() + i);
                                --i;
                            }
                                
                            ImGui::InputTextMultiline("##data", ptr, blockSize);

                            ImGui::Separator();

							ImGui::PopID();
                        }

                        ImGui::TreePop();
                    }

                }

                ImGui::TreePop();
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }


        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        stackAllocator.Reset();

        FrameMark;
    }


    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}