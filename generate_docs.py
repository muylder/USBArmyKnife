import os
import re

def extract_docs_from_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        return f"Error reading {filepath}: {e}\n"

    # Very naive extraction of comments followed by function/class signatures
    # Look for block comments /** ... */ or /* ... */ and the next line
    results = []
    
    # regex to match doxygen style or normal block comments followed by some code
    pattern = re.compile(r'(/\*\*.*?\*/|//.*?(?:\n//.*?)*)\s*\n\s*([^/{;]+(?:;|\{|\n))', re.DOTALL | re.MULTILINE)
    
    matches = pattern.findall(content)
    if not matches:
        return ""
        
    output = f"## File: `{os.path.basename(filepath)}`\n\n"
    for comment, code in matches:
        # clean up code signature
        code_sig = code.strip().split('{')[0].split(';')[0].strip()
        if not code_sig: continue
        if code_sig.startswith('#'): continue # skip preprocessor directives
        
        # clean up comment
        comment_clean = '\n'.join([line.strip('/* \t') for line in comment.split('\n')])
        
        output += f"### `{code_sig}`\n"
        output += f"{comment_clean}\n\n"
        
    return output

def main():
    src_dirs = ['src', 'lib', 'include']
    files_to_parse = []
    for d in src_dirs:
        for root, _, files in os.walk(d):
            for f in files:
                if f.endswith('.cpp') or f.endswith('.h') or f.endswith('.c') or f.endswith('.hpp'):
                    files_to_parse.append(os.path.join(root, f))
    
    out_path = r"C:\Users\andre\.gemini\antigravity\brain\35ab6ff5-ebbd-4997-a7f7-2c4bdc192d03\documentation.md"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    
    with open(out_path, 'w', encoding='utf-8') as out_f:
        out_f.write("# USBArmyKnife Code Documentation\n\n")
        out_f.write("This document contains automatically extracted documentation for functions and classes in the codebase.\n\n")
        
        for fp in files_to_parse:
            docs = extract_docs_from_file(fp)
            if docs:
                out_f.write(docs)
                
    print(f"Documentation generated at {out_path}")

if __name__ == "__main__":
    main()
