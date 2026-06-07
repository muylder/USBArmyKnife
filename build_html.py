import gzip
import os

def compress_to_c_array(srcFile, dstFile, varName, dataConvertFunc=None):
    try:
        with open(srcFile, 'rb') as f:
            data = f.read()

        if dataConvertFunc:
            data = dataConvertFunc(data)

        compressed_data = gzip.compress(data)
        c_array_string = ', '.join(str(byte) for byte in compressed_data)
        os.makedirs(os.path.dirname(dstFile), exist_ok=True)
        try:
            os.remove(dstFile)
        except:
            pass
        with open(dstFile, 'w') as f:
            f.write(f"#pragma once\n\n#ifndef NO_WEB\n\nconst uint8_t PROGMEM {varName}[{len(compressed_data)}] = {{ {c_array_string} }};\n\n#endif")

        print(f"Compressed data written to {dstFile}")
    except Exception as e:
        print("Error: ", e)

compress_to_c_array("ui/content/assets/css/styles.min.css", "src/html/stylesMinCss.h", "stylesMinCssGz")
compress_to_c_array("ui/content/assets/js/script.min.js", "src/html/scriptMinJs.h", "scriptMinJsGz")
compress_to_c_array("ui/content/index.html", "src/html/indexHtml.h", "indexHtmlGz")

print("Done generating headers")
