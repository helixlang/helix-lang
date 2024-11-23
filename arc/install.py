import os
import sys
import platform
try:
    import requests
    from tqdm import tqdm
except ImportError:
    os.system(sys.executable + " -m pip install requests tqdm")
    import requests
    from tqdm import tqdm
import tarfile
import shutil
import zipfile

def download_file(url, local_filename):
    response = requests.get(url, stream=True)
    response.raise_for_status()
    total_size = int(response.headers.get('content-length', 0))
    block_size = 1024*100 # 100 kb

    with open(local_filename, 'wb') as file, tqdm(
        desc=local_filename,
        total=total_size,
        unit='iB',
        unit_scale=True,
        unit_divisor=1024,
    ) as bar:
        for chunk in response.iter_content(chunk_size=block_size):
            file.write(chunk)
            bar.update(len(chunk))
    return local_filename

def extract_file(file_path, extract_to):
    if file_path.endswith('.zip'):
        with zipfile.ZipFile(file_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
    elif file_path.endswith('.tar.xz'):
        with tarfile.open(file_path, 'r:xz') as tar_ref:
            tar_ref.extractall(extract_to)
    else:
        raise ValueError("Unsupported file format")
    
def move_files(src_dir, dest_dir):
    for item in os.listdir(src_dir):
        s = os.path.join(src_dir, item)
        d = os.path.join(dest_dir, item)
        if os.path.isdir(s):
            shutil.move(s, d)
        else:
            shutil.move(s, d)

def main():
    base_url = "https://github.com/mstorsjo/llvm-mingw/releases/download/20240619/"
    files = [
        "llvm-mingw-20240619-msvcrt-i686.zip",
        "llvm-mingw-20240619-msvcrt-ubuntu-20.04-x86_64.tar.xz",
        "llvm-mingw-20240619-msvcrt-x86_64.zip",
        "llvm-mingw-20240619-ucrt-aarch64.zip",
        "llvm-mingw-20240619-ucrt-armv7.zip",
        "llvm-mingw-20240619-ucrt-i686.zip",
        "llvm-mingw-20240619-ucrt-macos-universal.tar.xz",
        "llvm-mingw-20240619-ucrt-ubuntu-20.04-aarch64.tar.xz",
        "llvm-mingw-20240619-ucrt-ubuntu-20.04-x86_64.tar.xz",
        "llvm-mingw-20240619-ucrt-x86_64.zip"
    ]

    # Select the appropriate file based on the platform
    system = platform.system()
    arch = platform.machine()

    if system == 'Windows':
        file_to_download = "llvm-mingw-20240619-ucrt-x86_64.zip" if arch == 'AMD64' else "llvm-mingw-20240619-ucrt-i686.zip"
    elif system == 'Darwin':
        file_to_download = "llvm-mingw-20240619-ucrt-macos-universal.tar.xz"
    elif system == 'Linux':
        if 'aarch64' in arch:
            file_to_download = "llvm-mingw-20240619-ucrt-ubuntu-20.04-aarch64.tar.xz"
        else:
            file_to_download = "llvm-mingw-20240619-ucrt-ubuntu-20.04-x86_64.tar.xz"
    else:
        raise ValueError("Unsupported operating system")

    # Construct download URL
    download_url = base_url + file_to_download

    # Define local file path
    local_file = os.path.join(os.getcwd(), file_to_download)

    # Download the file
    download_file(download_url, local_file) if not os.path.exists(local_file) and os.stat(local_file).st_size != int(requests.get(download_url, stream=True).headers.get('content-length', 0)) else None

    # Extract the file
    extract_dir = os.path.abspath(os.path.join(sys.path[0], "..", "core"))
    os.makedirs(extract_dir, exist_ok=True)
    print(f"Extracting {file_to_download} to {extract_dir}")
    extract_file(local_file, extract_dir)
    move_files(os.path.join(extract_dir, local_file.split('.')[0]), extract_dir)

    print("Download and extraction complete.")

if __name__ == "__main__":
    main()
