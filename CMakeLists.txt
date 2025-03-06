cmake_minimum_required(VERSION 3.5.0)
project(UHD_test 
    VERSION 0.1.0 
    LANGUAGES C CXX
)

include_directories(
    ./include
    /usr/include
    /usr/include/gtk-3.0 
    /usr/include/at-spi2-atk/2.0 
    /usr/include/at-spi-2.0
    /usr/include/dbus-1.0 
    /usr/lib/aarch64-linux-gnu/dbus-1.0/include 
    /usr/include/gtk-3.0 
    /usr/include/gio-unix-2.0/ 
    /usr/include/cairo 
    /usr/include/pango-1.0 
    /usr/include/harfbuzz 
    /usr/include/pango-1.0 
    /usr/include/atk-1.0 
    /usr/include/cairo 
    /usr/include/pixman-1 
    /usr/include/freetype2 
    /usr/include/libpng16 
    /usr/include/freetype2 
    /usr/include/libpng16 
    /usr/include/gdk-pixbuf-2.0 
    /usr/include/libpng16 
    /usr/include/gstreamer-1.0 
    /usr/include/glib-2.0 
    /usr/lib/aarch64-linux-gnu/glib-2.0/include  
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/include
   
)

link_directories(
    /usr/lib
    /usr/lib/aarch64-linux-gnu
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0
    /usr/lib/aarch64-linux-gnu/glib-2.0
    
)

add_executable(${PROJECT_NAME} 
    #./src/rx_sample_c.c
    #./src/tx_sample_c.c
    #./src/tx_samples_test.c
    #./src/rx_samples_test.c
    ./src/read_sample_from_file.c
)

target_compile_options(${PROJECT_NAME} 
    PUBLIC -pthread
)

target_link_libraries(${PROJECT_NAME} 
    gtk-3 
    gdk-3 
    pangocairo-1.0 
    pango-1.0 
    atk-1.0 
    cairo-gobject 
    cairo 
    gdk_pixbuf-2.0 
    gio-2.0 
    gstreamer-1.0 
    gobject-2.0 
    glib-2.0
    gstrtspserver-1.0
    gstvideo-1.0
    gstaudio-1.0
    gstapp-1.0
    pthread
    uhd
    m
)
