cmake_minimum_required(VERSION 3.15)
project(stm_lcd_touch_gt9271 C)
add_library(stm_lcd_touch_gt9271 src/stm_lcd_touch_gt9271.c)
target_include_directories(stm_lcd_touch_gt9271 PUBLIC include)
