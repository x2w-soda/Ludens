import sys
import os

def generate_embedded_files(target_file: str, base_name: str, output_dir: str):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    cpp_path = os.path.join(output_dir, f"{base_name}.cpp")
    h_path = os.path.join(output_dir, f"{base_name}.h")

    try:
        with open(target_file, "rb") as f:
            data = f.read()
    except Exception as e:
        print(f"error reading target file: {e}")
        sys.exit(1)

    # header
    with open(h_path, "w") as h_file:
        h_file.write("#pragma once\n")
        h_file.write("#include <cstddef>\n\n")
        h_file.write('extern "C" {\n')
        h_file.write(f"extern const unsigned char {base_name}Data[];\n")
        h_file.write(f"extern const size_t {base_name}Size;\n")
        h_file.write('}\n')

    # cpp
    with open(cpp_path, "w") as cpp_file:
        cpp_file.write(f'#include "{os.path.basename(h_path)}"\n\n')
        cpp_file.write('extern "C" {\n')
        cpp_file.write(f"const unsigned char {base_name}Data[] = {{\n")
        
        bytes_per_line = 1024
        for i, byte in enumerate(data):
            cpp_file.write(f"0x{byte:02x},")
            if (i + 1) % bytes_per_line == 0:
                cpp_file.write("\n")
        
        cpp_file.write(f"\n}};\n")
        cpp_file.write(f"const size_t {base_name}Size = sizeof({base_name}Data);\n")
        cpp_file.write('}\n')

    print(f"generated: {cpp_path}")
    print(f"generated: {h_path}")

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print("usage: python Embed.py <TargetFile> <BaseName> <OutputDirectory>")
        sys.exit(1)

    target_file = sys.argv[1]
    base_name = sys.argv[2]
    output_dir = sys.argv[3]
    generate_embedded_files(target_file, base_name, output_dir)
