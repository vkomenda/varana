set name [lindex $argv 2]
set freq [lindex $argv 3]
# set par_hashes [lindex $argv 4]

open_project ${name} -reset

add_files ../include/${name}.h
add_files ../src/${name}/${name}.cpp -cflags "-I../include/"
#add_files -tb ../src/${name}/${name}_tb.cpp -cflags "-I../include/"

set_top ${name}
open_solution "${name}_f${freq}" -flow_target vivado -reset
set_part {xcu55n-fsvh2892-2l-e}
create_clock -period ${freq}MHz -name default

set_directive_interface -mode ap_ctrl_none ${name}

config_compile -pragma_strict_mode
config_rtl -reset none

#csim_design
csynth_design
#cosim_design
# file copy -force ./${name}/${name}_f${freq}/syn/report/csynth.rpt ../reports/csynth_${name}_f${freq}.rpt
close_solution
close_project
exit
