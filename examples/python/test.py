# import sys
# sys.path.append(r"D:\mateusz\studia\5_semestr\Programowanie_genetyczne\GAsm\build\Release")
# # sys.path.append(r"C:/Users/mateu/AppData/Local/Programs/Python/Python311/python311.dll")
#
# import os, pprint
# # pprint.pprint(os.environ["PATH"].split(";"))
#
# import platform
# # print(platform.architecture())
#
# # import pefile
# # pe = pefile.PE("build/Release/gasm_python.pyd")
# # print(hex(pe.OPTIONAL_HEADER.MajorLinkerVersion), hex(pe.OPTIONAL_HEADER.MinorLinkerVersion))
#
# import os
# path = r"D:\mateusz\studia\5_semestr\Programowanie_genetyczne\GAsm\build\Release\gasm_python.pyd"
# print("Exists:", os.path.exists(path))
#
# # pe = pefile.PE(path)
# # print("Machine:", hex(pe.FILE_HEADER.Machine))
# #
# # print("Python executable:", sys.executable)
# # print("Python arch:", 8 * __import__("struct").calcsize("P"))
#
# import sys, os
# import ctypes
#
# dllpath = os.path.join(os.path.dirname(sys.executable), "python311.dll")
# print(dllpath, os.path.exists(dllpath))
#
# import os
# import ctypes
#
# print(sys.path)
#
# # dll_path = os.path.join(os.getcwd(), "gasm_python.pyd")
# # print(ctypes.WinDLL(dll_path))

import gasm

g = gasm.GAsm()

inputs = [[1], [2]]
outputs = [[1], [2]]

g.evolve(inputs, outputs)

# cmake -G "Visual Studio 17 2022" -A x64 -DPython3_EXECUTABLE="C:/Users/mateu/AppData/Local/Programs/Python/Python311/python.exe" -DCMAKE_BUILD_TYPE=Release ..
# dumpbin /DEPENDENTS "D:\mateusz\studia\5_semestr\Programowanie_genetyczne\GAsm\build\Release\gasm_python.pyd"