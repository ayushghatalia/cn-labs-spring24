import os
import sys
import zipfile

def create_zip(zip_filename, zip_filepath):
    impl_folder = "../impl"

    if not os.path.exists(impl_folder):
        print(f"Error: Implementation folder '{impl_folder}' not found.")
        exit(1)

    server_file = "server.c"

    if not os.path.isfile(os.path.join(impl_folder, server_file)):
        print(f"Error: '{server_file}' not found in '{impl_folder}'.")
        exit(1)

    with zipfile.ZipFile(zip_filepath, 'w') as zip_file:
        zip_file.write(os.path.join(impl_folder, server_file), os.path.join("impl", server_file))

    print(f"Zip file '{zip_filename}' created successfully.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <zip_filename>")
        exit(1)

    zip_folder = ".."
    zip_filename = sys.argv[1]
    zip_filepath = os.path.join(zip_folder, zip_filename)
    create_zip(zip_filename, zip_filepath)