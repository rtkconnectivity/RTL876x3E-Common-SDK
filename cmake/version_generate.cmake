#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

function(generate_version target_name generate_path)
message("### generate ${target_name} version:")
find_program(Bash_EXECUTABLE bash NO_CMAKE_FIND_ROOTO_PATH)
set(Version_Script_Path "${REALTEK_SDK_BASE_PATH1}/tool/Gadgets/version_gen_config.sh")

message("### ${Bash_EXECUTABLE} ${Version_Script_Path} ${generate_path}")
execute_process(
  COMMAND ${Bash_EXECUTABLE} ${Version_Script_Path} ${generate_path}
)
endfunction(generate_version)