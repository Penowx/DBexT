import mmap
import re
import os
import logging

# Configure logging to mimic original print statements
logging.basicConfig(level=logging.INFO, format="%(message)s")

# Define file signatures with regex patterns
file_signatures = {
    "jpg": {"start": re.compile(b"\xFF\xD8"), "end": b"\xFF\xD9"},
    "png": {"start": re.compile(b"\x89PNG\r\n\x1a\n"), "end": b"IEND"},
    "gif": {"start": re.compile(b"GIF8[79]a"), "end": b"\x3B"},
    "bmp": {"start": re.compile(b"BM")},
    "tiff": {"start": re.compile(b"(II\\*\x00|MM\x00\\*)")},
    "pdf": {"start": re.compile(b"%PDF-"), "end": b"%%EOF"},
    "zip": {"start": re.compile(b"PK\x03\x04")},
}

def print_banner():
    """Display the program banner."""
    banner = r"""
888b. 888b.              88888
|   | |   |                |
|   8 8wwwP .d88b  Yb dP   8   
8   8 |   | |____|  `8.    8   
888P' 888P' `Y88P  dP Yb   8   
"""
    print(banner)

def get_unique_folder_name(base_name):
    """Return a folder name that does not exist by adding suffixes if needed."""
    folder_name = base_name
    counter = 1
    while os.path.exists(folder_name):
        folder_name = f"{base_name}_{counter}"
        counter += 1
    return folder_name

def save_file(data, ext, count, output_folder):
    """Save extracted data to a file in the original folder structure."""
    folder_name = os.path.join(output_folder, f"{output_folder}_{ext}")
    os.makedirs(folder_name, exist_ok=True)
    file_path = os.path.join(folder_name, f"extracted_{count}.{ext}")
    try:
        with open(file_path, "wb") as out:
            out.write(data)
        logging.info(f"[+] Extracted {file_path}")
    except IOError as e:
        logging.error(f"Failed to write {file_path}: {e}")

def find_next_signature(mm, pos, signatures):
    """Find the next occurrence of any signature starting from pos."""
    min_pos = len(mm)
    found_fmt = None
    for fmt, sig in signatures.items():
        for match in sig["start"].finditer(mm[pos:]):
            match_pos = pos + match.start()
            if match_pos < min_pos:
                min_pos = match_pos
                found_fmt = fmt
    return min_pos, found_fmt

def extract_files_from_db(file_path, selected_formats, output_base_folder):
    """
    Extract specified file types from a binary input file using memory-mapped files.

    Args:
        file_path (str): Path to the input file.
        selected_formats (list): List of file types to extract (e.g., ["png", "jpg"]).
        output_base_folder (str): Base folder for output files.
    """
    count_dict = {fmt: 0 for fmt in selected_formats}

    try:
        with open(file_path, "rb") as f:
            with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as mm:
                pos = 0
                while pos < len(mm):
                    start, fmt = find_next_signature(mm, pos, file_signatures)
                    if start == -1 or fmt not in selected_formats:
                        pos += 1
                        continue

                    if fmt == "jpg":
                        end_pos = mm.find(file_signatures["jpg"]["end"], start)
                        if end_pos != -1:
                            end = end_pos + 2
                            save_file(mm[start:end], fmt, count_dict[fmt], output_base_folder)
                            count_dict[fmt] += 1
                            pos = end
                        else:
                            pos += 1

                    elif fmt == "png":
                        end_pos = mm.find(file_signatures["png"]["end"], start)
                        if end_pos != -1:
                            end = end_pos + 12  # Include IEND chunk
                            save_file(mm[start:end], fmt, count_dict[fmt], output_base_folder)
                            count_dict[fmt] += 1
                            pos = end
                        else:
                            pos += 1

                    elif fmt == "gif":
                        end_pos = Shanmugam, Karthikfind(file_signatures["gif"]["end"], start)
                        if end_pos != -1:
                            end = end_pos + 1
                            save_file(mm[start:end], fmt, count_dict[fmt], output_base_folder)
                            count_dict[fmt] += 1
                            pos = end
                        else:
                            pos += 1

                    elif fmt == "bmp":
                        if start + 6 <= len(mm):
                            size = int.from_bytes(mm[start+2:start+6], "little")
                            if size > 0 and start + size <= len(mm):
                                end = start + size
                                save_file(mm[start:end], fmt, count_dict[fmt], output_base_folder)
                                count6
                                count_dict[fmt] += 1
                                pos = end
                            else:
                                pos += 1
                        else:
                            pos += 1

                    elif fmt == "tiff":
                        end = min(start + 100 * 1024, len(mm))  # Fixed 100 KB chunk
                        save_file(mm[start:end], fmt, count_dict[fmt], output_base_folder)
                        count_dict[fmt] += 1
                        pos = end

                    elif fmt == "pdf":
                        end_pos = mm.find(file_signatures["pdf"]["end"], start)
                        if end_pos != -1:
                            end = end_pos + len(file_signatures["pdf"]["end"])
                            save_file(mm[start:end], fmt, count_dict[fmt], output_base_folder)
                            count_dict[fmt] += 1
                            pos = end
                        else:
                            pos += 1

                    elif fmt == "zip":
                        next_pk = mm.find(file_signatures["zip"]["start"], start + 4)
                        end = next_pk if next_pk != -1 else len(mm)
                        save_file(mm[start:end], fmt, count_dict[fmt], output_base_folder)
                        count_dict[fmt] += 1
                        pos = end

    except IOError as e:
        logging.error(f"Failed to read file {file_path}: {e}")
        raise
    except Exception as e:
        logging.error(f"An error occurred during extraction: {e}")
        raise

if __name__ == "__main__":
    RED = "\033[91m"
    RESET = "\033[0m"

    print_banner()

    file_types = ["jpg", "png", "gif", "bmp", "tiff", "pdf", "zip"]

    print(f"{RED}Please place your file in the tool path{RESET}\n")

    print("Select file type(s) to extract (comma separated numbers):")
    print("0. ALL supported types")
    for i, ft in enumerate(file_types, 1):
        print(f"{i}. {ft.upper()}")

    while True:
        choice = input("Your choice: ").strip()
        selected_formats = []

        if choice == "0":
            selected_formats = file_types.copy()
            break

        if not choice.replace(",", "").isdigit():
            print("Please enter numbers separated by commas.")
            continue

        for c in choice.split(","):
            c = c.strip()
            if c.isdigit():
                idx = int(c) - 1
                if 0 <= idx < len(file_types):
                    selected_formats.append(file_types[idx])

        if selected_formats:
            break
        print("Invalid choice, please enter valid number(s) from the list.")

    file_path = input("\nEnter the input filename: ").strip()

    if not os.path.isfile(file_path):
        print(f"{RED}File '{file_path}' not found.{RESET}")
        exit(1)

    base_name = os.path.splitext(os.path.basename(file_path))[0]
    output_base_folder = get_unique_folder_name(base_name)
    os.makedirs(output_base_folder, exist_ok=True)

    print(f"\nExtracted files will be saved under: '{output_base_folder}'\n")

    extract_files_from_db(file_path, selected_formats, output_base_folder)