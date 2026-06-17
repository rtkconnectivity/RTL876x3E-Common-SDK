#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess
import time
import json
import signal
import re
import glob
import argparse

# 将脚本拖到HoneyComb\sdk\bin目录下运行
#查看生成的文件rom_symbol_gcc.axf，删掉第一行“#<SYMDEFS># ARM Linker, 5060750: Last Updated: Thu Jan 16 15:16:11 2020”
file = 'upperstack_4M.lib'

distfile = 'upperstack_4M_gcc.axf'


with open(file, mode='r', newline='', errors='surrogateescape') as fd:
    stream = fd.read()
    stream = re.sub(r'^(0x\S\S\S\S\S\S\S\S)(\s[T,N,D]\s)(\S+)', lambda objs: objs.group(3) + " = " + objs.group(1)+" ;", stream, flags=re.M)
    with open(distfile, mode='w+', newline='', errors='surrogateescape') as fd:
        fd.write(stream)

