#  DBexT - Database File Extractor

DBexT is a forensic utility for extracting embedded files (such as images, documents, archives) from binary database-like containers.



##  Features

*  **Extract specific file types**: Choose one or more formats like `JPG`, `PNG`, `PDF`, etc.
*  **Automatic signature-based scanning**: Uses magic numbers to locate embedded files.
*  **Organized output**: Creates a clean folder structure based on file type.
*  **Collision-safe folder names**: Prevents overwriting by auto-incrementing folders.
*  **Supports multi-selection** or "Extract All" option.
*  **Works locally**: No internet required — full offline functionality.
*  Includes a retro ASCII banner for tool branding.



##  Supported Formats

| Format | Description               | Extension |
| ------ | ------------------------- | --------- |
| JPG    | JPEG images               | `.jpg`    |
| PNG    | Portable Network Graphics | `.png`    |
| GIF    | Animated/static GIFs      | `.gif`    |
| BMP    | Bitmap image              | `.bmp`    |
| TIFF   | Tagged Image Format       | `.tiff`   |
| PDF    | Adobe PDF files           | `.pdf`    |
| ZIP    | Compressed archives       | `.zip`    |



##  Usage

1. **Place the `.db` or binary file** in the same directory as the script.
2. **Run the script**:

   ```bash
   python DBexT.py
   ```
3. **Follow the interactive menu**:

   * Choose the file formats you want to extract (e.g., JPG, PDF)
   * Enter the input filename when prompted
4. Extracted files will appear in a structured folder with subfolders per format.



##  Example Output

```
📁 Thumbs_1/
   ├── Thumbs_1_jpg/
   │   ├── extracted_0.jpg
   │   └── extracted_1.jpg
   └── Thumbs_1_pdf/
       └── extracted_0.pdf
```


