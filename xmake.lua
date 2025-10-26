add_rules("mode.debug", "mode.release")


-- shaders
includes("shaders/")

-- Add required packages (glfwpp needs glfw)
add_requires("vulkan-hpp", "vulkan-loader", "glm", "glfw")

set_defaultmode("debug")

target("galaxy")
    set_kind("binary")
    set_languages("c++23") -- glfwpp requires C++17

    add_deps("shaders")
    
    add_files("src/**.cpp")
    add_packages("vulkan-hpp", "vulkan-loader", "glm", "glfw")
    
    add_includedirs("include/")
    add_includedirs("third_party/glfwpp/include") -- Add glfwpp header path
    
    add_links("xml2", "z", "icuuc", "icudata")

    on_load(function (target)
        local slang = target:pkg("slang")
        if slang then
            local libdir = path.join(slang:installdir(), "lib")
            target:add("rpathdirs", libdir)
            target:add("linkdirs", libdir)
        end
    end)