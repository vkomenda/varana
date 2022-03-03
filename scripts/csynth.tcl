set name [lindex $argv 2]
set freq [lindex $argv 3]
# set par_hashes [lindex $argv 4]

open_project ${name} -reset

add_files ../include/${name}.h
add_files ../src/${name}/${name}.cpp -cflags "-I../include/"

set_top ${name}
open_solution "${name}_f${freq}" -flow_target vivado -reset
set_part {xcu55n-fsvh2892-2l-e}
create_clock -period ${freq}MHz -name default

set_directive_interface -mode ap_ctrl_none ${name}
# set_directive_interface -mode ap_none ${name} msg
# set_directive_aggregate -compact bit ${name} msg
# set_directive_aggregate -compact bit ${name} tmp
# for {set i 0} {${i} < ${par_hashes}} {incr i} {
#     set_directive_stream -type fifo -depth 8 pkts_dataflow "in_pkt_ctrls_par[${i}]"
# }

config_compile -pragma_strict_mode
config_rtl -reset none
csynth_design
# file copy -force ./${name}/${name}_f${freq}/syn/report/csynth.rpt ../reports/csynth_${name}_f${freq}.rpt
close_solution
close_project
exit
