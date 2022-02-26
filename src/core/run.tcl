# Create a project
open_project simple -reset
# Add design files
add_files simple.cpp
# Add test bench & files
add_files -tb simple_tb.cpp
# Set the top-level function
set_top simple_square
# Create a solution
open_solution "solution" -reset -flow_target vivado

set_part {xcu55n-fsvh2892-2l-e}
create_clock -period 250MHz -name default
config_compile -pragma_strict_mode
csim_design
csynth_design
#cosim_design
export_design
