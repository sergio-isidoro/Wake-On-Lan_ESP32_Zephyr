# 1. Define the minimum CMake version required by Zephyr
cmake_minimum_required(VERSION 3.20.0)

# 2. Find the Zephyr package. 
# This pulls in all the networking, GPIO, and board-specific configurations.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# 3. Define your project name
project(esp32c3_wol_button)

# 4. Add your source files. 
# In this case, we only have main.c inside the src directory.
target_sources(app PRIVATE src/main.c)

# 5. (Optional) If you have extra header directories, you would add them here:
target_include_directories(app PRIVATE include)