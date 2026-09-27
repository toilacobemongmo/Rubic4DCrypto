"""
Convert haogianguyen_vi.tex to clean, human-readable plain text (.txt)
"""

import re
import os

def clean_latex(tex_path, txt_path):
    with open(tex_path, 'r', encoding='utf-8') as f:
        raw_content = f.read()

    # 1. Remove comments
    lines = []
    for line in raw_content.splitlines():
        line_no_comment = re.sub(r'(?<!\\)%.*$', '', line)
        lines.append(line_no_comment)
    text = '\n'.join(lines)

    # 2. Extract Title
    title = "Hệ Mã Khối Đối Xứng 128-bit Tích Hợp Hoán Vị Siêu Lập Phương 4D và Khuếch Tán ARX Cho Môi Trường Thông Lượng Cao"

    # 3. Extract Abstract
    abs_match = re.search(r'\\textbf\{Tóm tắt\}—(.*?)\\vspace', text, re.DOTALL)
    abstract = abs_match.group(1).strip() if abs_match else ""

    # 4. Extract Keywords
    kw_match = re.search(r'\\noindent\\textbf\{Từ khóa:\}\s*(.*?)\\end\{quote\}', text, re.DOTALL)
    keywords = kw_match.group(1).strip() if kw_match else ""

    # 5. Extract body after \twocolumn[...]
    body_idx = text.find(r'\section{Giới thiệu}')
    if body_idx != -1:
        body = text[body_idx:]
    else:
        body = text

    # Helper function to clean LaTeX tags
    def text_clean(s):
        # Section titles
        s = re.sub(r'\\section\{([^}]+)\}', r'\n\n================================================================================\n\1\n================================================================================\n', s)
        s = re.sub(r'\\subsection\{([^}]+)\}', r'\n\n--- \1 ---\n', s)
        s = re.sub(r'\\subsubsection\{([^}]+)\}', r'\n* \1:\n', s)

        # Basic text formatting
        s = re.sub(r'\\textbf\{([^}]+)\}', r'\1', s)
        s = re.sub(r'\\textit\{([^}]+)\}', r'\1', s)
        s = re.sub(r'\\texttt\{([^}]+)\}', r'\1', s)
        s = re.sub(r'\\MakeUppercase\{([^}]+)\}', r'\1', s)

        # Labels, citations, refs
        s = re.sub(r'\\label\{[^}]+\}', '', s)
        s = re.sub(r'\\cite\{([^}]+)\}', r'[\1]', s)
        s = re.sub(r'\\ref\{([^}]+)\}', r'(\1)', s)
        s = re.sub(r'\\eqref\{([^}]+)\}', r'(\1)', s)
        s = re.sub(r'\\href\{[^}]+\}\{([^}]+)\}', r'\1', s)
        s = re.sub(r'\\url\{([^}]+)\}', r'\1', s)

        # Lists
        s = re.sub(r'\\begin\{itemize\}', '', s)
        s = re.sub(r'\\end\{itemize\}', '', s)
        s = re.sub(r'\\begin\{enumerate\}', '', s)
        s = re.sub(r'\\end\{enumerate\}', '', s)
        s = re.sub(r'\\item\s*', '\n• ', s)

        # Clean figures, tables, algorithms
        s = re.sub(r'\\begin\{figure\*?\}.*?\\end\{figure\*?\}', '\n[Hình ảnh minh họa kiến trúc / sơ đồ]\n', s, flags=re.DOTALL)
        s = re.sub(r'\\begin\{table\*?\}.*?\\end\{table\*?\}', '\n[Bảng số liệu đối chuẩn & phân tích]\n', s, flags=re.DOTALL)
        s = re.sub(r'\\begin\{algorithm\}.*?\\end\{algorithm\}', '\n[Đặc tả giải thuật hình thức]\n', s, flags=re.DOTALL)

        # Bibliography environment
        s = re.sub(r'\\begin\{thebibliography\}\{00\}', '\n\n================================================================================\nDANH MỤC TÀI LIỆU THAM KHẢO\n================================================================================\n', s)
        s = re.sub(r'\\end\{thebibliography\}', '', s)
        s = re.sub(r'\\bibitem\{([^}]+)\}', r'\n[\1] ', s)

        # Math formulas
        s = re.sub(r'\\begin\{equation\*?\}(.*?)\\end\{equation\*?\}', r'\n   [Công thức: \1]\n', s, flags=re.DOTALL)
        s = re.sub(r'\\begin\{align\*?\}(.*?)\\end\{align\*?\}', r'\n   [Hệ phương trình: \1]\n', s, flags=re.DOTALL)
        s = re.sub(r'\$([^$]+)\$', r'\1', s)

        # Math symbols
        s = s.replace(r'\oplus', ' XOR ')
        s = s.replace(r'\boxplus', ' [+] ')
        s = s.replace(r'\mathbin{\Vert}', ' || ')
        s = s.replace(r'\times', ' x ')
        s = s.replace(r'\cdot', ' . ')
        s = s.replace(r'\le', ' <= ')
        s = s.replace(r'\ge', ' >= ')
        s = s.replace(r'\approx', ' ~ ')
        s = s.replace(r'\neq', ' != ')
        s = s.replace(r'\to', ' -> ')
        s = s.replace(r'\in', ' thuộc ')
        s = s.replace(r'\mathbb{F}', 'GF')
        s = s.replace(r'\gg', ' >> ')
        s = s.replace(r'\ll', ' << ')
        s = s.replace(r'\_', '_')
        s = s.replace(r'~', ' ')
        s = s.replace(r'\%', '%')
        s = s.replace(r'\small', '')
        s = s.replace(r'\setlength{\itemsep}{1.5pt plus 0.5pt minus 0.5pt}', '')
        s = s.replace(r'\end{document}', '')

        # Remove remaining \command
        s = re.sub(r'\\[a-zA-Z]+', '', s)
        s = re.sub(r'[{}]', '', s)

        # Clean multiple spaces and blank lines
        s = re.sub(r'[ \t]+', ' ', s)
        s = re.sub(r'\n{3,}', '\n\n', s)
        return s

    clean_body = text_clean(body)
    clean_abs = text_clean(abstract).strip()
    clean_kw = text_clean(keywords).strip()

    out_lines = [
        "================================================================================",
        "BẢN TOÀN VĂN TIẾNG VIỆT - BÀI BÁO KHOA HỌC HỆ MÃ RUBIK-4D",
        "(Bản văn bản thuần .txt phục vụ biên tập, rà soát và Humanize)",
        "================================================================================",
        "",
        f"TIÊU ĐỀ: {title}",
        "",
        "TÁC GIẢ: Nguyễn Gia Hào, Quản Thế Trọng",
        "ĐƠN VỊ: Học viện Công nghệ Bưu chính Viễn thông (PTIT), Hà Nội, Việt Nam",
        "",
        "--------------------------------------------------------------------------------",
        "TÓM TẮT (ABSTRACT):",
        "--------------------------------------------------------------------------------",
        clean_abs,
        "",
        f"TỪ KHÓA: {clean_kw}",
        "",
        clean_body.strip(),
        "",
        "================================================================================",
        "HẾT TOÀN VĂN BẢN THẢO BÀI BÁO",
        "================================================================================"
    ]

    output_text = '\n'.join(out_lines)

    with open(txt_path, 'w', encoding='utf-8') as f:
        f.write(output_text)

    print(f"Exported clean text successfully to: {txt_path} ({len(output_text)} characters)")

if __name__ == '__main__':
    clean_latex('draft/haogianguyen_vi.tex', 'draft/bai_bao_tieng_viet_rubik4d.txt')
