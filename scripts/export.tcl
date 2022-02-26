set name [lindex $argv 2]
set freq [lindex $argv 3]

open_project ${name}

set_top ${name}
open_solution "${name}_f${freq}" -flow_target vivado
export_design -format ip_catalog
close_solution
close_project
exit
